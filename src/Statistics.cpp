#include "Statistics.h"

#include <algorithm>

#include "Game.h"
#include "Move.h"
#include "MoveLogger.h"

namespace muehle {

bool evaluateLog(const std::string& path, GameResult& out) {
    MoveLogger logger;
    std::string whiteName;
    std::string blackName;
    if (!logger.readHeader(path, whiteName, blackName)) {
        return false;
    }
    std::vector<Move> moves;
    if (!logger.loadGame(path, moves)) {
        return false;
    }

    // Die Partie ueber denselben Pfad wie Spiel und Wiedergabe nachspielen.
    Game game(whiteName, blackName);
    std::string reason;
    for (const Move& m : moves) {
        if (!game.replayLogged(m, reason)) {
            return false;
        }
    }

    out.whiteName = whiteName;
    out.blackName = blackName;
    // Nur eine tatsaechlich beendete Partie hat einen Gewinner; ein offener
    // Zwischenstand bleibt ohne Sieger.
    out.winner = game.isGameOver() ? game.winner() : Color::None;
    return true;
}

Statistics::Entry& Statistics::entryFor(const std::string& name) {
    auto it = players_.find(name);
    if (it == players_.end()) {
        it = players_.emplace(name, Entry{name, 0, 0, 0}).first;
    }
    return it->second;
}

void Statistics::addResult(const GameResult& result) {
    Entry& white = entryFor(result.whiteName);
    Entry& black = entryFor(result.blackName);
    ++white.games;
    ++black.games;
    if (result.winner == Color::White) {
        ++white.wins;
        ++black.losses;
    } else if (result.winner == Color::Black) {
        ++black.wins;
        ++white.losses;
    }
    // Color::None: gespielte, aber unentschiedene oder offene Partie.
    ++totalGames_;
}

int Statistics::totalGames() const {
    return totalGames_;
}

std::vector<Statistics::Entry> Statistics::ranking() const {
    std::vector<Entry> result;
    result.reserve(players_.size());
    for (const auto& pair : players_) {
        result.push_back(pair.second);
    }
    std::sort(result.begin(), result.end(), [](const Entry& a, const Entry& b) {
        if (a.wins != b.wins) {
            return a.wins > b.wins;  // mehr Siege zuerst
        }
        return a.name < b.name;  // bei Gleichstand alphabetisch
    });
    return result;
}

} // namespace muehle
