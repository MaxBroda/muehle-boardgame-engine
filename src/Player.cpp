#include "Player.h"

#include <utility>

#include "Board.h"

namespace muehle {

Player::Player(Color color, std::string name)
    : color_(color),
      name_(std::move(name)),
      stonesInHand_(kStonesPerPlayer),
      stonesOnBoard_(0) {
}

Color Player::color() const {
    return color_;
}

const std::string& Player::name() const {
    return name_;
}

int Player::stonesInHand() const {
    return stonesInHand_;
}

int Player::stonesOnBoard() const {
    return stonesOnBoard_;
}

Phase Player::currentPhase() const {
    // Solange Steine in der Hand sind, wird gesetzt.
    if (stonesInHand_ > 0) {
        return Phase::Placing;
    }
    // Mit genau drei Steinen auf dem Brett darf gesprungen werden. Weniger als
    // drei kann hier nicht auftreten: bei zwei Steinen ist die Partie vorbei
    // (siehe Game::isGameOver), bevor erneut nach der Phase gefragt wird.
    if (stonesOnBoard_ == kFlyingThreshold) {
        return Phase::Flying;
    }
    return Phase::Moving;
}

bool Player::hasStonesInHand() const {
    return stonesInHand_ > 0;
}

bool Player::canMove(const Board& board) const {
    // In der Setzphase wird ein Stein aus der Hand auf irgendein freies Feld
    // gesetzt. Es gibt also einen Zug, solange ueberhaupt ein Feld frei ist.
    if (hasStonesInHand()) {
        return !board.emptyFields().empty();
    }
    // In der Springphase darf der Spieler auf jedes freie Feld springen, ein
    // einziges freies Feld genuegt damit ebenfalls.
    if (currentPhase() == Phase::Flying) {
        return !board.emptyFields().empty();
    }
    // In der Ziehphase braucht mindestens einer der eigenen Steine ein freies
    // Nachbarfeld.
    for (int f = 0; f < kFieldCount; ++f) {
        if (board.colorAt(f) != color_) {
            continue;
        }
        for (Field n : board.neighbors(f)) {
            if (board.isEmpty(n)) {
                return true;
            }
        }
    }
    return false;
}

void Player::removeFromHand() {
    if (stonesInHand_ > 0) {
        --stonesInHand_;
    }
}

void Player::addToBoard() {
    ++stonesOnBoard_;
}

void Player::removeFromBoard() {
    if (stonesOnBoard_ > 0) {
        --stonesOnBoard_;
    }
}

} // namespace muehle
