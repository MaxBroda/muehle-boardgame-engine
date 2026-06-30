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
const char* kHighlight = "\033[1;36m";  // fett tuerkis fuer Hervorhebungen

// Faerbt einen Stein: ein gefuellter Kreis, gelb fuer Weiss, blau fuer Schwarz.
std::string stoneSymbol(Color c) {
    return (c == Color::White ? std::string(kWhite) : std::string(kBlack)) +
           "●" + kReset;
}

// Umschliesst ein Zeichen mit der gedaempften Darstellung fuer das Gitter.
std::string dim(const std::string& s) {
    return std::string(kDim) + s + kReset;
}

} // namespace

const char* ConsoleRenderer::nodeGlyph(bool up, bool down, bool left, bool right) {
    // Anschluesse als Bitmaske: oben=1, unten=2, links=4, rechts=8. Jede gueltige
    // Kombination auf dem Muehlebrett bekommt das passende Box-Drawing-Zeichen.
    const int mask = (up ? 1 : 0) | (down ? 2 : 0) | (left ? 4 : 0) | (right ? 8 : 0);
    switch (mask) {
        case 3:  return "│";  // up+down              │
        case 12: return "─";  // left+right           ─
        case 10: return "┌";  // down+right           ┌
        case 6:  return "┐";  // down+left            ┐
        case 9:  return "└";  // up+right             └
        case 5:  return "┘";  // up+left              ┘
        case 14: return "┬";  // down+left+right      ┬
        case 11: return "├";  // up+down+right        ├
        case 13: return "┴";  // up+left+right        ┴
        case 7:  return "┤";  // up+down+left          ┤
        case 15: return "┼";  // up+down+left+right   ┼
        default: return "·";  // Fallback: Mittelpunkt ·
    }
}

void ConsoleRenderer::drawBoard(const Board& board,
                                const std::vector<std::string>& sidebar) const {
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

    // Anschluesse jedes Feldes aus den Kanten ableiten: das linke Ende einer
    // waagerechten Kante hat eine Linie nach rechts, das rechte eine nach links;
    // analog oben/unten bei den senkrechten Kanten. Daraus ergibt sich das
    // Knotenzeichen rechnerisch, ganz ohne Zeichen von Hand zu setzen.
    bool up[kFieldCount] = {};
    bool down[kFieldCount] = {};
    bool left[kFieldCount] = {};
    bool right[kFieldCount] = {};
    for (const auto& e : hEdges) {
        right[e[0]] = true;
        left[e[1]] = true;
    }
    for (const auto& e : vEdges) {
        down[e[0]] = true;
        up[e[1]] = true;
    }

    // Leinwand mit Einzelzeichen-Zellen, anfangs alles Leerzeichen.
    std::vector<std::vector<std::string>> cell(
        height, std::vector<std::string>(width, " "));

    for (const auto& e : hEdges) {
        int row = fieldRow[e[0]];
        for (int c = fieldCol[e[0]] + 1; c < fieldCol[e[1]]; ++c) {
            cell[row][c] = dim("─");
        }
    }
    for (const auto& e : vEdges) {
        int c = fieldCol[e[0]];
        for (int r = fieldRow[e[0]] + 1; r < fieldRow[e[1]]; ++r) {
            cell[r][c] = dim("│");
        }
    }
    // Felder zuletzt setzen, damit sie ueber den Linien liegen. Ein leeres Feld
    // erscheint als passender Gitterknoten, ein besetztes als farbiger Stein.
    for (int i = 0; i < kFieldCount; ++i) {
        Color c = board.colorAt(i);
        cell[fieldRow[i]][fieldCol[i]] =
            (c == Color::None)
                ? dim(nodeGlyph(up[i], down[i], left[i], right[i]))
                : stoneSymbol(c);
    }

    // Abstand zwischen Brett und Infospalte und die Brettzeile, ab der die
    // Infospalte beginnt (eine Zeile Luft oben).
    const int sidebarGap = 4;
    const int sidebarStart = 1;

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
        // Jede Brettzeile ist gleich breit (Prefix plus feste Zellzahl), deshalb
        // beginnt die Infospalte nach einem festen Abstand immer buendig.
        int infoIndex = r - sidebarStart;
        if (infoIndex >= 0 && infoIndex < static_cast<int>(sidebar.size()) &&
            !sidebar[static_cast<std::size_t>(infoIndex)].empty()) {
            line += std::string(sidebarGap, ' ') +
                    sidebar[static_cast<std::size_t>(infoIndex)];
        }
        std::cout << line << "\n";
    }
    std::cout << "\n   a  b  c     d     e  f  g\n";
}

void ConsoleRenderer::showHighlighted(const std::string& text) const {
    std::cout << kHighlight << text << kReset << "\n";
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
