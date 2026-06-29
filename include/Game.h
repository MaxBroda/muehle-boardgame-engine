#pragma once

#include <string>
#include <vector>

#include "Board.h"
#include "Move.h"
#include "Player.h"
#include "Types.h"

namespace muehle {

// Die zentrale Spielklasse. Sie orchestriert den Ablauf, haelt Brett und beide
// Spieler und enthaelt die einzige Regelpruefung des Projekts. Sowohl das
// laufende Spiel als auch das Laden und die Wiedergabe rufen genau diese
// Pruefung auf, damit Regeln nur an einer Stelle stehen.
class Game {
public:
    Game(std::string whiteName, std::string blackName);

    // Der Spieler, der gerade am Zug ist.
    const Player& currentPlayer() const;

    // Lesezugriff auf einen Spieler ueber seine Farbe (z.B. fuer Namen).
    const Player& playerByColor(Color c) const;

    // Prueft einen Zug gegen Brett- und Spielerzustand. Liefert true, wenn der
    // Zug gueltig ist. Bei false steht in "reason" eine Klartextbegruendung.
    // Dies ist die einzige Stelle, an der Regeln geprueft werden; Spiel, Laden
    // und Wiedergabe nutzen sie gemeinsam.
    bool validateMove(const Move& m, std::string& reason) const;

    // Fuehrt einen Zug aus. Setzt voraus, dass er gueltig ist. Liefert true bei
    // Erfolg.
    bool applyMove(const Move& m);

    // Spielt einen protokollierten Komplettzug (Aktion und optional ein
    // entfernter Stein in einem Datensatz) ueber dieselbe Validierung erneut
    // ab. Die Zugart wird aus der aktuellen Phase rekonstruiert. Liefert false
    // mit Begruendung, wenn der Datensatz nicht zum Spielzustand passt; so
    // erkennt das Laden beschaedigte Protokolle.
    bool replayLogged(const Move& logged, std::string& reason);

    // Nimmt den letzten vollstaendigen Zug zurueck. Das Spiel wird dazu aus dem
    // Zugjournal ohne den letzten Eintrag neu aufgebaut, ueber dieselbe zentrale
    // Validierung wie beim Laden. So gibt es keinen zweiten Regelpfad und keinen
    // von Hand gepflegten Rueckwaerts-Code. Liefert false, wenn es keinen Zug
    // gibt oder gerade ein Entfernen aussteht (dann ist der Zug noch nicht
    // abgeschlossen und kann nicht als Ganzes zurueckgenommen werden).
    bool undoLastMove();

    // Wurde mit dem letzten Zug eine Muehle geschlossen, sodass jetzt ein
    // gegnerischer Stein zu entfernen ist? Solange das gilt, bleibt derselbe
    // Spieler am Zug und muss als naechstes ein Entfernen einreichen.
    bool needsRemoval() const;

    // Die gegnerischen Steine, die aktuell legal entfernt werden duerfen
    // (Schutzregel beruecksichtigt). Nur waehrend needsRemoval() gefuellt,
    // sonst leer. Dient der Eingabeaufforderung und dem Hinweis-Modus.
    std::vector<Field> removableStones() const;

    // Ist die Partie beendet?
    bool isGameOver() const;

    // Gewinner der Partie (Color::None, solange sie laeuft).
    Color winner() const;

    // Das vollstaendige Zugjournal.
    const std::vector<Move>& history() const;

    // Lesezugriff auf das Brett (z.B. zum Zeichnen).
    const Board& board() const;

private:
    // Schreibzugriff auf einen Spieler ueber seine Farbe.
    Player& playerOf(Color c);
    const Player& playerOf(Color c) const;

    // Die gegnerischen Steine des Eigentuemers "owner", die nach der
    // Schutzregel entfernt werden duerfen: alle Steine ausserhalb einer Muehle;
    // sind alle Steine in Muehlen, sind ausnahmsweise alle entfernbar.
    std::vector<Field> removableTargets(Color owner) const;

    // Wechselt den Spieler am Zug.
    void switchPlayer();

    Board board_;
    Player white_;
    Player black_;
    Color toMove_;          // wer ist am Zug
    bool pendingRemoval_;    // wartet das Spiel auf das Entfernen eines Steins?
    std::vector<Move> history_;
};

} // namespace muehle
