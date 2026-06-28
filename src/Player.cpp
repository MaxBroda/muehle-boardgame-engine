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
    // Mit genau drei Steinen auf dem Brett darf gesprungen werden.
    if (stonesOnBoard_ <= kFlyingThreshold) {
        return Phase::Flying;
    }
    return Phase::Moving;
}

bool Player::hasStonesInHand() const {
    return stonesInHand_ > 0;
}

bool Player::canMove(const Board& board) const {
    // TODO Sprint C/D: prueft, ob mindestens ein gueltiger Zug existiert.
    (void)board;
    return true;
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
