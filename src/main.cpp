#include <iostream>
#include <string>

#include "ConsoleRenderer.h"
#include "Game.h"
#include "InputParser.h"
#include "Move.h"
#include "Types.h"

// Einstiegspunkt der Konsolenanwendung. main verbindet die Bausteine zu einem
// spielbaren Ablauf: Hauptmenue, Spielernamen, Spielschleife mit Ein- und
// Ausgabe. Die Spielregeln liegen ausschliesslich in der Game-Klasse, die
// Konsolen-Interaktion ausschliesslich im ConsoleRenderer.

namespace muehle {
namespace {

// Klartextname einer Phase fuer die Anzeige.
std::string phaseName(Phase p) {
    switch (p) {
        case Phase::Placing: return "Setzphase";
        case Phase::Moving:  return "Ziehphase";
        case Phase::Flying:  return "Springphase";
    }
    return "";
}

// Liest einen nicht leeren Spielernamen ein. Bei leerer Eingabe wird ein
// Standardname verwendet.
std::string askName(const ConsoleRenderer& renderer, const std::string& fallback) {
    renderer.showMessage("Name fuer " + fallback + ":");
    std::string name = renderer.promptInput();
    if (name.empty()) {
        return fallback;
    }
    return name;
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

// Behandelt den Schritt, in dem nach einer geschlossenen Muehle ein
// gegnerischer Stein entfernt wird. Liefert false, wenn der Spieler abbricht.
bool handleRemoval(const ConsoleRenderer& renderer, const InputParser& parser,
                   Game& game) {
    // Die erlaubten Ziele als Koordinaten anzeigen.
    std::string list;
    for (Field f : game.removableStones()) {
        if (!list.empty()) list += ", ";
        list += kFieldNames[static_cast<std::size_t>(f)];
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

// Fragt einen regulaeren Zug ab und fuehrt ihn aus. Liefert false, wenn der
// Spieler die Partie verlassen will oder die Eingabe endet.
bool handleMove(const ConsoleRenderer& renderer, const InputParser& parser,
                Game& game) {
    Phase phase = game.currentPlayer().currentPhase();
    if (phase == Phase::Placing) {
        renderer.showMessage("Zug (Feld zum Setzen, z.B. d3) oder 'q' zum Beenden:");
    } else {
        renderer.showMessage("Zug (von-nach, z.B. a1-a4) oder 'q' zum Beenden:");
    }
    while (true) {
        std::string line = renderer.promptInput();
        if (!std::cin) {
            return false;
        }
        if (line == "q" || line == "quit") {
            return false;
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
        return true;
    }
}

// Spielt eine vollstaendige Partie von Anfang bis Ende.
void playGame(const ConsoleRenderer& renderer, const InputParser& parser) {
    std::string whiteName = askName(renderer, "Spieler 1 (Weiss)");
    std::string blackName = askName(renderer, "Spieler 2 (Schwarz)");
    Game game(whiteName, blackName);

    while (!game.isGameOver()) {
        if (game.needsRemoval()) {
            if (!handleRemoval(renderer, parser, game)) {
                renderer.showMessage("Partie abgebrochen.");
                return;
            }
            continue;
        }
        showSituation(renderer, game);
        if (!handleMove(renderer, parser, game)) {
            renderer.showMessage("Partie abgebrochen.");
            return;
        }
    }

    renderer.drawBoard(game.board());
    Color w = game.winner();
    std::string winnerName = (w == Color::White) ? whiteName : blackName;
    renderer.showMessage("");
    renderer.showMessage("Spielende. Es gewinnt: " + winnerName + ".");
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
            playGame(renderer, parser);
        } else if (choice == "2" || choice == "3" || choice == "4") {
            renderer.showMessage("Diese Funktion folgt in einem spaeteren Sprint.");
        } else if (choice == "5" || choice == "q" || choice == "quit") {
            break;
        } else {
            renderer.showMessage("Bitte 1 bis 5 waehlen.");
        }
    }
    renderer.showMessage("Auf Wiedersehen.");
    return 0;
}
