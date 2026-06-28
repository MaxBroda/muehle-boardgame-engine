#pragma once

#include <string>

#include "Types.h"

namespace muehle {

class Board; // nur als Referenz benoetigt, daher Vorwaertsdeklaration

// Ein Spieler. Kapselt Farbe, Name und die Anzahl der Steine in der Hand und
// auf dem Brett. Die aktuelle Phase wird aus diesen Zahlen abgeleitet.
class Player {
public:
    Player(Color color, std::string name);

    Color color() const;
    const std::string& name() const;

    int stonesInHand() const;
    int stonesOnBoard() const;

    // Leitet die aktuelle Phase aus den Steinzahlen ab.
    Phase currentPhase() const;

    // Hat der Spieler noch Steine in der Hand (also Setzphase)?
    bool hasStonesInHand() const;

    // Gibt es fuer den Spieler ueberhaupt noch einen gueltigen Zug?
    bool canMove(const Board& board) const;

    // Buchhaltung der Steine. Wird von der Game-Klasse aufgerufen, wenn ein
    // Zug ausgefuehrt wird.
    void removeFromHand();
    void addToBoard();
    void removeFromBoard();

private:
    Color color_;
    std::string name_;
    int stonesInHand_;   // anfangs kStonesPerPlayer
    int stonesOnBoard_;  // anfangs 0
};

} // namespace muehle
