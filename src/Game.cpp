#include "Game.h"

#include <utility>

namespace muehle {

Game::Game(std::string whiteName, std::string blackName)
    : board_(),
      white_(Color::White, std::move(whiteName)),
      black_(Color::Black, std::move(blackName)),
      toMove_(Color::White),  // Weiss beginnt
      pendingRemoval_(false),
      history_() {
}

const Player& Game::currentPlayer() const {
    return (toMove_ == Color::White) ? white_ : black_;
}

bool Game::validateMove(const Move& m, std::string& reason) const {
    // TODO Sprint D: vollstaendige Regelpruefung je Phase, Muehle-Schutzregel
    // und Spielende-Bedingungen. Diese Methode ist die einzige Stelle, an der
    // Regeln geprueft werden; Spiel, Laden und Wiedergabe nutzen sie gemeinsam.
    (void)m;
    reason = "Regelpruefung noch nicht implementiert (Sprint D).";
    return false;
}

bool Game::applyMove(const Move& m) {
    // TODO Sprint D: gueltigen Zug ausfuehren, Steine umbuchen, Muehle pruefen,
    // Spieler wechseln und Zug ins Journal aufnehmen.
    (void)m;
    return false;
}

bool Game::needsRemoval() const {
    return pendingRemoval_;
}

bool Game::isGameOver() const {
    // TODO Sprint D: Spielende bei zwei Steinen oder ohne gueltigen Zug.
    return false;
}

Color Game::winner() const {
    // TODO Sprint D: Gewinner bestimmen.
    return Color::None;
}

const std::vector<Move>& Game::history() const {
    return history_;
}

const Board& Game::board() const {
    return board_;
}

} // namespace muehle
