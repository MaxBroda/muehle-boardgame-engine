#pragma once

#include <array>
#include <vector>

#include "Types.h"

namespace muehle {

// Das Spielfeld. Haelt die Belegung der 24 Felder und bietet die elementaren
// Operationen darauf. Die Brett-Topologie (welche Felder benachbart sind und
// welche 16 Linien eine Muehle bilden) liegt als statische Tabelle in der
// Implementierung; sie haengt nur von der Feld-Nummerierung in Types.h ab.
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

    // Sind zwei Felder direkt benachbart?
    bool areAdjacent(Field a, Field b) const;

    // Die direkt benachbarten Felder von f (zwischen zwei und vier Stueck).
    // Leer, wenn f kein gueltiges Feld ist.
    std::vector<Field> neighbors(Field f) const;

    // Liegt das Feld f in einer vollstaendigen Muehle der Farbe c, gehoert also
    // zu einer der 16 Linien, auf der alle drei Felder mit c belegt sind?
    bool formsMill(Field f, Color c) const;

    // Alle aktuell freien Felder.
    std::vector<Field> emptyFields() const;

    // Anzahl der Steine einer Farbe auf dem Brett.
    int stoneCount(Color c) const;

    // Anzahl der Muehlen-Linien, auf denen genau zwei Steine der Farbe c liegen
    // und das dritte Feld frei ist (also fast fertige Muehlen). Dient der
    // KI-Bewertung als Mass fuer Drohpotenzial.
    int twoInLineCount(Color c) const;

private:
    // Belegung der 24 Felder. Index = Feldnummer.
    std::array<Color, kFieldCount> fields_;
};

} // namespace muehle
