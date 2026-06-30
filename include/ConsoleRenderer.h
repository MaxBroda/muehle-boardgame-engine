#pragma once

#include <string>
#include <vector>

namespace muehle {

class Board;

// Die einzige Komponente mit Konsolen-Interaktion. Sie zeichnet das Brett,
// zeigt Menue und Meldungen und liest Eingaben. Damit bleibt die uebrige
// Spiellogik frei von Ein- und Ausgabe und gut testbar.
class ConsoleRenderer {
public:
    // Zeichnet das aktuelle Brett. Die optionalen "sidebar"-Zeilen werden rechts
    // neben das Brett gesetzt, eine Zeile je Brettzeile von oben. So stehen
    // allgemeine Angaben (Spieler am Zug, Bedenkzeit, Steinzahlen) neben dem
    // Brett statt darunter. Weil jede Brettzeile gleich breit ist, beginnt die
    // Infospalte immer an derselben Position.
    void drawBoard(const Board& board,
                   const std::vector<std::string>& sidebar = {}) const;

    // Zeigt das Hauptmenue.
    void showMainMenu() const;

    // Gibt eine Meldung an den Benutzer aus.
    void showMessage(const std::string& text) const;

    // Gibt eine hervorgehobene Meldung aus (fett und eingefaerbt), etwa fuer die
    // aktuelle Spielphase.
    void showHighlighted(const std::string& text) const;

    // Liest eine Eingabezeile von der Konsole.
    std::string promptInput() const;

    // Liefert das Gitterzeichen fuer einen Knotenpunkt anhand seiner Anschluesse
    // (oben, unten, links, rechts). Damit wird das Box-Drawing-Brett rechnerisch
    // aus der Topologie gesetzt, statt Zeichen von Hand zu platzieren. Oeffentlich
    // und statisch, damit die Auswahl unabhaengig vom Zeichnen testbar ist.
    static const char* nodeGlyph(bool up, bool down, bool left, bool right);
};

} // namespace muehle
