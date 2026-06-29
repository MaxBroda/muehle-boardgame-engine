#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "ConsoleRenderer.h"
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

// Zeigt den Kopf eines Zuges: Brett, Spieler am Zug, Phase und Steinzahlen.
void showSituation(const ConsoleRenderer& renderer, const Game& game) {
    renderer.drawBoard(game.board());
    const Player& p = game.currentPlayer();
    renderer.showMessage("");
    renderer.showMessage("Am Zug: " + p.name() + "  (" + phaseName(p.currentPhase()) + ")");
    renderer.showMessage("Steine in der Hand: " + std::to_string(p.stonesInHand()) +
                         ", auf dem Brett: " + std::to_string(p.stonesOnBoard()));
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
            std::to_string(timer.total()) + " ms, im Schnitt " +
            std::to_string(timer.average()) + " ms, laengster " +
            std::to_string(timer.longest()) + " ms");
    };
    renderer.showMessage("");
    renderer.showMessage("Bedenkzeit:");
    report(whiteTimer, Color::White);
    report(blackTimer, Color::Black);
}

// Spielt eine bereits angelegte Partie bis zum Ende oder bis zum Abbruch. Misst
// dabei die Bedenkzeit je Zug und ordnet sie dem Spieler zu, der am Zug war.
void runGameLoop(const ConsoleRenderer& renderer, const InputParser& parser,
                 Game& game) {
    MoveTimer whiteTimer;
    MoveTimer blackTimer;
    while (!game.isGameOver()) {
        if (game.needsRemoval()) {
            if (!handleRemoval(renderer, parser, game)) {
                return;
            }
            continue;
        }
        showSituation(renderer, game);
        Color mover = game.currentPlayer().color();
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
        }
    }

    renderer.drawBoard(game.board());
    Color w = game.winner();
    const std::string& name = game.playerByColor(w).name();
    renderer.showMessage("");
    renderer.showMessage("Spielende. Es gewinnt: " + name + ".");
    showTiming(renderer, game, whiteTimer, blackTimer);
}

// Menuepunkt 1: neue Partie.
void playNewGame(const ConsoleRenderer& renderer, const InputParser& parser) {
    std::string whiteName = askName(renderer, "Spieler 1 (Weiss)");
    std::string blackName = askName(renderer, "Spieler 2 (Schwarz)");
    Game game(whiteName, blackName);
    runGameLoop(renderer, parser, game);
}

// Menuepunkt 2: gespeicherten Spielstand fortsetzen.
void continueGame(const ConsoleRenderer& renderer, const InputParser& parser) {
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
    runGameLoop(renderer, parser, game);
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
    int skipped = 0;
    for (const std::string& path : files) {
        GameResult result;
        if (evaluateLog(path, result)) {
            stats.addResult(result);
        } else {
            ++skipped;  // nicht lesbar oder beschaedigt
        }
    }
    if (stats.totalGames() == 0) {
        renderer.showMessage("Keine auswertbaren Partien gefunden.");
        return;
    }

    renderer.showMessage("");
    renderer.showMessage("Statistik ueber " + std::to_string(stats.totalGames()) +
                         " Partie(n):");
    renderer.showMessage(padRight("Name", 16) + padRight("Partien", 9) +
                         padRight("Siege", 7) + "Niederlagen");
    for (const Statistics::Entry& e : stats.ranking()) {
        renderer.showMessage(padRight(e.name, 16) +
                             padRight(std::to_string(e.games), 9) +
                             padRight(std::to_string(e.wins), 7) +
                             std::to_string(e.losses));
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

int main() {
    using namespace muehle;
    ConsoleRenderer renderer;
    InputParser parser;

    renderer.showMessage("Willkommen bei Muehle.");
    while (true) {
        renderer.showMainMenu();
        std::string choice = renderer.promptInput();
        if (!std::cin) {
            break;
        }
        if (choice == "1") {
            playNewGame(renderer, parser);
        } else if (choice == "2") {
            continueGame(renderer, parser);
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
