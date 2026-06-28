#include "ConsoleRenderer.h"

#include <iostream>
#include <string>

#include "Board.h"

namespace muehle {

void ConsoleRenderer::drawBoard(const Board& board) const {
    // TODO Sprint E: vollstaendiges ASCII-Brett mit Koordinaten und ANSI-Farben.
    // Vorlaeufig nur eine kurze Bestaetigung, dass das Brett angesprochen wird.
    (void)board;
    std::cout << "[Brett wird in Sprint E gezeichnet]\n";
}

void ConsoleRenderer::showMainMenu() const {
    std::cout << "========= MUEHLE =========\n";
    std::cout << "  1) Neues Spiel\n";
    std::cout << "  2) Spielstand fortsetzen\n";
    std::cout << "  3) Protokoll wiedergeben\n";
    std::cout << "  4) Statistik anzeigen\n";
    std::cout << "  5) Beenden\n";
    std::cout << "Auswahl: ";
}

void ConsoleRenderer::showMessage(const std::string& text) const {
    std::cout << text << "\n";
}

std::string ConsoleRenderer::promptInput() const {
    std::string line;
    std::getline(std::cin, line);
    return line;
}

} // namespace muehle
