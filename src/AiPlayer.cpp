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

// Gewicht je fast fertiger Muehle (zwei eigene Steine, drittes Feld frei). Gibt
// der Bewertung ein Gefaelle, sodass die KI auf Muehlen hinarbeitet, statt bei
// gleichem Material ziellos hin und her zu ziehen.
constexpr int kTwoWeight = 4;

// Gewicht je moeglichem Ziehschritt (eigener Stein neben einem freien Feld).
// Belohnt Beweglichkeit und das Einengen des Gegners; klein gehalten, damit es
// Material und Muehlen nicht ueberlagert.
constexpr int kMobilityWeight = 1;

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

    // Beweglichkeit: wie viele Ziehschritte auf ein benachbartes freies Feld eine
    // Farbe hat. Nutzt die oeffentliche Nachbarschaft des Bretts, dupliziert also
    // keine Topologie.
    auto mobility = [&](Color c) {
        int m = 0;
        for (int f = 0; f < kFieldCount; ++f) {
            if (b.colorAt(f) != c) {
                continue;
            }
            for (Field n : b.neighbors(f)) {
                if (b.isEmpty(n)) {
                    ++m;
                }
            }
        }
        return m;
    };

    return kMaterialWeight * (myMaterial - oppMaterial) +
           kMillWeight * (myMills - oppMills) +
           kTwoWeight * (b.twoInLineCount(perspective) - b.twoInLineCount(opp)) +
           kMobilityWeight * (mobility(perspective) - mobility(opp));
}

bool AiPlayer::chooseMove(const Game& game, Move& out) {
    std::vector<Move> moves = game.legalMoves();
    if (moves.empty()) {
        // Beendete Partie: es gibt keinen Zug zu waehlen.
        return false;
    }

    // Die Wurzel ist ein Max-Knoten: die KI ist am Zug und sucht den hoechsten
    // Wert. Jeder Kandidat wird auf einer Kopie des Spiels durchgespielt. Hier
    // wird mit vollem Fenster gesucht, damit gleichwertige Zuege verlaesslich
    // erkannt werden; das Alpha-Beta-Schneiden wirkt in den tieferen Ebenen.
    int bestScore = kNegInfinity;
    std::vector<Move> bestMoves;
    for (const Move& m : moves) {
        Game child = game;
        child.applyMove(m);
        int score = search(child, searchDepth_ - 1, kNegInfinity, kPosInfinity);
        if (score > bestScore) {
            bestScore = score;
            bestMoves.clear();
            bestMoves.push_back(m);
        } else if (score == bestScore) {
            bestMoves.push_back(m);
        }
    }

    // Anti-Pendel: unter gleichwertigen Zuegen den zuletzt gespielten Zug nicht
    // einfach umkehren (etwa d7->a7 direkt nach a7->d7), solange es eine
    // gleichwertige Alternative gibt. Das verhindert sinnloses Hin und Her.
    auto reversesLast = [&](const Move& m) {
        return lastChosen_.from != -1 && lastChosen_.to != -1 &&
               m.from == lastChosen_.to && m.to == lastChosen_.from;
    };
    Move chosen = bestMoves.front();
    if (bestMoves.size() > 1 && reversesLast(chosen)) {
        for (const Move& m : bestMoves) {
            if (!reversesLast(m)) {
                chosen = m;
                break;
            }
        }
    }

    // Nur echte Zugbewegungen (Ziehen und Springen) merken; Setzen und Entfernen
    // koennen nicht pendeln.
    if (chosen.from != -1 && chosen.to != -1) {
        lastChosen_ = chosen;
    }
    out = chosen;
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
