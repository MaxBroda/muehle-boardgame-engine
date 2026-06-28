#include "Board.h"

namespace muehle {

// --- Statische Brett-Topologie ----------------------------------------------
// Beide Tabellen beziehen sich auf die Feld-Nummerierung aus Types.h (siehe
// dortige Skizze). Sie sind das Herzstueck des Bretts und werden zur
// Uebersetzungszeit festgelegt, nicht zur Laufzeit berechnet.
namespace {

// Nachbarschaft je Feld. kNeighbors[f] listet alle direkt verbundenen Felder.
// Jeder Eintrag taucht in beiden Richtungen auf (a Nachbar von b und umgekehrt),
// das Brett hat insgesamt 32 solcher Verbindungen.
const std::array<std::vector<Field>, kFieldCount> kNeighbors = {{
    {1, 9},          // 0  a7
    {0, 2, 4},       // 1  d7
    {1, 14},         // 2  g7
    {4, 10},         // 3  b6
    {1, 3, 5, 7},    // 4  d6
    {4, 13},         // 5  f6
    {7, 11},         // 6  c5
    {4, 6, 8},       // 7  d5
    {7, 12},         // 8  e5
    {0, 10, 21},     // 9  a4
    {3, 9, 11, 18},  // 10 b4
    {6, 10, 15},     // 11 c4
    {8, 13, 17},     // 12 e4
    {5, 12, 14, 20}, // 13 f4
    {2, 13, 23},     // 14 g4
    {11, 16},        // 15 c3
    {15, 17, 19},    // 16 d3
    {12, 16},        // 17 e3
    {10, 19},        // 18 b2
    {16, 18, 20, 22},// 19 d2
    {13, 19},        // 20 f2
    {9, 22},         // 21 a1
    {19, 21, 23},    // 22 d1
    {14, 22}         // 23 g1
}};

// Die 16 Muehlen-Linien: acht waagerechte (je Reihe) und acht senkrechte (je
// Datei). Eine Muehle ist vollstaendig, wenn alle drei Felder dieselbe Farbe
// tragen.
const std::array<std::array<Field, 3>, 16> kMills = {{
    {{0, 1, 2}},    // a7 d7 g7  (Reihe 7)
    {{3, 4, 5}},    // b6 d6 f6  (Reihe 6)
    {{6, 7, 8}},    // c5 d5 e5  (Reihe 5)
    {{9, 10, 11}},  // a4 b4 c4  (Reihe 4 links)
    {{12, 13, 14}}, // e4 f4 g4  (Reihe 4 rechts)
    {{15, 16, 17}}, // c3 d3 e3  (Reihe 3)
    {{18, 19, 20}}, // b2 d2 f2  (Reihe 2)
    {{21, 22, 23}}, // a1 d1 g1  (Reihe 1)
    {{0, 9, 21}},   // a7 a4 a1  (Datei a)
    {{3, 10, 18}},  // b6 b4 b2  (Datei b)
    {{6, 11, 15}},  // c5 c4 c3  (Datei c)
    {{1, 4, 7}},    // d7 d6 d5  (Datei d oben)
    {{16, 19, 22}}, // d3 d2 d1  (Datei d unten)
    {{8, 12, 17}},  // e5 e4 e3  (Datei e)
    {{5, 13, 20}},  // f6 f4 f2  (Datei f)
    {{2, 14, 23}}   // g7 g4 g1  (Datei g)
}};

// Liegt der Feldindex im gueltigen Bereich 0..23?
bool isValidField(Field f) {
    return f >= 0 && f < kFieldCount;
}

} // namespace

Board::Board() {
    reset();
}

void Board::reset() {
    // Alle Felder auf leer setzen.
    for (auto& cell : fields_) {
        cell = Color::None;
    }
}

bool Board::isEmpty(Field f) const {
    return colorAt(f) == Color::None;
}

Color Board::colorAt(Field f) const {
    // Felder ausserhalb des gueltigen Bereichs gelten als leer.
    if (f < 0 || f >= kFieldCount) {
        return Color::None;
    }
    return fields_[static_cast<std::size_t>(f)];
}

void Board::placeStone(Field f, Color c) {
    if (f < 0 || f >= kFieldCount) {
        return;
    }
    fields_[static_cast<std::size_t>(f)] = c;
}

void Board::removeStone(Field f) {
    placeStone(f, Color::None);
}

void Board::moveStone(Field from, Field to) {
    Color c = colorAt(from);
    removeStone(from);
    placeStone(to, c);
}

bool Board::areAdjacent(Field a, Field b) const {
    if (!isValidField(a) || !isValidField(b)) {
        return false;
    }
    const auto& list = kNeighbors[static_cast<std::size_t>(a)];
    for (Field n : list) {
        if (n == b) {
            return true;
        }
    }
    return false;
}

std::vector<Field> Board::neighbors(Field f) const {
    if (!isValidField(f)) {
        return {};
    }
    return kNeighbors[static_cast<std::size_t>(f)];
}

bool Board::formsMill(Field f, Color c) const {
    // Eine leere Farbe kann keine Muehle bilden.
    if (!isValidField(f) || c == Color::None) {
        return false;
    }
    // Jede Linie pruefen, die das Feld f enthaelt: liegen dort drei Steine
    // der Farbe c, ist die Muehle vollstaendig.
    for (const auto& line : kMills) {
        bool contains = (line[0] == f || line[1] == f || line[2] == f);
        if (!contains) {
            continue;
        }
        if (colorAt(line[0]) == c && colorAt(line[1]) == c &&
            colorAt(line[2]) == c) {
            return true;
        }
    }
    return false;
}

std::vector<Field> Board::emptyFields() const {
    std::vector<Field> result;
    for (int i = 0; i < kFieldCount; ++i) {
        if (fields_[static_cast<std::size_t>(i)] == Color::None) {
            result.push_back(i);
        }
    }
    return result;
}

int Board::stoneCount(Color c) const {
    int count = 0;
    for (const auto& cell : fields_) {
        if (cell == c) {
            ++count;
        }
    }
    return count;
}

} // namespace muehle
