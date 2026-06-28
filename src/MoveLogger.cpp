#include "MoveLogger.h"

namespace muehle {

// Die Datei-Logik wird in Sprint F implementiert. Die Methoden sind hier als
// Geruest vorhanden, damit Schnittstelle und Bibliothek bereits stehen.

bool MoveLogger::writeHeader(const std::string& path,
                             const std::string& whiteName,
                             const std::string& blackName) {
    (void)path;
    (void)whiteName;
    (void)blackName;
    return false;
}

bool MoveLogger::appendMove(const std::string& path, const Move& move) {
    (void)path;
    (void)move;
    return false;
}

bool MoveLogger::loadGame(const std::string& path, std::vector<Move>& outMoves) {
    (void)path;
    (void)outMoves;
    return false;
}

bool MoveLogger::saveSnapshot(const std::string& path, const Game& game) {
    (void)path;
    (void)game;
    return false;
}

bool MoveLogger::loadSnapshot(const std::string& path, Game& game) {
    (void)path;
    (void)game;
    return false;
}

} // namespace muehle
