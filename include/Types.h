#pragma once

#include <array>

// Grundlegende Typen und Konstanten, die im ganzen Projekt verwendet werden.
// Bewusst klein gehalten: ein paar Aufzaehlungen und Konstanten, mehr braucht
// die Engine an gemeinsamen Bausteinen nicht.

namespace muehle {

// Belegung eines Feldes bzw. Farbe eines Spielers.
// None bedeutet: das Feld ist leer.
enum class Color {
    None,
    White,
    Black
};

// Ein Feld wird ueber seinen Index 0..23 angesprochen.
// -1 steht fuer "kein Feld" (z.B. unbenutztes Quellfeld bei einem Setzzug).
using Field = int;

// Das Muehlebrett hat genau 24 Felder.
constexpr int kFieldCount = 24;

// Jeder Spieler startet mit neun Steinen.
constexpr int kStonesPerPlayer = 9;

// Bei dieser Steinzahl darf ein Spieler springen (Springphase).
constexpr int kFlyingThreshold = 3;

// Spielphase eines Spielers. Sie ergibt sich aus seinen Steinzahlen,
// wird also abgeleitet und nicht separat gespeichert.
enum class Phase {
    Placing,  // Setzphase: Steine aus der Hand aufs Brett
    Moving,   // Ziehphase: Stein auf ein benachbartes freies Feld
    Flying    // Springphase: Stein auf ein beliebiges freies Feld
};

// Art eines Spielzugs.
enum class MoveType {
    Place,  // Stein aus der Hand setzen
    Slide,  // Stein auf ein benachbartes Feld ziehen
    Jump    // Stein auf ein beliebiges freies Feld springen
};

// Koordinaten-Name jedes Feldes in der Standard-Muehle-Notation (Dateien a..g,
// Reihen 1..7). Der Index 0..23 ist gelesen von oben (Reihe 7) nach unten
// (Reihe 1) und in jeder Reihe von links nach rechts vergeben. Diese Zuordnung
// ist die einzige Wahrheit fuer Adjazenz-Tabelle, Muehlen-Linien, den Parser
// (Eingabe wie "d3") und den Renderer (Beschriftung). Wer sie aendert, muss
// nur diese Tabelle und die Topologie in Board anfassen.
inline constexpr std::array<const char*, kFieldCount> kFieldNames = {
    "a7", "d7", "g7",              // Reihe 7: aeusseres Quadrat oben
    "b6", "d6", "f6",              // Reihe 6: mittleres Quadrat oben
    "c5", "d5", "e5",              // Reihe 5: inneres Quadrat oben
    "a4", "b4", "c4", "e4", "f4", "g4",  // Reihe 4: linke und rechte Speiche
    "c3", "d3", "e3",              // Reihe 3: inneres Quadrat unten
    "b2", "d2", "f2",              // Reihe 2: mittleres Quadrat unten
    "a1", "d1", "g1"               // Reihe 1: aeusseres Quadrat unten
};

// Liefert die Gegenfarbe. None bleibt None.
inline Color opponent(Color c) {
    if (c == Color::White) return Color::Black;
    if (c == Color::Black) return Color::White;
    return Color::None;
}

} // namespace muehle
