#include "AiPlayer.h"

#include "Board.h"

namespace muehle {
namespace {

// Betrag fuer ein entschiedenes Spiel. Gross genug, dass Sieg und Niederlage
// jede Materialbewertung ueberlagern.
constexpr int kWinScore = 100000;

// Gewicht eines Steins (auf dem Brett oder in der Hand) im Materialvergleich.
constexpr int kMaterialWeight = 100;

// Gewicht je Stein, der in einer vollstaendigen Muehle steht. Eine ganze Muehle
// zaehlt drei Felder und damit dreimal dieses Gewicht; sie wirkt als
// Stellungsbonus, der unter dem Wert eines ganzen Steins bleibt.
constexpr int kMillWeight = 8;

} // namespace

AiPlayer::AiPlayer(Color color, int searchDepth)
    : color_(color), searchDepth_(searchDepth < 1 ? 1 : searchDepth) {
}

Color AiPlayer::color() const {
    return color_;
}

int AiPlayer::searchDepth() const {
    return searchDepth_;
}

int AiPlayer::evaluate(const Game& game, Color perspective) {
    const Color opp = opponent(perspective);

    // Ein entschiedenes Spiel zaehlt vor allem anderen.
    if (game.isGameOver()) {
        Color w = game.winner();
        if (w == perspective) {
            return kWinScore;
        }
        if (w == opp) {
            return -kWinScore;
        }
        return 0;
    }

    const Board& b = game.board();

    // Material: Steine auf dem Brett plus die noch in der Hand wartenden.
    int myMaterial = b.stoneCount(perspective) +
                     game.playerByColor(perspective).stonesInHand();
    int oppMaterial = b.stoneCount(opp) +
                      game.playerByColor(opp).stonesInHand();

    // Steine, die in einer vollstaendigen Muehle stehen (je Muehle drei Felder).
    int myMills = 0;
    int oppMills = 0;
    for (int f = 0; f < kFieldCount; ++f) {
        Color c = b.colorAt(f);
        if (c == perspective && b.formsMill(f, perspective)) {
            ++myMills;
        } else if (c == opp && b.formsMill(f, opp)) {
            ++oppMills;
        }
    }

    return kMaterialWeight * (myMaterial - oppMaterial) +
           kMillWeight * (myMills - oppMills);
}

} // namespace muehle
