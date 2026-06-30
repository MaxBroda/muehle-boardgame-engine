#pragma once

#include <random>

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
    // color:          die Farbe, die die KI spielt.
    // searchDepth:    Suchtiefe in Halbzuegen (mindestens 1). Groesser bedeutet
    //                 staerker, aber langsamer. Werte unter 1 werden auf 1 gehoben.
    // blunderPercent: Wahrscheinlichkeit in Prozent, statt des besten Zugs einen
    //                 zufaelligen gueltigen Zug zu spielen. So laesst sich die
    //                 Spielstaerke senken. 0 heisst immer bestmoeglich; der Wert
    //                 wird auf den Bereich 0..100 begrenzt.
    AiPlayer(Color color, int searchDepth, int blunderPercent = 0);

    Color color() const;
    int searchDepth() const;
    int blunderPercent() const;

    // Bewertet eine Stellung aus Sicht von "perspective". Ein positiver Wert ist
    // gut fuer perspective, ein negativer gut fuer den Gegner. Die Bewertung
    // setzt sich aus vier Faktoren zusammen, jeweils als Differenz beider Seiten:
    // Materialvorsprung (Steine auf dem Brett und in der Hand), Steine in einer
    // vollstaendigen Muehle, fast fertige Muehlen (zwei eigene Steine plus freies
    // drittes Feld) und Beweglichkeit (moegliche Ziehschritte auf freie
    // Nachbarfelder). Ein entschiedenes Spiel liefert einen sehr grossen Betrag,
    // damit Sieg und Niederlage jede Feinbewertung ueberlagern.
    static int evaluate(const Game& game, Color perspective);

    // Waehlt den aus KI-Sicht besten Halbzug in der aktuellen Stellung und legt
    // ihn in "out" ab. Setzt voraus, dass die KI am Zug ist. Liefert true, wenn
    // ein Zug moeglich war, und false, wenn die Partie beendet ist und es keinen
    // Zug mehr gibt. Nicht const, weil sich die KI ihren letzten Zug merkt, um
    // unter gleichwertigen Zuegen kein sinnloses Hin und Her zu spielen.
    bool chooseMove(const Game& game, Move& out);

private:
    // Minimax mit Alpha-Beta-Schnitt. Sucht bis zur Tiefe "depth" (in
    // Halbzuegen) und bewertet die Blattstellungen aus Sicht der KI-Farbe. Wer am
    // Zug ist, ergibt sich aus dem Spielzustand: ist die KI am Zug, wird
    // maximiert, sonst minimiert. Das Entfernen nach einer Muehle gehoert
    // demselben Spieler und wird so automatisch der richtigen Seite zugerechnet.
    int search(const Game& game, int depth, int alpha, int beta) const;

    Color color_;
    int searchDepth_;
    int blunderPercent_;
    Move lastChosen_;        // zuletzt gespielter Zug, fuer den Anti-Pendel-Schutz
    std::mt19937 rng_;       // Zufallsquelle fuer absichtliche Fehlzuege
};

} // namespace muehle
