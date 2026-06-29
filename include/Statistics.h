#pragma once

#include <map>
#include <string>
#include <vector>

#include "Types.h"

namespace muehle {

// Ergebnis einer einzelnen Partie. winner ist Color::None, wenn die Partie noch
// offen war (ein gespeicherter Zwischenstand) oder unentschieden endete.
struct GameResult {
    std::string whiteName;
    std::string blackName;
    Color winner = Color::None;
};

// Wertet eine Protokolldatei aus, indem sie ueber dieselbe zentrale Validierung
// erneut abgespielt wird, und legt Namen und Gewinner in out ab. Liefert false,
// wenn die Datei nicht lesbar oder das Protokoll beschaedigt ist. So nutzt auch
// die Statistik genau den Regelpfad des Spiels.
bool evaluateLog(const std::string& path, GameResult& out);

// Sammelt die Ergebnisse mehrerer Partien und wertet sie spieleruebergreifend
// aus. Bewusst frei vom Dateisystem gehalten: das Einlesen erledigt evaluateLog,
// hier wird nur gezaehlt, damit die Auswertung leicht testbar bleibt.
class Statistics {
public:
    // Auswertung je Spielername.
    struct Entry {
        std::string name;
        int games = 0;   // Teilnahmen insgesamt
        int wins = 0;    // gewonnene Partien
        int losses = 0;  // verlorene Partien
    };

    // Nimmt das Ergebnis einer Partie auf.
    void addResult(const GameResult& result);

    // Anzahl insgesamt aufgenommener Partien.
    int totalGames() const;

    // Auswertung je Spieler, absteigend nach Siegen sortiert, bei Gleichstand
    // alphabetisch nach Name.
    std::vector<Entry> ranking() const;

private:
    Entry& entryFor(const std::string& name);

    std::map<std::string, Entry> players_;
    int totalGames_ = 0;
};

} // namespace muehle
