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
    return playerOf(toMove_);
}

Player& Game::playerOf(Color c) {
    return (c == Color::White) ? white_ : black_;
}

const Player& Game::playerOf(Color c) const {
    return (c == Color::White) ? white_ : black_;
}

void Game::switchPlayer() {
    toMove_ = opponent(toMove_);
}

std::vector<Field> Game::removableTargets(Color owner) const {
    // Erst alle Steine des Eigentuemers ausserhalb einer Muehle sammeln.
    std::vector<Field> free;
    std::vector<Field> all;
    for (int f = 0; f < kFieldCount; ++f) {
        if (board_.colorAt(f) != owner) {
            continue;
        }
        all.push_back(f);
        if (!board_.formsMill(f, owner)) {
            free.push_back(f);
        }
    }
    // Gibt es Steine ausserhalb von Muehlen, sind nur diese entfernbar. Sind
    // alle Steine in Muehlen gebunden, hebt die Regel den Schutz auf und alle
    // sind entfernbar.
    return free.empty() ? all : free;
}

bool Game::validateMove(const Move& m, std::string& reason) const {
    if (isGameOver()) {
        reason = "Die Partie ist bereits beendet.";
        return false;
    }

    // --- Schritt zwei: ein gegnerischer Stein wird entfernt -----------------
    if (pendingRemoval_) {
        Color opp = opponent(toMove_);
        if (m.removed < 0 || m.removed >= kFieldCount) {
            reason = "Kein gueltiges Feld zum Entfernen angegeben.";
            return false;
        }
        if (board_.colorAt(m.removed) != opp) {
            reason = "Auf diesem Feld steht kein gegnerischer Stein.";
            return false;
        }
        std::vector<Field> targets = removableTargets(opp);
        for (Field t : targets) {
            if (t == m.removed) {
                return true;
            }
        }
        reason = "Der Stein ist durch eine Muehle geschuetzt.";
        return false;
    }

    // --- Schritt eins: der eigentliche Zug (Setzen, Ziehen, Springen) -------
    if (m.removed != -1) {
        reason = "Erst den Zug ausfuehren, das Entfernen folgt danach.";
        return false;
    }

    const Player& p = currentPlayer();
    Phase phase = p.currentPhase();

    if (m.to < 0 || m.to >= kFieldCount) {
        reason = "Das Zielfeld ist kein gueltiges Feld.";
        return false;
    }
    if (!board_.isEmpty(m.to)) {
        reason = "Das Zielfeld ist bereits belegt.";
        return false;
    }

    if (phase == Phase::Placing) {
        if (m.type != MoveType::Place) {
            reason = "In der Setzphase wird ein Stein aus der Hand gesetzt.";
            return false;
        }
        if (m.from != -1) {
            reason = "Beim Setzen gibt es kein Quellfeld.";
            return false;
        }
        return true;
    }

    // Zieh- und Springphase teilen sich die Pruefung von Quellfeld und Besitz.
    if (m.from < 0 || m.from >= kFieldCount) {
        reason = "Das Quellfeld ist kein gueltiges Feld.";
        return false;
    }
    if (board_.colorAt(m.from) != toMove_) {
        reason = "Auf dem Quellfeld steht kein eigener Stein.";
        return false;
    }

    if (phase == Phase::Moving) {
        if (m.type != MoveType::Slide) {
            reason = "In der Ziehphase wird auf ein Nachbarfeld gezogen.";
            return false;
        }
        if (!board_.areAdjacent(m.from, m.to)) {
            reason = "Das Zielfeld ist nicht benachbart.";
            return false;
        }
        return true;
    }

    // Phase::Flying
    if (m.type != MoveType::Jump) {
        reason = "In der Springphase wird auf ein beliebiges freies Feld gesprungen.";
        return false;
    }
    return true;
}

bool Game::applyMove(const Move& m) {
    std::string reason;
    if (!validateMove(m, reason)) {
        return false;
    }

    // --- Schritt zwei: Entfernen aufloesen ----------------------------------
    if (pendingRemoval_) {
        Color opp = opponent(toMove_);
        board_.removeStone(m.removed);
        playerOf(opp).removeFromBoard();
        // Das Entfernen gehoert zum Zug, der die Muehle geschlossen hat, und
        // wird in dessen Journal-Eintrag vermerkt. So bleibt es eine Zeile pro
        // Zug, auch beim spaeteren Speichern.
        if (!history_.empty()) {
            history_.back().removed = m.removed;
        }
        pendingRemoval_ = false;
        switchPlayer();
        return true;
    }

    // --- Schritt eins: Zug ausfuehren ---------------------------------------
    Player& p = playerOf(toMove_);
    if (m.type == MoveType::Place) {
        board_.placeStone(m.to, toMove_);
        p.removeFromHand();
        p.addToBoard();
    } else {
        // Ziehen oder Springen: der Stein wechselt das Feld, die Steinzahl
        // bleibt gleich.
        board_.moveStone(m.from, m.to);
    }
    history_.push_back(m);

    // Hat der Zug eine Muehle geschlossen und gibt es ueberhaupt einen
    // entfernbaren gegnerischen Stein, bleibt der Spieler am Zug und entfernt
    // im naechsten Schritt. Andernfalls ist der Gegner an der Reihe.
    if (board_.formsMill(m.to, toMove_) &&
        !removableTargets(opponent(toMove_)).empty()) {
        pendingRemoval_ = true;
    } else {
        switchPlayer();
    }
    return true;
}

bool Game::needsRemoval() const {
    return pendingRemoval_;
}

std::vector<Field> Game::removableStones() const {
    if (!pendingRemoval_) {
        return {};
    }
    return removableTargets(opponent(toMove_));
}

bool Game::isGameOver() const {
    // Mitten in einem Zug (Muehle geschlossen, Entfernen offen) endet nichts.
    if (pendingRemoval_) {
        return false;
    }
    const Player& p = currentPlayer();
    // Wer keine Steine mehr in der Hand hat und auf unter drei Steine faellt,
    // hat verloren. In der Setzphase greift das nicht.
    if (!p.hasStonesInHand() && p.stonesOnBoard() < kFlyingThreshold) {
        return true;
    }
    // Wer am Zug ist, aber keinen gueltigen Zug mehr hat, hat ebenfalls verloren.
    if (!p.canMove(board_)) {
        return true;
    }
    return false;
}

Color Game::winner() const {
    if (!isGameOver()) {
        return Color::None;
    }
    // Verloren hat der Spieler, der am Zug ist (zu wenige Steine oder kein Zug).
    return opponent(toMove_);
}

const std::vector<Move>& Game::history() const {
    return history_;
}

const Board& Game::board() const {
    return board_;
}

} // namespace muehle
