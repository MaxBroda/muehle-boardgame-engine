#include "test_framework.h"

#include <string>

#include "Board.h"
#include "Player.h"

using namespace muehle;

// Beweist, dass das Test-Framework selbst funktioniert.
TEST(Smoke, frameworkLaeuft) {
    ASSERT_TRUE(1 + 1 == 2);
    ASSERT_FALSE(2 + 2 == 5);
    ASSERT_EQ(3 * 3, 9);
}

// Erste echte Tests auf der bereits umgesetzten Brett-Grundlogik.
TEST(Board, istAnfangsLeer) {
    Board board;
    ASSERT_EQ(board.stoneCount(Color::White), 0);
    ASSERT_EQ(board.stoneCount(Color::Black), 0);
    ASSERT_EQ(static_cast<int>(board.emptyFields().size()), kFieldCount);
    ASSERT_TRUE(board.isEmpty(0));
}

TEST(Board, setztUndEntferntStein) {
    Board board;
    board.placeStone(5, Color::White);
    ASSERT_FALSE(board.isEmpty(5));
    ASSERT_EQ(board.colorAt(5), Color::White);
    ASSERT_EQ(board.stoneCount(Color::White), 1);

    board.removeStone(5);
    ASSERT_TRUE(board.isEmpty(5));
    ASSERT_EQ(board.stoneCount(Color::White), 0);
}

// --- Sprint B: Brett-Topologie ----------------------------------------------

// Hilfsfunktion: Index eines Feldes ueber seinen Koordinaten-Namen finden.
// Macht die Tests lesbar (areAdjacent(idx("a1"), idx("a4")) statt roher Zahlen).
static Field idx(const std::string& name) {
    for (int i = 0; i < kFieldCount; ++i) {
        if (name == kFieldNames[static_cast<std::size_t>(i)]) {
            return i;
        }
    }
    return -1;
}

TEST(Board, benachbarteFelderWerdenErkannt) {
    Board board;
    // Kante des aeusseren Quadrats und eine Speiche.
    ASSERT_TRUE(board.areAdjacent(idx("a1"), idx("a4")));
    ASSERT_TRUE(board.areAdjacent(idx("a1"), idx("d1")));
    // Kreuzungspunkt d2 hat vier Nachbarn.
    ASSERT_TRUE(board.areAdjacent(idx("d2"), idx("d1")));
    ASSERT_TRUE(board.areAdjacent(idx("d2"), idx("d3")));
    ASSERT_TRUE(board.areAdjacent(idx("d2"), idx("b2")));
    ASSERT_TRUE(board.areAdjacent(idx("d2"), idx("f2")));
}

TEST(Board, nichtBenachbarteFelderWerdenAbgelehnt) {
    Board board;
    // Die Ecken eines Quadrats sind nie direkt verbunden.
    ASSERT_FALSE(board.areAdjacent(idx("a1"), idx("g1")));
    ASSERT_FALSE(board.areAdjacent(idx("a7"), idx("g7")));
    // Ueber die Mittellinie springen die Quadrate nicht ineinander.
    ASSERT_FALSE(board.areAdjacent(idx("d7"), idx("d5")));
    // Ein Feld ist nicht zu sich selbst benachbart.
    ASSERT_FALSE(board.areAdjacent(idx("d2"), idx("d2")));
    // Ungueltige Indizes.
    ASSERT_FALSE(board.areAdjacent(-1, 0));
    ASSERT_FALSE(board.areAdjacent(0, kFieldCount));
}

TEST(Board, adjazenzIstSymmetrisch) {
    Board board;
    // Wenn a Nachbar von b ist, muss auch b Nachbar von a sein. Zugleich die
    // Gesamtzahl der Kanten zaehlen: 32 Verbindungen, also 64 gerichtete Paare.
    int directed = 0;
    for (int a = 0; a < kFieldCount; ++a) {
        for (Field b : board.neighbors(a)) {
            ASSERT_TRUE(board.areAdjacent(b, a));
            ++directed;
        }
    }
    ASSERT_EQ(directed, 64);
}

TEST(Board, erkenntGeschlosseneMuehle) {
    Board board;
    // Reihe 1 mit Weiss vollstaendig besetzen: a1 d1 g1.
    board.placeStone(idx("a1"), Color::White);
    board.placeStone(idx("d1"), Color::White);
    board.placeStone(idx("g1"), Color::White);
    ASSERT_TRUE(board.formsMill(idx("a1"), Color::White));
    ASSERT_TRUE(board.formsMill(idx("d1"), Color::White));
    ASSERT_TRUE(board.formsMill(idx("g1"), Color::White));
}

TEST(Board, unvollstaendigeOderGemischteLinieIstKeineMuehle) {
    Board board;
    // Nur zwei der drei Felder besetzt.
    board.placeStone(idx("a1"), Color::White);
    board.placeStone(idx("d1"), Color::White);
    ASSERT_FALSE(board.formsMill(idx("a1"), Color::White));
    // Drittes Feld in der Gegenfarbe: keine Muehle fuer Weiss, keine fuer Schwarz.
    board.placeStone(idx("g1"), Color::Black);
    ASSERT_FALSE(board.formsMill(idx("a1"), Color::White));
    ASSERT_FALSE(board.formsMill(idx("g1"), Color::Black));
    // Leere Farbe bildet nie eine Muehle.
    ASSERT_FALSE(board.formsMill(idx("a1"), Color::None));
}

TEST(Board, senkrechteMuehleZaehltUnabhaengigVonWaagerechter) {
    Board board;
    // Datei a komplett: a7 a4 a1. Pruefen, dass die senkrechte Linie greift.
    board.placeStone(idx("a7"), Color::Black);
    board.placeStone(idx("a4"), Color::Black);
    board.placeStone(idx("a1"), Color::Black);
    ASSERT_TRUE(board.formsMill(idx("a4"), Color::Black));
    // a4 ist Teil keiner waagerechten Dreierlinie mit nur einem Feldnamen,
    // die Erkennung darf also nicht von der Reihe abhaengen.
    ASSERT_FALSE(board.formsMill(idx("a4"), Color::White));
}

// Erste Tests auf der Phasen-Ableitung des Spielers.
TEST(Player, startetInSetzphase) {
    Player p(Color::White, "Test");
    ASSERT_EQ(p.stonesInHand(), kStonesPerPlayer);
    ASSERT_TRUE(p.hasStonesInHand());
    ASSERT_TRUE(p.currentPhase() == Phase::Placing);
}

// --- Sprint C: Player-Phasen und canMove ------------------------------------

// Hilfsfunktion: liefert einen Spieler, der die Hand geleert hat und genau
// "onBoard" Steine auf dem Brett fuehrt. So lassen sich Zieh- und Springphase
// gezielt herstellen.
static Player makePlacedPlayer(Color c, int onBoard) {
    Player p(c, "X");
    for (int i = 0; i < kStonesPerPlayer; ++i) {
        p.removeFromHand();
    }
    for (int i = 0; i < onBoard; ++i) {
        p.addToBoard();
    }
    return p;
}

TEST(Player, phaseWechseltMitSteinzahl) {
    // Frisch: Hand voll, also Setzphase.
    Player p(Color::White, "X");
    ASSERT_TRUE(p.currentPhase() == Phase::Placing);

    // Hand leer mit vier Steinen auf dem Brett: Ziehphase.
    Player moving = makePlacedPlayer(Color::White, 4);
    ASSERT_FALSE(moving.hasStonesInHand());
    ASSERT_TRUE(moving.currentPhase() == Phase::Moving);

    // Genau drei Steine: Springphase.
    Player flying = makePlacedPlayer(Color::White, kFlyingThreshold);
    ASSERT_TRUE(flying.currentPhase() == Phase::Flying);
}

TEST(Player, kannInSetzphaseImmerZiehenSolangeEinFeldFreiIst) {
    Board board;
    Player p(Color::White, "X");
    ASSERT_TRUE(p.canMove(board));
}

TEST(Player, eingeschlossenerSteinKannInZiehphaseNichtZiehen) {
    // Vier weisse Steine auf den vier Aussenecken, alle Nachbarfelder durch
    // Schwarz blockiert. Damit hat kein weisser Stein ein freies Nachbarfeld.
    Board board;
    board.placeStone(idx("a7"), Color::White);
    board.placeStone(idx("g7"), Color::White);
    board.placeStone(idx("a1"), Color::White);
    board.placeStone(idx("g1"), Color::White);
    board.placeStone(idx("d7"), Color::Black);
    board.placeStone(idx("a4"), Color::Black);
    board.placeStone(idx("g4"), Color::Black);
    board.placeStone(idx("d1"), Color::Black);

    Player white = makePlacedPlayer(Color::White, 4);
    ASSERT_TRUE(white.currentPhase() == Phase::Moving);
    ASSERT_FALSE(white.canMove(board));

    // Ein Nachbarfeld frei raeumen: jetzt ist ein Zug moeglich.
    board.removeStone(idx("d7"));
    ASSERT_TRUE(white.canMove(board));
}

TEST(Player, springphaseBrauchtNurEinFreiesFeld) {
    // Drei Steine, Brett fast leer: in der Springphase immer ein Zug moeglich,
    // auch wenn kein Stein ein freies direktes Nachbarfeld haette.
    Board board;
    board.placeStone(idx("a7"), Color::White);
    board.placeStone(idx("g7"), Color::White);
    board.placeStone(idx("g1"), Color::White);
    Player flying = makePlacedPlayer(Color::White, kFlyingThreshold);
    ASSERT_TRUE(flying.currentPhase() == Phase::Flying);
    ASSERT_TRUE(flying.canMove(board));
}

int main() {
    return ::testing::runAll();
}
