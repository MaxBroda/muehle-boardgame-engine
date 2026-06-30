#include "AiPlayer.h"

#include <vector>

#include "Board.h"

namespace muehle {
namespace {

// Schranken fuer die Alpha-Beta-Suche, jenseits jeder echten Bewertung.
constexpr int kNegInfinity = -1000000;
constexpr int kPosInfinity = 1000000;

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

bool AiPlayer::chooseMove(const Game& game, Move& out) const {
    std::vector<Move> moves = game.legalMoves();
    if (moves.empty()) {
        // Beendete Partie: es gibt keinen Zug zu waehlen.
        return false;
    }

    // Die Wurzel ist ein Max-Knoten: die KI ist am Zug und sucht den hoechsten
    // Wert. Jeder Kandidat wird auf einer Kopie des Spiels durchgespielt.
    int bestScore = kNegInfinity;
    Move bestMove = moves.front();
    int alpha = kNegInfinity;
    const int beta = kPosInfinity;
    for (const Move& m : moves) {
        Game child = game;
        child.applyMove(m);
        int score = search(child, searchDepth_ - 1, alpha, beta);
        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
        if (bestScore > alpha) {
            alpha = bestScore;
        }
    }
    out = bestMove;
    return true;
}

int AiPlayer::search(const Game& game, int depth, int alpha, int beta) const {
    // Blatt der Suche: Tiefe erschoepft oder Partie entschieden.
    if (depth <= 0 || game.isGameOver()) {
        return evaluate(game, color_);
    }
    std::vector<Move> moves = game.legalMoves();
    if (moves.empty()) {
        return evaluate(game, color_);
    }

    // Ist die KI am Zug, maximiert sie; ist der Gegner am Zug, minimiert er.
    const bool maximizing = (game.currentPlayer().color() == color_);
    if (maximizing) {
        int best = kNegInfinity;
        for (const Move& m : moves) {
            Game child = game;
            child.applyMove(m);
            int score = search(child, depth - 1, alpha, beta);
            if (score > best) {
                best = score;
            }
            if (best > alpha) {
                alpha = best;
            }
            if (alpha >= beta) {
                break;  // der minimierende Gegner laesst diesen Ast nicht zu
            }
        }
        return best;
    }

    int best = kPosInfinity;
    for (const Move& m : moves) {
        Game child = game;
        child.applyMove(m);
        int score = search(child, depth - 1, alpha, beta);
        if (score < best) {
            best = score;
        }
        if (best < beta) {
            beta = best;
        }
        if (alpha >= beta) {
            break;  // die maximierende KI laesst diesen Ast nicht zu
        }
    }
    return best;
}

} // namespace muehle
