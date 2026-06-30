#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "AiPlayer.h"
#include "CommandLine.h"
#include "ConsoleRenderer.h"
#include "EventLog.h"
#include "Game.h"
#include "InputParser.h"
#include "Move.h"
#include "MoveLogger.h"
#include "MoveTimer.h"
#include "Statistics.h"
#include "Types.h"

// Einstiegspunkt der Konsolenanwendung. main verbindet die Bausteine zu einem
// spielbaren Ablauf: Hauptmenue, Spielernamen, Spielschleife, Speichern,
// Fortsetzen und Wiedergabe. Die Spielregeln liegen ausschliesslich in der
// Game-Klasse, die Konsolen-Interaktion ausschliesslich im ConsoleRenderer.

namespace muehle {
namespace {

namespace fs = std::filesystem;

// Ordner, in dem Spielstaende und Protokolle liegen. Relativ zum Arbeits-
// verzeichnis; das Programm wird laut README aus dem Projektwurzel-Ordner
// gestartet.
const char* kSavesDir = "data";

// Suchtiefe des KI-Gegners in Halbzuegen. Tief genug, um Drohungen zu blockieren
// und Muehlen vorzubereiten, und auch ohne optimierten Build sofort spielbar.
const int kAiDepth = 4;

// Ausgang eines Zug-Dialogs.
enum class Turn { Applied, Undone, Quit, EndOfInput };

// Legt den Speicherordner an, falls er fehlt.
void ensureSavesDir() {
    std::error_code ec;
    fs::create_directories(kSavesDir, ec);
}

// Alle .txt-Dateien im Speicherordner, alphabetisch sortiert.
std::vector<std::string> listSaveFiles() {
    std::vector<std::string> files;
    std::error_code ec;
    if (fs::exists(kSavesDir, ec)) {
        for (const auto& entry : fs::directory_iterator(kSavesDir, ec)) {
            if (entry.is_regular_file() && entry.path().extension() == ".txt") {
                files.push_back(entry.path().string());
            }
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

// Prueft, ob ein Text nur aus Ziffern besteht.
bool isAllDigits(const std::string& s) {
    if (s.empty()) {
        return false;
    }
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

// Kurze, lesbare Notation eines Zugs fuer den Hinweis-Modus. Setzzug als
// einzelnes Feld, Zieh- oder Springzug als "von-nach", Entfernen als "x feld".
std::string describeMove(const Move& m) {
    if (m.removed != -1 && m.to == -1) {
        return "x " + fieldName(m.removed);
    }
    if (m.from == -1) {
        return fieldName(m.to);
    }
    return fieldName(m.from) + "-" + fieldName(m.to);
}

// Das passende Tatwort zu einer Zugart fuer lesbare Meldungen.
std::string moveVerb(MoveType type) {
    return (type == MoveType::Place)  ? "setzt"
           : (type == MoveType::Slide) ? "zieht"
                                       : "springt";
}

// Beschreibt einen kompletten Computerzug in einem natuerlichen Satz. Quell- und
// Zielfeld werden ausgeschrieben, damit klar ist, von wo nach wo gezogen wurde,
// und ein eventuelles Entfernen gehoert sichtbar zum selben Zug.
std::string describeAiTurn(const Move& move, Field removed) {
    std::string s = "Computer ";
    if (move.type == MoveType::Place) {
        s += "setzt einen Stein auf " + fieldName(move.to);
    } else {
        s += (move.type == MoveType::Slide ? "zieht von " : "springt von ") +
             fieldName(move.from) + " nach " + fieldName(move.to);
    }
    if (removed != -1) {
        s += " und entfernt den gegnerischen Stein auf " + fieldName(removed);
    }
    s += ".";
    return s;
}

// Zeigt alle aktuell gueltigen Zuege als durchgehende Zeile.
void showHints(const ConsoleRenderer& renderer, const Game& game) {
    std::vector<Move> moves = game.legalMoves();
    if (moves.empty()) {
        renderer.showMessage("Keine gueltigen Zuege.");
        return;
    }
    std::string list;
    for (const Move& m : moves) {
        if (!list.empty()) list += ", ";
        list += describeMove(m);
    }
    renderer.showMessage("Moegliche Zuege (" + std::to_string(moves.size()) +
                         "): " + list);
}

// Klartextname einer Phase fuer die Anzeige.
std::string phaseName(Phase p) {
    switch (p) {
        case Phase::Placing: return "Setzphase";
        case Phase::Moving:  return "Ziehphase";
        case Phase::Flying:  return "Springphase";
    }
    return "";
}

// Klartextname einer Farbe fuer die Anzeige.
std::string colorName(Color c) {
    if (c == Color::White) return "Weiss";
    if (c == Color::Black) return "Schwarz";
    return "?";
}

// Liest einen nicht leeren Spielernamen ein. Bei leerer Eingabe gilt der
// Standardname.
std::string askName(const ConsoleRenderer& renderer, const std::string& fallback) {
    renderer.showMessage("Name fuer " + fallback + ":");
    std::string name = renderer.promptInput();
    return name.empty() ? fallback : name;
}

// Zeigt die vorhandenen Dateien als nummerierte Liste und liefert den gewaehlten
// Pfad zurueck. Der Benutzer kann eine Nummer waehlen oder einen Pfad tippen.
// Leere Eingabe bedeutet Abbruch (leerer Rueckgabewert).
std::string chooseSaveFile(const ConsoleRenderer& renderer,
                           const std::string& verb) {
    std::vector<std::string> files = listSaveFiles();
    if (files.empty()) {
        renderer.showMessage("Keine Dateien im Ordner '" + std::string(kSavesDir) +
                             "' gefunden. Pfad eingeben oder mit Enter abbrechen:");
    } else {
        renderer.showMessage("Datei zum " + verb + " waehlen:");
        for (std::size_t i = 0; i < files.size(); ++i) {
            renderer.showMessage("  " + std::to_string(i + 1) + ") " + files[i]);
        }
        renderer.showMessage("Nummer waehlen, Pfad eingeben oder Enter zum Abbrechen:");
    }
    std::string input = renderer.promptInput();
    if (input.empty()) {
        return "";
    }
    if (isAllDigits(input)) {
        std::size_t n = 0;
        try {
            n = static_cast<std::size_t>(std::stoul(input));
        } catch (...) {
            // Unrealistisch grosse Zahl: wie eine ungueltige Nummer behandeln,
            // statt mit einer Ausnahme abzustuerzen.
            return "";
        }
        if (n >= 1 && n <= files.size()) {
            return files[n - 1];
        }
        return "";  // ungueltige Nummer
    }
    return input;  // als Pfad behandeln
}

// Baut aus einem Namen einen Speicherpfad im Speicherordner. Enthaelt der Name
// einen Schraegstrich, wird er als vollstaendiger Pfad uebernommen.
std::string makeSavePath(std::string name) {
    if (name.find('/') != std::string::npos) {
        return name;
    }
    if (name.size() < 4 || name.substr(name.size() - 4) != ".txt") {
        name += ".txt";
    }
    return std::string(kSavesDir) + "/" + name;
}

// Baut einen eindeutigen Pfad mit Zeitstempel im Speicherordner, etwa
// "data/partie_20260629_153012.txt". So lassen sich Partien und Logs ohne
// Rueckfrage und ohne eine fruehere Datei zu ueberschreiben ablegen. Die
// Endung steuert auch, ob die Statistik die Datei beachtet: nur .txt zaehlt.
std::string timestampedPath(const std::string& prefix, const std::string& ext) {
    std::time_t now = std::time(nullptr);
    char stamp[32] = {0};
    std::strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", std::localtime(&now));
    return std::string(kSavesDir) + "/" + prefix + "_" + stamp + ext;
}

// Baut die Infospalte rechts neben dem Brett: Spieler am Zug, die Bedenkzeit des
// letzten Zugs beider Spieler und die Steinzahlen. Gesamtzeit und Durchschnitt
// bleiben bewusst der Auswertung am Spielende vorbehalten, damit die Ansicht
// waehrend des Spiels uebersichtlich bleibt.
std::vector<std::string> buildSidebar(const Game& game, const MoveTimer& whiteTimer,
                                      const MoveTimer& blackTimer) {
    const Player& white = game.playerByColor(Color::White);
    const Player& black = game.playerByColor(Color::Black);
    Color toMove = game.currentPlayer().color();

    // Namen auf gemeinsame Breite bringen, damit die Werte buendig stehen.
    std::size_t nameWidth = std::max(white.name().size(), black.name().size());
    auto pad = [&](const std::string& s) {
        return s.size() < nameWidth ? s + std::string(nameWidth - s.size(), ' ')
                                     : s;
    };
    auto lastTime = [](const MoveTimer& t) {
        return t.count() == 0 ? std::string("noch kein Zug")
                              : MoveTimer::formatSeconds(t.last());
    };
    auto stones = [](const Player& p) {
        return std::to_string(p.stonesInHand()) + " / " +
               std::to_string(p.stonesOnBoard());
    };

    std::vector<std::string> s;
    s.push_back("Am Zug: " + game.playerByColor(toMove).name() + " (" +
                colorName(toMove) + ")");
    s.push_back("");
    s.push_back("Bedenkzeit (letzter Zug)");
    s.push_back("  " + pad(white.name()) + "  " + lastTime(whiteTimer));
    s.push_back("  " + pad(black.name()) + "  " + lastTime(blackTimer));
    s.push_back("");
    s.push_back("Steine (Hand / Brett)");
    s.push_back("  " + pad(white.name()) + "  " + stones(white));
    s.push_back("  " + pad(black.name()) + "  " + stones(black));
    return s;
}

// Zeigt den Kopf eines Zuges: Brett mit der Infospalte rechts daneben und die
// aktuelle Phase hervorgehoben darunter.
void showSituation(const ConsoleRenderer& renderer, const Game& game,
                   const MoveTimer& whiteTimer, const MoveTimer& blackTimer) {
    renderer.drawBoard(game.board(), buildSidebar(game, whiteTimer, blackTimer));
    renderer.showHighlighted("Phase: " +
                             phaseName(game.currentPlayer().currentPhase()));
}

// Liefert einen Hinweis, wenn ein Spieler in eine neue Phase vorrueckt, sonst
// einen leeren String. Der Text erklaert die neue Phase kurz, damit der Wechsel
// auch fuer Neulinge verstaendlich ist (besonders die Springphase). Rueckwege
// (etwa durch Undo) loesen bewusst keinen Hinweis aus.
std::string phaseChangeMessage(const std::string& name, Phase from, Phase to) {
    if (from == Phase::Placing && to == Phase::Moving) {
        return name + " hat alle Steine gesetzt und ist jetzt in der Ziehphase. "
                      "Steine wandern auf ein benachbartes freies Feld.";
    }
    if (to == Phase::Flying && from != Phase::Flying) {
        return name + " hat nur noch drei Steine und ist in der Springphase. "
                      "Steine duerfen auf ein beliebiges freies Feld springen.";
    }
    return "";
}

// Behandelt das Entfernen eines gegnerischen Steins nach einer Muehle.
// Liefert false, wenn die Eingabe endet.
bool handleRemoval(const ConsoleRenderer& renderer, const InputParser& parser,
                   Game& game) {
    std::string list;
    for (Field f : game.removableStones()) {
        if (!list.empty()) list += ", ";
        list += fieldName(f);
    }
    renderer.showMessage("Muehle geschlossen. Gegnerischen Stein entfernen ("
                         + list + "):");
    while (true) {
        std::string line = renderer.promptInput();
        if (!std::cin) {
            return false;
        }
        Field target = parser.parseField(line);
        if (target < 0) {
            renderer.showMessage("Bitte ein gueltiges Feld angeben.");
            continue;
        }
        Move m;
        m.removed = target;
        std::string reason;
        if (!game.validateMove(m, reason)) {
            renderer.showMessage(reason);
            continue;
        }
        game.applyMove(m);
        return true;
    }
}

// Fragt einen regulaeren Zug ab und fuehrt ihn aus.
Turn handleMove(const ConsoleRenderer& renderer, const InputParser& parser,
                Game& game) {
    Phase phase = game.currentPlayer().currentPhase();
    if (phase == Phase::Placing) {
        renderer.showMessage("Zug (Feld z.B. d3), 'h' Hinweis, 'u' Undo oder 'q' Beenden:");
    } else {
        renderer.showMessage("Zug (von-nach z.B. a1-a4), 'h' Hinweis, 'u' Undo oder 'q' Beenden:");
    }
    while (true) {
        std::string line = renderer.promptInput();
        if (!std::cin) {
            return Turn::EndOfInput;
        }
        if (line == "q" || line == "quit") {
            return Turn::Quit;
        }
        if (line == "h" || line == "hint") {
            showHints(renderer, game);
            continue;
        }
        if (line == "u" || line == "undo") {
            if (game.undoLastMove()) {
                renderer.showMessage("Letzter Zug zurueckgenommen.");
                return Turn::Undone;
            }
            renderer.showMessage("Kein Zug zum Zuruecknehmen vorhanden.");
            continue;
        }
        Move m;
        if (!parser.parseMove(line, phase, m)) {
            renderer.showMessage("Eingabe nicht verstanden. Bitte erneut versuchen.");
            continue;
        }
        std::string reason;
        if (!game.validateMove(m, reason)) {
            renderer.showMessage(reason);
            continue;
        }
        game.applyMove(m);
        return Turn::Applied;
    }
}

// Bietet beim Verlassen an, den Spielstand zu sichern.
void offerSave(const ConsoleRenderer& renderer, const Game& game) {
    renderer.showMessage("Spielstand speichern? (j/n)");
    std::string answer = renderer.promptInput();
    if (answer != "j" && answer != "J") {
        return;
    }
    renderer.showMessage("Name des Spielstands [spielstand]:");
    std::string name = renderer.promptInput();
    if (name.empty()) {
        name = "spielstand";
    }
    std::string path = makeSavePath(name);
    ensureSavesDir();
    MoveLogger logger;
    if (logger.saveSnapshot(path, game)) {
        renderer.showMessage("Gespeichert unter " + path + ".");
    } else {
        renderer.showMessage("Speichern fehlgeschlagen (Pfad pruefen).");
    }
}

// Schreibt einen ausgefuehrten Zug ins Ereignislog (nur wenn dieses aktiv ist).
void logMoveEvent(EventLog& eventLog, const Game& game, Color mover, Phase phase,
                  const Move& done, long long millis) {
    if (!eventLog.isActive()) {
        return;
    }
    std::string message = game.playerByColor(mover).name() + " " +
                          moveVerb(done.type) + " " +
                          describeMove(done) + " (" + phaseName(phase) + ", " +
                          std::to_string(millis) + " ms)";
    if (game.needsRemoval()) {
        message += " - Muehle geschlossen";
    }
    eventLog.log(message);
}

// Zeigt die ausgewertete Bedenkzeit je Spieler nach dem Spielende.
void showTiming(const ConsoleRenderer& renderer, const Game& game,
                const MoveTimer& whiteTimer, const MoveTimer& blackTimer) {
    auto report = [&](const MoveTimer& timer, Color c) {
        if (timer.count() == 0) {
            return;
        }
        renderer.showMessage(
            game.playerByColor(c).name() + ": " +
            std::to_string(timer.count()) + " Zuege, gesamt " +
            MoveTimer::formatSeconds(timer.total()) + ", im Schnitt " +
            MoveTimer::formatSeconds(timer.average()) + ", laengster " +
            MoveTimer::formatSeconds(timer.longest()));
    };
    renderer.showMessage("");
    renderer.showMessage("Bedenkzeit:");
    report(whiteTimer, Color::White);
    report(blackTimer, Color::Black);
}

// Spielt eine bereits angelegte Partie bis zum Ende oder bis zum Abbruch. Misst
// dabei die Bedenkzeit je Zug und ordnet sie dem Spieler zu, der am Zug war.
void runGameLoop(const ConsoleRenderer& renderer, const InputParser& parser,
                 Game& game, EventLog& eventLog, AiPlayer* ai) {
    MoveTimer whiteTimer;
    MoveTimer blackTimer;
    // Zuletzt gesehene Phase je Spieler, um einen Wechsel genau einmal zu melden.
    // Auf den Startzustand gesetzt, damit ein geladener Spielstand keinen
    // Wechsel vortaeuscht.
    Phase prevWhitePhase = game.playerByColor(Color::White).currentPhase();
    Phase prevBlackPhase = game.playerByColor(Color::Black).currentPhase();
    while (!game.isGameOver()) {
        // Phasenwechsel beider Spieler melden, sobald er auftritt.
        auto announcePhase = [&](Color c, Phase& prev) {
            Phase now = game.playerByColor(c).currentPhase();
            std::string msg =
                phaseChangeMessage(game.playerByColor(c).name(), prev, now);
            if (!msg.empty()) {
                renderer.showHighlighted(msg);
            }
            prev = now;
        };
        announcePhase(Color::White, prevWhitePhase);
        announcePhase(Color::Black, prevBlackPhase);

        // Ist der Computer am Zug, spielt er seinen vollstaendigen Zug selbst.
        // Schliesst der Zug eine Muehle, gehoert das Entfernen zum selben Zug und
        // wird gemeinsam beschrieben, damit der Mensch ihn als eine Einheit
        // nachvollziehen kann.
        Color toMove = game.currentPlayer().color();
        if (ai != nullptr && toMove == ai->color()) {
            Phase aiPhase = game.currentPlayer().currentPhase();
            auto start = std::chrono::steady_clock::now();
            Move move;
            if (!ai->chooseMove(game, move)) {
                break;  // bei laufender Partie nicht zu erwarten
            }
            game.applyMove(move);
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - start)
                               .count();
            // Die Rechenzeit wie einen menschlichen Zug erfassen.
            (toMove == Color::White ? whiteTimer : blackTimer).record(elapsed);
            if (eventLog.isActive() && !game.history().empty()) {
                logMoveEvent(eventLog, game, toMove, aiPhase,
                             game.history().back(), elapsed);
            }

            // Schliesst der Zug eine Muehle, im selben Schritt den entfernten
            // Stein bestimmen und entfernen.
            Field removed = -1;
            if (game.needsRemoval()) {
                Move removal;
                if (ai->chooseMove(game, removal)) {
                    game.applyMove(removal);
                    removed = game.history().back().removed;
                    if (eventLog.isActive()) {
                        eventLog.log(game.playerByColor(toMove).name() +
                                     " entfernt " + fieldName(removed));
                    }
                }
            }

            // Nur den Zug in Worten ausgeben. Die Endstellung zeichnet die
            // naechste Situationsanzeige (oder der Endstand); so wird jede
            // Stellung genau einmal gezeichnet und nichts flackert.
            renderer.showMessage("");
            renderer.showMessage(describeAiTurn(move, removed));
            continue;
        }
        if (game.needsRemoval()) {
            // Der entfernende Spieler bleibt am Zug; Name vor dem Entfernen merken.
            std::string remover = game.currentPlayer().name();
            if (!handleRemoval(renderer, parser, game)) {
                return;
            }
            if (eventLog.isActive() && !game.history().empty()) {
                eventLog.log(remover + " entfernt " +
                             fieldName(game.history().back().removed));
            }
            continue;
        }
        showSituation(renderer, game, whiteTimer, blackTimer);
        Color mover = game.currentPlayer().color();
        Phase phase = game.currentPlayer().currentPhase();
        auto start = std::chrono::steady_clock::now();
        Turn result = handleMove(renderer, parser, game);
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - start)
                           .count();
        if (result == Turn::EndOfInput) {
            return;
        }
        if (result == Turn::Quit) {
            offerSave(renderer, game);
            renderer.showMessage("Zurueck zum Hauptmenue.");
            return;
        }
        // Nur ein tatsaechlich ausgefuehrter Zug zaehlt; ein Undo nicht.
        if (result == Turn::Applied) {
            (mover == Color::White ? whiteTimer : blackTimer).record(elapsed);
            if (!game.history().empty()) {
                logMoveEvent(eventLog, game, mover, phase,
                             game.history().back(), elapsed);
            }
        }
    }

    renderer.drawBoard(game.board());
    Color w = game.winner();
    const std::string& name = game.playerByColor(w).name();
    renderer.showMessage("");
    renderer.showMessage("Spielende. Es gewinnt: " + name + ".");
    showTiming(renderer, game, whiteTimer, blackTimer);

    // Eine zu Ende gespielte Partie automatisch als Protokoll sichern, damit sie
    // in die spieluebergreifende Statistik einfliesst, auch ohne manuelles
    // Speichern zwischendurch.
    ensureSavesDir();
    std::string path = timestampedPath("partie", ".txt");
    MoveLogger logger;
    if (logger.saveSnapshot(path, game)) {
        renderer.showMessage("Partie als Protokoll gespeichert: " + path);
    }
    if (eventLog.isActive()) {
        eventLog.log("Partie beendet, Sieger: " + name);
    }
}

// Menuepunkt 1: neue Partie.
void playNewGame(const ConsoleRenderer& renderer, const InputParser& parser,
                 EventLog& eventLog) {
    renderer.showMessage("Gegner waehlen: 1) zweiter Mensch  2) Computer");
    std::string opp = renderer.promptInput();
    bool vsComputer = (opp == "2");

    // Der KI-Gegner wird immer angelegt (das kostet nichts), aber nur bei Wahl
    // des Computers tatsaechlich verwendet. Er spielt Schwarz, der Mensch beginnt
    // als Weiss.
    AiPlayer computer(Color::Black, kAiDepth);
    AiPlayer* ai = vsComputer ? &computer : nullptr;

    std::string whiteName;
    std::string blackName;
    if (vsComputer) {
        whiteName = askName(renderer, "Spieler (Weiss)");
        blackName = "Computer";
    } else {
        whiteName = askName(renderer, "Spieler 1 (Weiss)");
        blackName = askName(renderer, "Spieler 2 (Schwarz)");
    }
    if (eventLog.isActive()) {
        eventLog.log("Neue Partie: " + whiteName + " (Weiss) gegen " + blackName +
                     " (Schwarz)" +
                     (vsComputer ? ", Computer spielt Schwarz" : ""));
    }
    Game game(whiteName, blackName);
    runGameLoop(renderer, parser, game, eventLog, ai);
}

// Menuepunkt 2: gespeicherten Spielstand fortsetzen.
void continueGame(const ConsoleRenderer& renderer, const InputParser& parser,
                  EventLog& eventLog) {
    std::string path = chooseSaveFile(renderer, "Fortsetzen");
    if (path.empty()) {
        renderer.showMessage("Abgebrochen.");
        return;
    }
    MoveLogger logger;
    std::string whiteName;
    std::string blackName;
    if (!logger.readHeader(path, whiteName, blackName)) {
        renderer.showMessage("Datei konnte nicht gelesen werden.");
        return;
    }
    Game game(whiteName, blackName);
    if (!logger.loadSnapshot(path, game)) {
        renderer.showMessage("Spielstand ist beschaedigt und wurde nicht geladen.");
        return;
    }
    renderer.showMessage("Spielstand geladen. Weiter geht es.");
    if (eventLog.isActive()) {
        eventLog.log("Spielstand fortgesetzt: " + whiteName + " gegen " +
                     blackName);
    }
    // Ein fortgesetzter Spielstand wird von zwei Menschen weitergespielt; das
    // Dateiformat haelt keine KI-Information fest.
    runGameLoop(renderer, parser, game, eventLog, nullptr);
}

// Haengt Leerzeichen an, bis der Text die gewuenschte Breite hat. Fuer eine
// einfache, ausgerichtete Tabelle ohne <iomanip>.
std::string padRight(std::string text, std::size_t width) {
    if (text.size() < width) {
        text.append(width - text.size(), ' ');
    }
    return text;
}

// Menuepunkt 4: spieluebergreifende Statistik ueber alle gespeicherten Partien.
void showStatistics(const ConsoleRenderer& renderer) {
    std::vector<std::string> files = listSaveFiles();
    if (files.empty()) {
        renderer.showMessage("Keine gespeicherten Partien im Ordner '" +
                             std::string(kSavesDir) + "'.");
        return;
    }
    Statistics stats;
    int skipped = 0;  // nicht lesbar oder beschaedigt
    int open = 0;     // offener Zwischenstand ohne Ergebnis
    for (const std::string& path : files) {
        GameResult result;
        if (!evaluateLog(path, result)) {
            ++skipped;
            continue;
        }
        // Nur abgeschlossene Partien fliessen in die Sieg-Statistik ein; ein
        // gespeicherter Zwischenstand hat keinen Gewinner und wird nur gezaehlt.
        if (result.winner == Color::None) {
            ++open;
            continue;
        }
        stats.addResult(result);
    }
    if (stats.totalGames() == 0) {
        renderer.showMessage("Keine abgeschlossenen Partien gefunden.");
        if (open > 0) {
            renderer.showMessage(std::to_string(open) +
                                 " offene(r) Zwischenstand(e) nicht gewertet.");
        }
        return;
    }

    renderer.showMessage("");
    renderer.showMessage("Statistik ueber " + std::to_string(stats.totalGames()) +
                         " abgeschlossene Partie(n):");
    renderer.showMessage(padRight("Name", 16) + padRight("Partien", 9) +
                         padRight("Siege", 7) + "Niederlagen");
    for (const Statistics::Entry& e : stats.ranking()) {
        renderer.showMessage(padRight(e.name, 16) +
                             padRight(std::to_string(e.games), 9) +
                             padRight(std::to_string(e.wins), 7) +
                             std::to_string(e.losses));
    }
    if (open > 0) {
        renderer.showMessage(std::to_string(open) +
                             " offene(r) Zwischenstand(e) nicht gewertet.");
    }
    if (skipped > 0) {
        renderer.showMessage(std::to_string(skipped) +
                             " Datei(en) nicht auswertbar, uebersprungen.");
    }
}

// Menuepunkt 3: ein Protokoll wiedergeben, am Stueck oder schrittweise.
void replayProtocol(const ConsoleRenderer& renderer) {
    std::string path = chooseSaveFile(renderer, "Wiedergeben");
    if (path.empty()) {
        renderer.showMessage("Abgebrochen.");
        return;
    }
    MoveLogger logger;
    std::string whiteName;
    std::string blackName;
    if (!logger.readHeader(path, whiteName, blackName)) {
        renderer.showMessage("Datei konnte nicht gelesen werden.");
        return;
    }
    std::vector<Move> moves;
    if (!logger.loadGame(path, moves)) {
        renderer.showMessage("Protokoll ist beschaedigt.");
        return;
    }

    renderer.showMessage("Wiedergabe: 1) am Stueck  2) schrittweise (Enter pro Zug)");
    std::string mode = renderer.promptInput();
    bool stepwise = (mode == "2");

    Game game(whiteName, blackName);
    for (std::size_t i = 0; i < moves.size(); ++i) {
        if (stepwise) {
            renderer.drawBoard(game.board());
            renderer.showMessage("Zug " + std::to_string(i + 1) + " von " +
                                 std::to_string(moves.size()) + " - Enter:");
            renderer.promptInput();
        }
        std::string reason;
        if (!game.replayLogged(moves[i], reason)) {
            renderer.showMessage("Protokoll beschaedigt bei Zug " +
                                 std::to_string(i + 1) + ": " + reason);
            return;
        }
    }

    renderer.drawBoard(game.board());
    if (game.isGameOver()) {
        renderer.showMessage("Endstand. Gewinner: " +
                             game.playerByColor(game.winner()).name() + ".");
    } else {
        renderer.showMessage("Ende des Protokolls (Partie war noch offen).");
    }
}

} // namespace
} // namespace muehle

int main(int argc, char** argv) {
    using namespace muehle;
    ConsoleRenderer renderer;
    InputParser parser;

    // Argumente (ohne den Programmnamen) auswerten.
    std::vector<std::string> args(argv + 1, argv + argc);
    CommandLineOptions options = parseCommandLine(args);
    if (options.help) {
        renderer.showMessage("Aufruf: muehle [--log[=datei]] [-h|--help]");
        renderer.showMessage("  --log          erweiterte Protokollierung (Dateiname automatisch)");
        renderer.showMessage("  --log=datei    erweiterte Protokollierung in eine eigene Datei");
        renderer.showMessage("  -h, --help     diese Hilfe");
        return 0;
    }
    if (options.unknownOption) {
        renderer.showMessage("Unbekanntes Argument '" + options.unknown +
                             "', wird ignoriert. '--help' zeigt die Optionen.");
    }

    // Bei aktivem Flag das Ereignislog oeffnen. Der Standardpfad liegt im
    // Speicherordner, der dafuer bei Bedarf angelegt wird.
    EventLog eventLog;
    if (options.logging) {
        ensureSavesDir();
        // Ohne ausdruecklichen Pfad automatisch einen eindeutigen Namen je
        // Sitzung vergeben (eigene Endung, damit die Statistik ihn nicht als
        // Spielprotokoll liest).
        std::string logPath =
            options.logPath.empty() ? timestampedPath("log", ".log")
                                    : options.logPath;
        if (eventLog.open(logPath)) {
            renderer.showMessage("Erweiterte Protokollierung aktiv: " + logPath);
        } else {
            renderer.showMessage("Logdatei '" + logPath +
                                 "' konnte nicht geoeffnet werden.");
        }
    }

    renderer.showMessage("Willkommen bei Muehle.");
    while (true) {
        renderer.showMainMenu();
        std::string choice = renderer.promptInput();
        if (!std::cin) {
            break;
        }
        if (choice == "1") {
            playNewGame(renderer, parser, eventLog);
        } else if (choice == "2") {
            continueGame(renderer, parser, eventLog);
        } else if (choice == "3") {
            replayProtocol(renderer);
        } else if (choice == "4") {
            showStatistics(renderer);
        } else if (choice == "5" || choice == "q" || choice == "quit") {
            break;
        } else {
            renderer.showMessage("Bitte 1 bis 5 waehlen.");
        }
    }
    renderer.showMessage("Auf Wiedersehen.");
    return 0;
}
