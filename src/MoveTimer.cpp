#include "MoveTimer.h"

namespace muehle {

void MoveTimer::record(long long millis) {
    if (millis < 0) {
        return;  // unplausible Messung verwerfen
    }
    ++count_;
    total_ += millis;
    if (millis > longest_) {
        longest_ = millis;
    }
}

int MoveTimer::count() const {
    return count_;
}

long long MoveTimer::total() const {
    return total_;
}

long long MoveTimer::average() const {
    if (count_ == 0) {
        return 0;
    }
    return total_ / count_;
}

long long MoveTimer::longest() const {
    return longest_;
}

std::string MoveTimer::formatSeconds(long long millis) {
    if (millis < 0) {
        millis = 0;
    }
    // Auf Hundertstelsekunden kaufmaennisch runden, dann ganze Sekunden und
    // Nachkommastellen getrennt aufbauen. So bleibt die Ausgabe exakt zwei-
    // stellig hinter dem Punkt, ohne Gleitkomma-Formatierung.
    long long hundredths = (millis + 5) / 10;
    long long whole = hundredths / 100;
    long long frac = hundredths % 100;
    std::string fracText = (frac < 10 ? "0" : "") + std::to_string(frac);
    return std::to_string(whole) + "." + fracText + " s";
}

} // namespace muehle
