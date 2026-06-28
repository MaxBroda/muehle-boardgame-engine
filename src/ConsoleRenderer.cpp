#include "ConsoleRenderer.h"

#include <array>
#include <iostream>
#include <string>
#include <vector>

#include "Board.h"
#include "Types.h"

namespace muehle {

namespace {

// ANSI-Sequenzen. Weiss erscheint gelb, Schwarz blau, damit beide Farben auf
// hellen wie dunklen Terminals gut sichtbar sind.
const char* kReset = "\033[0m";
const char* kWhite = "\033[1;33m";  // gelb, kraeftig
const char* kBlack = "\033[1;34m";  // blau, kraeftig
const char* kDim = "\033[2m";       // gedaempft fuer leere Felder

// Liefert das gefaerbte Einzelzeichen fuer ein Feld: ein gefuellter Kreis fuer
// einen Stein (gelb fuer Weiss, blau fuer Schwarz) und ein gedaempfter
// Mittelpunkt fuer ein leeres Feld. Die Ausrichtung des Bretts haengt am festen
// Raster in drawBoard, nicht an der Breite dieser Zeichen.
std::string symbol(Color c) {
    switch (c) {
        case Color::White: return std::string(kWhite) + "●" + kReset;
        case Color::Black: return std::string(kBlack) + "●" + kReset;
        default:           return std::string(kDim) + "·" + kReset;
    }
}

} // namespace

void ConsoleRenderer::drawBoard(const Board& board) const {
    // Das Brett wird auf einer festen Leinwand mit 13 Zeilen und 25 Spalten
    // gezeichnet. Jedes Feld hat eine feste Position; die Verbindungslinien
    // werden rechnerisch zwischen die Felder gesetzt. So bleibt die Ausrichtung
    // unabhaengig von der Strichlaenge immer korrekt.
    const int height = 13;
    const int width = 25;

    // Spalte und Gitterzeile (0..12) jedes Feldes 0..23. Reihenfolge wie in der
    // Feldtabelle in Types.h.
    static const int fieldCol[kFieldCount] = {
        0, 12, 24,        // a7 d7 g7
        3, 12, 21,        // b6 d6 f6
        6, 12, 18,        // c5 d5 e5
        0, 3, 6, 18, 21, 24,  // a4 b4 c4 e4 f4 g4
        6, 12, 18,        // c3 d3 e3
        3, 12, 21,        // b2 d2 f2
        0, 12, 24         // a1 d1 g1
    };
    static const int fieldRow[kFieldCount] = {
        0, 0, 0,
        2, 2, 2,
        4, 4, 4,
        6, 6, 6, 6, 6, 6,
        8, 8, 8,
        10, 10, 10,
        12, 12, 12
    };

    // Waagerechte Verbindungen (Feldpaare in derselben Gitterzeile).
    static const int hEdges[][2] = {
        {0, 1}, {1, 2}, {3, 4}, {4, 5}, {6, 7}, {7, 8},
        {9, 10}, {10, 11}, {12, 13}, {13, 14},
        {15, 16}, {16, 17}, {18, 19}, {19, 20}, {21, 22}, {22, 23}
    };
    // Senkrechte Verbindungen (Feldpaare in derselben Spalte).
    static const int vEdges[][2] = {
        {0, 9}, {9, 21}, {3, 10}, {10, 18}, {6, 11}, {11, 15},
        {1, 4}, {4, 7}, {16, 19}, {19, 22}, {8, 12}, {12, 17},
        {5, 13}, {13, 20}, {2, 14}, {14, 23}
    };

    // Leinwand mit Einzelzeichen-Zellen, anfangs alles Leerzeichen.
    std::vector<std::vector<std::string>> cell(
        height, std::vector<std::string>(width, " "));

    for (const auto& e : hEdges) {
        int row = fieldRow[e[0]];
        for (int c = fieldCol[e[0]] + 1; c < fieldCol[e[1]]; ++c) {
            cell[row][c] = "-";
        }
    }
    for (const auto& e : vEdges) {
        int c = fieldCol[e[0]];
        for (int r = fieldRow[e[0]] + 1; r < fieldRow[e[1]]; ++r) {
            cell[r][c] = "|";
        }
    }
    // Felder zuletzt setzen, damit Steine bzw. Punkte ueber den Linien liegen.
    for (int i = 0; i < kFieldCount; ++i) {
        cell[fieldRow[i]][fieldCol[i]] = symbol(board.colorAt(i));
    }

    std::cout << "\n";
    for (int r = 0; r < height; ++r) {
        // Gerade Gitterzeilen tragen eine Reihen-Nummer (7 oben bis 1 unten).
        std::string prefix = (r % 2 == 0)
            ? std::string(1, static_cast<char>('7' - r / 2)) + "  "
            : "   ";
        std::string line = prefix;
        for (int c = 0; c < width; ++c) {
            line += cell[r][c];
        }
        std::cout << line << "\n";
    }
    std::cout << "\n   a  b  c     d     e  f  g\n";
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
