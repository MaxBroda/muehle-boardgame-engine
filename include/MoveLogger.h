#pragma once

#include <string>
#include <vector>

#include "Move.h"

namespace muehle {

class Game; // fuer Snapshot-Funktionen

// Liest und schreibt Spielprotokolle als lesbare Textdatei (eine Zeile pro
// Zug). Dieselbe Komponente wird sowohl beim Speichern waehrend des Spiels als
// auch beim Laden und bei der Wiedergabe verwendet.
class MoveLogger {
public:
    // Schreibt eine Kopfzeile mit den Spielernamen an den Anfang der Datei.
    bool writeHeader(const std::string& path,
                     const std::string& whiteName,
                     const std::string& blackName);

    // Haengt einen ausgefuehrten Zug an die Protokolldatei an.
    bool appendMove(const std::string& path, const Move& move);

    // Liest die Spielernamen aus der Kopfzeile. Liefert false, wenn die Datei
    // nicht gelesen werden kann oder die Kopfzeile unvollstaendig ist (ein Name
    // fehlt oder ist leer).
    bool readHeader(const std::string& path,
                    std::string& outWhiteName,
                    std::string& outBlackName);

    // Liest ein vollstaendiges Protokoll ein und legt die Zuege in outMoves ab.
    // Liefert false bei einem Lesefehler oder bei einer syntaktisch ungueltigen
    // Zeile. Die Zugart wird hier nur grob aus der Schreibweise abgeleitet; die
    // endgueltige Regelpruefung erfolgt beim erneuten Abspielen ueber
    // Game::replayLogged.
    bool loadGame(const std::string& path, std::vector<Move>& outMoves);

    // Speichert einen laufenden Spielstand zum spaeteren Fortsetzen.
    bool saveSnapshot(const std::string& path, const Game& game);

    // Laedt einen gespeicherten Spielstand.
    bool loadSnapshot(const std::string& path, Game& game);
};

} // namespace muehle
