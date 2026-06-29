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

} // namespace muehle
