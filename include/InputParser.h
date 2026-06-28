#pragma once

#include <string>

#include "Move.h"
#include "Types.h"

namespace muehle {

// Wandelt textuelle Benutzereingaben in Datenstrukturen um und prueft dabei nur
// die Syntax, nicht die Spielregeln. Die Regelpruefung bleibt allein Aufgabe
// der Game-Klasse.
class InputParser {
public:
    // Wandelt eine Feldnotation wie "d3" in einen Feldindex 0..23 um.
    // Liefert -1, wenn die Notation ungueltig ist.
    Field parseField(const std::string& text) const;

    // Wandelt eine Eingabe passend zur aktuellen Phase in einen Move um.
    // Liefert true bei syntaktisch gueltiger Eingabe, sonst false.
    bool parseMove(const std::string& text, Phase phase, Move& out) const;
};

} // namespace muehle
