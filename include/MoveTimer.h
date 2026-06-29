#pragma once

namespace muehle {

// Sammelt die Dauer einzelner Zuege in Millisekunden und wertet sie aus. Die
// Klasse misst selbst keine Zeit: die Wanduhr (std::chrono) liegt bewusst in der
// Anwendung, damit diese Auswertung ohne echte Zeit und damit reproduzierbar
// testbar bleibt.
class MoveTimer {
public:
    // Nimmt eine gemessene Zugdauer auf. Negative Werte werden ignoriert, damit
    // eine fehlerhafte Messung die Auswertung nicht verfaelscht.
    void record(long long millis);

    // Anzahl der aufgenommenen Zuege.
    int count() const;

    // Summe aller Zugdauern in Millisekunden.
    long long total() const;

    // Durchschnittliche Zugdauer (ganzzahlig gerundet), 0 ohne Zuege.
    long long average() const;

    // Laengste einzelne Zugdauer, 0 ohne Zuege.
    long long longest() const;

private:
    int count_ = 0;
    long long total_ = 0;
    long long longest_ = 0;
};

} // namespace muehle
