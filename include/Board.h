#pragma once

#include <array>
#include <vector>

#include "Types.h"

namespace muehle {

// Das Spielfeld. Haelt die Belegung der 24 Felder und bietet die elementaren
// Operationen darauf. Die Brett-Topologie (welche Felder benachbart sind und
// welche 16 Linien eine Muehle bilden) wird in Sprint B als statische Tabelle
// ergaenzt; die zugehoerigen Methoden sind hier bereits deklariert.
class Board {
public:
    // Legt ein leeres Brett an.
    Board();

    // Setzt alle Felder zurueck auf leer.
    void reset();

    // Ist das Feld frei?
    bool isEmpty(Field f) const;

    // Welche Farbe liegt auf dem Feld? (Color::None, wenn leer)
    Color colorAt(Field f) const;

    // Setzt einen Stein der Farbe c auf das Feld.
    void placeStone(Field f, Color c);

    // Entfernt den Stein von einem Feld.
    void removeStone(Field f);

    // Verschiebt einen Stein von einem Feld auf ein anderes.
    void moveStone(Field from, Field to);

    // Sind zwei Felder direkt benachbart? (Sprint B)
    bool areAdjacent(Field a, Field b) const;

    // Bildet das Feld f zusammen mit zwei weiteren Steinen der Farbe c
    // eine Muehle? (Sprint B)
    bool formsMill(Field f, Color c) const;

    // Alle aktuell freien Felder.
    std::vector<Field> emptyFields() const;

    // Anzahl der Steine einer Farbe auf dem Brett.
    int stoneCount(Color c) const;

private:
    // Belegung der 24 Felder. Index = Feldnummer.
    std::array<Color, kFieldCount> fields_;
};

} // namespace muehle
