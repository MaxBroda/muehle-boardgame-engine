#pragma once

#include <fstream>
#include <string>

namespace muehle {

// Schreibt eine erweiterte, fortlaufend nummerierte Protokollierung in eine
// Datei, wenn sie ueber das Kommandozeilen-Flag aktiviert wurde. Anders als das
// Spielprotokoll (eine Zeile pro Zug, wieder abspielbar) dient dieses Log nur
// der Beobachtung: Zeitpunkte, Bedenkzeiten und Hinweise. Solange das Log nicht
// aktiv ist, tun alle Aufrufe nichts.
class EventLog {
public:
    // Aktiviert das Log und legt die Datei frisch an (eine Datei je Sitzung).
    // Liefert false, wenn die Datei nicht geoeffnet werden kann; das Log bleibt
    // dann inaktiv.
    bool open(const std::string& path);

    // Ist das Log aktiv?
    bool isActive() const;

    // Schreibt eine nummerierte Zeile. Ohne aktives Log passiert nichts.
    void log(const std::string& message);

    // Anzahl der bisher geschriebenen Zeilen.
    int entryCount() const;

private:
    bool active_ = false;
    int count_ = 0;
    std::ofstream out_;
};

} // namespace muehle
