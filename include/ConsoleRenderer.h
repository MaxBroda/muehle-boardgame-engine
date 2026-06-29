#pragma once

#include <string>

namespace muehle {

class Board;

// Die einzige Komponente mit Konsolen-Interaktion. Sie zeichnet das Brett,
// zeigt Menue und Meldungen und liest Eingaben. Damit bleibt die uebrige
// Spiellogik frei von Ein- und Ausgabe und gut testbar.
class ConsoleRenderer {
public:
    // Zeichnet das aktuelle Brett.
    void drawBoard(const Board& board) const;

    // Zeigt das Hauptmenue.
    void showMainMenu() const;

    // Gibt eine Meldung an den Benutzer aus.
    void showMessage(const std::string& text) const;

    // Liest eine Eingabezeile von der Konsole.
    std::string promptInput() const;

    // Liefert das Gitterzeichen fuer einen Knotenpunkt anhand seiner Anschluesse
    // (oben, unten, links, rechts). Damit wird das Box-Drawing-Brett rechnerisch
    // aus der Topologie gesetzt, statt Zeichen von Hand zu platzieren. Oeffentlich
    // und statisch, damit die Auswahl unabhaengig vom Zeichnen testbar ist.
    static const char* nodeGlyph(bool up, bool down, bool left, bool right);
};

} // namespace muehle
