#include "InputParser.h"

namespace muehle {

// Die Parser-Logik (Koordinaten wie "d3", Zuege wie "a1-a4") folgt in Sprint E.

Field InputParser::parseField(const std::string& text) const {
    (void)text;
    return -1;
}

bool InputParser::parseMove(const std::string& text, Phase phase, Move& out) const {
    (void)text;
    (void)phase;
    (void)out;
    return false;
}

} // namespace muehle
