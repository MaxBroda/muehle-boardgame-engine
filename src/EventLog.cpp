#include "EventLog.h"

#include <iomanip>

namespace muehle {

bool EventLog::open(const std::string& path) {
    // Eine Logdatei steht fuer genau eine Sitzung, daher frisch anlegen statt
    // anzuhaengen. So gibt es genau eine Kopfzeile und eine durchgehende
    // Nummerierung ab 0001, auch wenn ein bestehender Pfad angegeben wird.
    out_.open(path, std::ios::trunc);
    active_ = static_cast<bool>(out_);
    if (active_) {
        out_ << "# Muehle-Ereignislog\n";
    }
    return active_;
}

bool EventLog::isActive() const {
    return active_;
}

void EventLog::log(const std::string& message) {
    if (!active_) {
        return;
    }
    ++count_;
    // Fortlaufende, vierstellige Nummer fuer eine ruhige, ausgerichtete Spalte.
    out_ << std::setw(4) << std::setfill('0') << count_ << "  " << message
         << "\n";
    out_.flush();  // damit das Log auch bei einem Absturz vollstaendig ist
}

int EventLog::entryCount() const {
    return count_;
}

} // namespace muehle
