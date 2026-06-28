#pragma once

#include "Types.h"

namespace muehle {

// Ein einzelner Spielzug als reine Datenstruktur.
// Je nach Zugart sind unterschiedliche Felder gesetzt:
//   - Place: nur "to" ist gesetzt, "from" bleibt -1
//   - Slide / Jump: "from" und "to" sind gesetzt
//   - "removed" ist gesetzt, wenn der Zug eine Muehle geschlossen hat und
//     dabei ein gegnerischer Stein entfernt wird, sonst -1.
struct Move {
    MoveType type = MoveType::Place;
    Field from = -1;     // Quellfeld (bei Slide/Jump)
    Field to = -1;       // Zielfeld
    Field removed = -1;  // entfernter gegnerischer Stein, -1 = keiner
};

} // namespace muehle
