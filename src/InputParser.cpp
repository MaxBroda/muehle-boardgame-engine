#include "InputParser.h"

#include <cctype>

namespace muehle {

namespace {

// Entfernt fuehrende und abschliessende Leerzeichen und macht alles klein.
// So sind "  D3 ", "d3" und "D3" gleichwertig.
std::string normalize(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin]))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }
    std::string result = text.substr(begin, end - begin);
    for (char& ch : result) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return result;
}

} // namespace

Field InputParser::parseField(const std::string& text) const {
    // Normalisieren (klein, ohne Leerzeichen) und gegen die Feldtabelle pruefen.
    return fieldIndex(normalize(text));
}

bool InputParser::parseMove(const std::string& text, Phase phase, Move& out) const {
    std::string token = normalize(text);

    if (phase == Phase::Placing) {
        // In der Setzphase ist die Eingabe ein einzelnes Feld, z.B. "d3".
        Field to = parseField(token);
        if (to < 0) {
            return false;
        }
        out = Move{};
        out.type = MoveType::Place;
        out.to = to;
        return true;
    }

    // Zieh- und Springphase: zwei Felder, getrennt durch einen Bindestrich,
    // z.B. "a1-a4".
    std::size_t dash = token.find('-');
    if (dash == std::string::npos) {
        return false;
    }
    Field from = parseField(token.substr(0, dash));
    Field to = parseField(token.substr(dash + 1));
    if (from < 0 || to < 0) {
        return false;
    }
    out = Move{};
    out.type = (phase == Phase::Moving) ? MoveType::Slide : MoveType::Jump;
    out.from = from;
    out.to = to;
    return true;
}

} // namespace muehle
