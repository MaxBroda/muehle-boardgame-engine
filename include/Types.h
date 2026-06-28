#pragma once

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

// Liefert die Gegenfarbe. None bleibt None.
inline Color opponent(Color c) {
    if (c == Color::White) return Color::Black;
    if (c == Color::Black) return Color::White;
    return Color::None;
}

} // namespace muehle
