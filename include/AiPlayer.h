#pragma once

#include "Game.h"
#include "Move.h"
#include "Types.h"

namespace muehle {

// Ein einfacher KI-Gegner fuer die Muehle-Engine. Er waehlt seinen Zug ueber
// eine begrenzte Minimax-Suche mit Alpha-Beta-Schnitt und einer
// Stellungsbewertung. Die Klasse kennt die Konsole nicht und ist damit voll
// testbar. Sie schreibt keine Regeln neu, sondern probiert Zuege auf Kopien des
// Spiels ueber dieselbe zentrale Validierung wie ein menschlicher Zug.
class AiPlayer {
public:
    // color:       die Farbe, die die KI spielt.
    // searchDepth: Suchtiefe in Halbzuegen (mindestens 1). Groesser bedeutet
    //              staerker, aber langsamer. Werte unter 1 werden auf 1 gehoben.
    AiPlayer(Color color, int searchDepth);

    Color color() const;
    int searchDepth() const;

    // Bewertet eine Stellung aus Sicht von "perspective". Ein positiver Wert ist
    // gut fuer perspective, ein negativer gut fuer den Gegner. Die Bewertung
    // beruht auf dem Materialvorsprung (Steine auf dem Brett und in der Hand) und
    // der Zahl der Steine, die in einer vollstaendigen Muehle stehen. Ein
    // entschiedenes Spiel liefert einen sehr grossen Betrag, damit Sieg und
    // Niederlage jede Feinbewertung ueberlagern.
    static int evaluate(const Game& game, Color perspective);

private:
    Color color_;
    int searchDepth_;
};

} // namespace muehle
