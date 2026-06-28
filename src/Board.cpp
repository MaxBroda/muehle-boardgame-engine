#include "Board.h"

namespace muehle {

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
    // TODO Sprint B: Adjazenz ueber die statische Topologie-Tabelle pruefen.
    (void)a;
    (void)b;
    return false;
}

bool Board::formsMill(Field f, Color c) const {
    // TODO Sprint B: gegen die 16 Muehlen-Linien pruefen.
    (void)f;
    (void)c;
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
