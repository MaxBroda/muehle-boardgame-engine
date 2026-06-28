#include <iostream>

#include "ConsoleRenderer.h"
#include "Game.h"

// Einstiegspunkt der Konsolenanwendung.
//
// Stand Sprint A: Das Programm beweist, dass Build, Bibliothek und
// Verzeichnisstruktur funktionieren. Es legt eine Partie an, gibt den Spieler
// am Zug aus und zeigt das Hauptmenue. Die eigentliche Spielschleife folgt in
// den spaeteren Sprints.
int main() {
    std::cout << "Hello Muehle\n";
    std::cout << "Muehle-Engine, Geruest aus Sprint A\n\n";

    muehle::Game game("Weiss", "Schwarz");
    std::cout << "Am Zug: " << game.currentPlayer().name() << "\n";
    std::cout << "Steine in der Hand: " << game.currentPlayer().stonesInHand()
              << "\n\n";

    muehle::ConsoleRenderer renderer;
    renderer.showMainMenu();
    std::cout << "\n(noch ohne Funktion, kommt in Sprint E)\n";

    return 0;
}
