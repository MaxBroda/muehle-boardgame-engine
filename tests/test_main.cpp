#include "test_framework.h"

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

// Erste Tests auf der Phasen-Ableitung des Spielers.
TEST(Player, startetInSetzphase) {
    Player p(Color::White, "Test");
    ASSERT_EQ(p.stonesInHand(), kStonesPerPlayer);
    ASSERT_TRUE(p.hasStonesInHand());
    ASSERT_TRUE(p.currentPhase() == Phase::Placing);
}

int main() {
    return ::testing::runAll();
}
