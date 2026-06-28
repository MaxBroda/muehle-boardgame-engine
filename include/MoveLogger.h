#pragma once

#include <string>
#include <vector>

#include "Move.h"

namespace muehle {

class Game; // fuer Snapshot-Funktionen

// Liest und schreibt Spielprotokolle als lesbare Textdatei (eine Zeile pro
// Zug). Dieselbe Komponente wird sowohl beim Speichern waehrend des Spiels als
// auch beim Laden und bei der Wiedergabe verwendet. Die eigentliche
// Datei-Logik folgt in Sprint F.
class MoveLogger {
public:
    // Schreibt eine Kopfzeile mit den Spielernamen an den Anfang der Datei.
    bool writeHeader(const std::string& path,
                     const std::string& whiteName,
                     const std::string& blackName);

    // Haengt einen ausgefuehrten Zug an die Protokolldatei an.
    bool appendMove(const std::string& path, const Move& move);

    // Liest ein vollstaendiges Protokoll ein und legt die Zuege in outMoves ab.
    // Liefert false, wenn die Datei nicht gelesen werden kann.
    bool loadGame(const std::string& path, std::vector<Move>& outMoves);

    // Speichert einen laufenden Spielstand zum spaeteren Fortsetzen.
    bool saveSnapshot(const std::string& path, const Game& game);

    // Laedt einen gespeicherten Spielstand.
    bool loadSnapshot(const std::string& path, Game& game);
};

} // namespace muehle
