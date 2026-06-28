#include "MoveLogger.h"

#include <fstream>
#include <sstream>

#include "Game.h"
#include "Types.h"

namespace muehle {

namespace {

// Schluesselwoerter der Kopfzeilen.
const char* kWhiteKey = "Weiss:";
const char* kBlackKey = "Schwarz:";

// Wandelt einen Zug in eine lesbare Protokollzeile. Beispiele:
//   "d3"            Setzzug
//   "a1-a4"         Zieh- oder Springzug
//   "a1-a4 x g7"    Zug, der eine Muehle schliesst und g7 entfernt
std::string formatMove(const Move& m) {
    std::string line;
    if (m.from == -1) {
        line = fieldName(m.to);
    } else {
        line = fieldName(m.from) + "-" + fieldName(m.to);
    }
    if (m.removed != -1) {
        line += " x " + fieldName(m.removed);
    }
    return line;
}

// Zerlegt eine Protokollzeile in einen Move. Die Zugart wird grob gesetzt
// (Place bei einem Feld, sonst Slide); die genaue Art bestimmt spaeter
// Game::replayLogged aus der Phase. Liefert false bei kaputter Syntax.
bool parseMoveLine(const std::string& line, Move& out) {
    std::istringstream stream(line);
    std::string action;
    if (!(stream >> action)) {
        return false;
    }

    out = Move{};
    std::size_t dash = action.find('-');
    if (dash == std::string::npos) {
        out.type = MoveType::Place;
        out.to = fieldIndex(action);
        if (out.to < 0) {
            return false;
        }
    } else {
        out.type = MoveType::Slide;
        out.from = fieldIndex(action.substr(0, dash));
        out.to = fieldIndex(action.substr(dash + 1));
        if (out.from < 0 || out.to < 0) {
            return false;
        }
    }

    // Optionaler Entfernen-Teil: "x <feld>".
    std::string marker;
    if (stream >> marker) {
        if (marker != "x") {
            return false;
        }
        std::string removedName;
        if (!(stream >> removedName)) {
            return false;
        }
        out.removed = fieldIndex(removedName);
        if (out.removed < 0) {
            return false;
        }
    }
    return true;
}

// Entfernt fuehrende und abschliessende Leerzeichen.
std::string trim(const std::string& text) {
    std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

} // namespace

bool MoveLogger::writeHeader(const std::string& path,
                             const std::string& whiteName,
                             const std::string& blackName) {
    std::ofstream file(path, std::ios::trunc);
    if (!file) {
        return false;
    }
    file << "# Muehle-Protokoll. Eine Zeile pro Zug: Feld zum Setzen (z.B. d3)\n";
    file << "# oder Zug von-nach (z.B. a1-a4); ein entfernter Stein folgt nach 'x'.\n";
    file << kWhiteKey << " " << whiteName << "\n";
    file << kBlackKey << " " << blackName << "\n";
    return static_cast<bool>(file);
}

bool MoveLogger::appendMove(const std::string& path, const Move& move) {
    std::ofstream file(path, std::ios::app);
    if (!file) {
        return false;
    }
    file << formatMove(move) << "\n";
    return static_cast<bool>(file);
}

bool MoveLogger::readHeader(const std::string& path,
                            std::string& outWhiteName,
                            std::string& outBlackName) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.rfind(kWhiteKey, 0) == 0) {
            outWhiteName = trim(trimmed.substr(std::string(kWhiteKey).size()));
        } else if (trimmed.rfind(kBlackKey, 0) == 0) {
            outBlackName = trim(trimmed.substr(std::string(kBlackKey).size()));
        }
    }
    return true;
}

bool MoveLogger::loadGame(const std::string& path, std::vector<Move>& outMoves) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }
    outMoves.clear();
    std::string line;
    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        // Leerzeilen, Kommentare und Kopfzeilen ueberspringen.
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }
        if (trimmed.rfind(kWhiteKey, 0) == 0 || trimmed.rfind(kBlackKey, 0) == 0) {
            continue;
        }
        Move move;
        if (!parseMoveLine(trimmed, move)) {
            return false;  // beschaedigte Zeile
        }
        outMoves.push_back(move);
    }
    return true;
}

bool MoveLogger::saveSnapshot(const std::string& path, const Game& game) {
    // Ein Spielstand ist nichts anderes als das vollstaendige Protokoll der
    // bisherigen Zuege. Fortsetzen heisst dann: Protokoll laden und erneut
    // abspielen. So gibt es nur ein Dateiformat.
    if (!writeHeader(path,
                     game.playerByColor(Color::White).name(),
                     game.playerByColor(Color::Black).name())) {
        return false;
    }
    for (const Move& m : game.history()) {
        if (!appendMove(path, m)) {
            return false;
        }
    }
    return true;
}

bool MoveLogger::loadSnapshot(const std::string& path, Game& game) {
    // Erwartet ein frisch mit den richtigen Namen angelegtes Spiel und spielt
    // die protokollierten Zuege ueber dieselbe Validierung erneut ein.
    std::vector<Move> moves;
    if (!loadGame(path, moves)) {
        return false;
    }
    std::string reason;
    for (const Move& m : moves) {
        if (!game.replayLogged(m, reason)) {
            return false;
        }
    }
    return true;
}

} // namespace muehle
