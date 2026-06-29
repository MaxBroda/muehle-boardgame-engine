#include "test_framework.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "Board.h"
#include "CommandLine.h"
#include "ConsoleRenderer.h"
#include "EventLog.h"
#include "Game.h"
#include "InputParser.h"
#include "Move.h"
#include "MoveLogger.h"
#include "MoveTimer.h"
#include "Player.h"
#include "Statistics.h"

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

// --- Sprint D: Game-Logik ---------------------------------------------------

// Setzt einen Stein und besteht darauf, dass der Zug gueltig war.
static void place(Game& g, const std::string& field) {
    Move m;
    m.type = MoveType::Place;
    m.to = idx(field);
    std::string reason;
    ASSERT_TRUE(g.validateMove(m, reason));
    ASSERT_TRUE(g.applyMove(m));
}

// Entfernt einen gegnerischen Stein und besteht auf Gueltigkeit.
static void removeStone(Game& g, const std::string& field) {
    Move m;
    m.removed = idx(field);
    std::string reason;
    ASSERT_TRUE(g.validateMove(m, reason));
    ASSERT_TRUE(g.applyMove(m));
}

TEST(Game, gueltigerSetzzugWechseltSpieler) {
    Game game("Weiss", "Schwarz");
    ASSERT_EQ(game.currentPlayer().color(), Color::White);
    place(game, "a1");
    // Nach dem Setzen ist Schwarz am Zug, kein Entfernen offen.
    ASSERT_FALSE(game.needsRemoval());
    ASSERT_EQ(game.currentPlayer().color(), Color::Black);
    ASSERT_EQ(static_cast<int>(game.history().size()), 1);
}

TEST(Game, ungueltigeSetzzuegeWerdenAbgelehnt) {
    Game game("Weiss", "Schwarz");
    std::string reason;
    // Besetztes Feld.
    place(game, "a1");        // Weiss auf a1
    Move onOccupied;
    onOccupied.type = MoveType::Place;
    onOccupied.to = idx("a1");
    ASSERT_FALSE(game.validateMove(onOccupied, reason)); // Schwarz auf a1
    // Falsches Zielfeld.
    Move bad;
    bad.type = MoveType::Place;
    bad.to = -1;
    ASSERT_FALSE(game.validateMove(bad, reason));
    // In der Setzphase darf kein Quellfeld gesetzt sein.
    Move withFrom;
    withFrom.type = MoveType::Place;
    withFrom.from = idx("d1");
    withFrom.to = idx("d2");
    ASSERT_FALSE(game.validateMove(withFrom, reason));
    // Entfernen ohne geschlossene Muehle ist nicht erlaubt.
    Move earlyRemove;
    earlyRemove.type = MoveType::Place;
    earlyRemove.to = idx("g1");
    earlyRemove.removed = idx("a1");
    ASSERT_FALSE(game.validateMove(earlyRemove, reason));
}

TEST(Game, geschlosseneMuehleVerlangtEntfernen) {
    Game game("Weiss", "Schwarz");
    // Weiss baut die Reihe a1-d1-g1, Schwarz setzt daneben.
    place(game, "a1");  // W
    place(game, "a7");  // B
    place(game, "d1");  // W
    place(game, "a4");  // B
    place(game, "g1");  // W schliesst Muehle a1-d1-g1
    // Jetzt muss Weiss einen Stein entfernen und bleibt am Zug.
    ASSERT_TRUE(game.needsRemoval());
    ASSERT_EQ(game.currentPlayer().color(), Color::White);
    // Schwarz hat keine Muehle, also sind beide Steine entfernbar.
    ASSERT_EQ(static_cast<int>(game.removableStones().size()), 2);
    removeStone(game, "a7");
    // Nach dem Entfernen ist Schwarz am Zug, der Stein ist weg.
    ASSERT_FALSE(game.needsRemoval());
    ASSERT_EQ(game.currentPlayer().color(), Color::Black);
    ASSERT_EQ(game.board().colorAt(idx("a7")), Color::None);
    // Das Entfernen wurde im selben Journal-Eintrag vermerkt (eine Zeile pro Zug).
    ASSERT_EQ(game.history().back().removed, idx("a7"));
}

TEST(Game, schutzregelSchuetztSteineInMuehle) {
    // Szenario von Hand gefuehrt, damit am Ende gilt: Schwarz hat eine
    // vollstaendige Muehle (geschuetzt) und zwei Steine ausserhalb (entfernbar),
    // waehrend Weiss frisch eine Muehle schliesst.
    Game game("Weiss", "Schwarz");
    place(game, "a4");  // W
    place(game, "a7");  // B
    place(game, "b4");  // W
    place(game, "d7");  // B
    place(game, "c4");  // W schliesst Muehle a4-b4-c4
    removeStone(game, "a7");  // Weiss entfernt einen freien schwarzen Stein
    place(game, "g7");  // B
    place(game, "e3");  // W (Fueller)
    place(game, "a7");  // B schliesst Muehle a7-d7-g7
    removeStone(game, "e3");  // Schwarz darf nur den freien weissen Stein nehmen
    place(game, "e4");  // W
    place(game, "b6");  // B (frei, neben der Muehle)
    place(game, "f4");  // W
    place(game, "f6");  // B (frei)
    place(game, "g4");  // W schliesst Muehle e4-f4-g4

    ASSERT_TRUE(game.needsRemoval());
    // Schwarz: a7,d7,g7 in einer Muehle (geschuetzt), b6 und f6 frei.
    std::vector<Field> targets = game.removableStones();
    ASSERT_EQ(static_cast<int>(targets.size()), 2);
    std::string reason;
    // Ein geschuetzter Stein darf nicht entfernt werden.
    Move protectedHit;
    protectedHit.removed = idx("d7");
    ASSERT_FALSE(game.validateMove(protectedHit, reason));
    // Ein freier Stein schon.
    Move freeHit;
    freeHit.removed = idx("b6");
    ASSERT_TRUE(game.validateMove(freeHit, reason));
}

// Spielt eine bewusst muehlenfreie Eroeffnung: 9 weisse und 9 schwarze Steine
// abwechselnd, sodass keine einzige Muehle entsteht und kein Entfernen anfaellt.
// Danach sind beide Spieler in der Ziehphase, Weiss ist am Zug.
static void playMillFreeOpening(Game& game) {
    const char* order[] = {
        "a7", "d7", "g7", "a4", "a1", "g4", "g1", "d1", "d6",
        "b6", "c5", "f6", "e5", "c4", "b2", "e4", "f4", "d2"
    };
    for (const char* field : order) {
        place(game, field);
        ASSERT_FALSE(game.needsRemoval());
    }
}

TEST(Game, ziehphaseErzwingtBenachbartesZielfeld) {
    Game game("Weiss", "Schwarz");
    playMillFreeOpening(game);
    ASSERT_TRUE(game.currentPlayer().currentPhase() == Phase::Moving);

    std::string reason;
    // Gueltig: e5 nach d5 (benachbart, d5 frei).
    Move slide;
    slide.type = MoveType::Slide;
    slide.from = idx("e5");
    slide.to = idx("d5");
    ASSERT_TRUE(game.validateMove(slide, reason));

    // Ungueltig: nicht benachbart.
    Move far;
    far.type = MoveType::Slide;
    far.from = idx("e5");
    far.to = idx("a7");
    ASSERT_FALSE(game.validateMove(far, reason));

    // Ungueltig: Zielfeld belegt.
    Move occupied;
    occupied.type = MoveType::Slide;
    occupied.from = idx("e5");
    occupied.to = idx("e4");
    ASSERT_FALSE(game.validateMove(occupied, reason));

    // Ungueltig: in der Ziehphase kein Setzzug.
    Move place;
    place.type = MoveType::Place;
    place.to = idx("d5");
    ASSERT_FALSE(game.validateMove(place, reason));
}

// Listet alle zur aktuellen Phase passenden, gueltigen Zuege (ohne Entfernen).
static std::vector<Move> legalActions(const Game& g) {
    std::vector<Move> out;
    const Board& b = g.board();
    Color c = g.currentPlayer().color();
    Phase ph = g.currentPlayer().currentPhase();
    std::string reason;
    if (ph == Phase::Placing) {
        for (int t = 0; t < kFieldCount; ++t) {
            Move m;
            m.type = MoveType::Place;
            m.to = t;
            if (g.validateMove(m, reason)) out.push_back(m);
        }
    } else {
        MoveType type = (ph == Phase::Moving) ? MoveType::Slide : MoveType::Jump;
        for (int f = 0; f < kFieldCount; ++f) {
            if (b.colorAt(f) != c) continue;
            for (int t = 0; t < kFieldCount; ++t) {
                Move m;
                m.type = type;
                m.from = f;
                m.to = t;
                if (g.validateMove(m, reason)) out.push_back(m);
            }
        }
    }
    return out;
}

TEST(Game, vollstaendigePartieEndetMitGewinner) {
    // End-to-End: eine deterministische Partie, in der beide Seiten bevorzugt
    // Muehlen schliessen. So fallen Steine, bis ein Spieler unter drei Steine
    // geraet oder keinen Zug mehr hat. Sichert Spielende und Gewinner ab.
    Game game("Weiss", "Schwarz");
    int ply = 0;
    const int kMaxPly = 3000;
    while (!game.isGameOver() && ply < kMaxPly) {
        if (game.needsRemoval()) {
            // Immer den ersten erlaubten gegnerischen Stein nehmen.
            std::vector<Field> targets = game.removableStones();
            ASSERT_FALSE(targets.empty());
            Move r;
            r.removed = targets.front();
            ASSERT_TRUE(game.applyMove(r));
            ++ply;
            continue;
        }
        std::vector<Move> actions = legalActions(game);
        ASSERT_FALSE(actions.empty());
        // Bevorzugt einen Zug, der eine Muehle schliesst (per Probekopie erkannt).
        Move chosen = actions.front();
        for (const Move& cand : actions) {
            Game probe = game;
            probe.applyMove(cand);
            if (probe.needsRemoval()) {
                chosen = cand;
                break;
            }
        }
        ASSERT_TRUE(game.applyMove(chosen));
        ++ply;
    }
    ASSERT_TRUE(game.isGameOver());
    ASSERT_FALSE(game.winner() == Color::None);
    // Der Verlierer ist der Spieler am Zug: zu wenige Steine oder kein Zug.
    ASSERT_TRUE(game.winner() == opponent(game.currentPlayer().color()));
}

// --- Sprint E: InputParser --------------------------------------------------

TEST(InputParser, erkenntGueltigeFelder) {
    InputParser parser;
    ASSERT_EQ(parser.parseField("a1"), idx("a1"));
    ASSERT_EQ(parser.parseField("d3"), idx("d3"));
    ASSERT_EQ(parser.parseField("g7"), idx("g7"));
    // Gross- und Kleinschreibung sowie Leerzeichen sind egal.
    ASSERT_EQ(parser.parseField("  D3 "), idx("d3"));
}

TEST(InputParser, lehntUngueltigeFelderAb) {
    InputParser parser;
    ASSERT_EQ(parser.parseField("h1"), -1);   // Datei h gibt es nicht
    ASSERT_EQ(parser.parseField("a2"), -1);   // a2 ist kein Spielfeld
    ASSERT_EQ(parser.parseField("d8"), -1);   // Reihe 8 gibt es nicht
    ASSERT_EQ(parser.parseField("ddd"), -1);  // falsche Laenge
    ASSERT_EQ(parser.parseField(""), -1);
}

TEST(InputParser, setzzugInSetzphase) {
    InputParser parser;
    Move m;
    ASSERT_TRUE(parser.parseMove("d3", Phase::Placing, m));
    ASSERT_TRUE(m.type == MoveType::Place);
    ASSERT_EQ(m.to, idx("d3"));
    ASSERT_EQ(m.from, -1);
    // Ein Bindestrich-Zug ist in der Setzphase syntaktisch falsch.
    ASSERT_FALSE(parser.parseMove("a1-a4", Phase::Placing, m));
}

TEST(InputParser, ziehUndSpringzugMitZweiFeldern) {
    InputParser parser;
    Move slide;
    ASSERT_TRUE(parser.parseMove("a1-a4", Phase::Moving, slide));
    ASSERT_TRUE(slide.type == MoveType::Slide);
    ASSERT_EQ(slide.from, idx("a1"));
    ASSERT_EQ(slide.to, idx("a4"));

    Move jump;
    ASSERT_TRUE(parser.parseMove("a1-g7", Phase::Flying, jump));
    ASSERT_TRUE(jump.type == MoveType::Jump);

    // Fehlende Trennung oder ungueltiges Feld.
    Move bad;
    ASSERT_FALSE(parser.parseMove("a1a4", Phase::Moving, bad));
    ASSERT_FALSE(parser.parseMove("a1-h9", Phase::Moving, bad));
}

// --- Sprint F: MoveLogger und Wiederherstellung -----------------------------

// Vergleicht zwei Spiele Feld fuer Feld plus Spieler am Zug.
static bool sameState(const Game& a, const Game& b) {
    for (int i = 0; i < kFieldCount; ++i) {
        if (a.board().colorAt(i) != b.board().colorAt(i)) {
            return false;
        }
    }
    return a.currentPlayer().color() == b.currentPlayer().color();
}

// Schreibt einen Text vollstaendig in eine Datei.
static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::trunc);
    file << content;
}

TEST(MoveLogger, speichernUndWiederherstellenErgibtGleichenZustand) {
    const std::string path = "/tmp/muehle_test_protocol.txt";

    // Eine kurze Partie mit einer geschlossenen Muehle (Entfernen) spielen.
    Game original("Anna", "Bert");
    place(original, "a1");  // W
    place(original, "a7");  // B
    place(original, "d1");  // W
    place(original, "a4");  // B
    place(original, "g1");  // W schliesst a1-d1-g1
    removeStone(original, "a7");
    place(original, "d7");  // B
    place(original, "c5");  // W

    MoveLogger logger;
    ASSERT_TRUE(logger.saveSnapshot(path, original));

    // Namen aus der Kopfzeile lesen.
    std::string white;
    std::string black;
    ASSERT_TRUE(logger.readHeader(path, white, black));
    ASSERT_EQ(white, std::string("Anna"));
    ASSERT_EQ(black, std::string("Bert"));

    // In ein frisches Spiel laden und Zustand vergleichen.
    Game restored(white, black);
    ASSERT_TRUE(logger.loadSnapshot(path, restored));
    ASSERT_TRUE(sameState(original, restored));
    ASSERT_EQ(static_cast<int>(restored.history().size()),
              static_cast<int>(original.history().size()));
    // Das gefaltete Entfernen ist erhalten geblieben.
    ASSERT_EQ(restored.history()[4].removed, idx("a7"));

    std::remove(path.c_str());
}

TEST(MoveLogger, lehntSyntaktischKaputtesProtokollAb) {
    const std::string path = "/tmp/muehle_test_broken.txt";
    writeFile(path, "Weiss: A\nSchwarz: B\nzz9\n");
    MoveLogger logger;
    std::vector<Move> moves;
    // Eine unsinnige Koordinate macht das Einlesen ungueltig.
    ASSERT_FALSE(logger.loadGame(path, moves));
    std::remove(path.c_str());
}

TEST(MoveLogger, erkenntRegelwidrigesProtokoll) {
    const std::string path = "/tmp/muehle_test_illegal.txt";
    // Syntaktisch in Ordnung, aber zweimal a1: das zweite Setzen ist regelwidrig.
    writeFile(path, "Weiss: A\nSchwarz: B\na1\na1\n");
    MoveLogger logger;
    std::vector<Move> moves;
    ASSERT_TRUE(logger.loadGame(path, moves));  // Syntax ist gueltig
    Game game("A", "B");
    // Beim erneuten Abspielen ueber die zentrale Validierung faellt der Fehler auf.
    ASSERT_FALSE(logger.loadSnapshot(path, game));
    std::remove(path.c_str());
}

TEST(MoveLogger, lehntZeileMitUeberfluessigenTokensAb) {
    const std::string path = "/tmp/muehle_test_extra.txt";
    // Nach einem vollstaendigen Zug (inkl. Entfernen) darf nichts mehr folgen.
    writeFile(path, "Weiss: A\nSchwarz: B\na1-a4 x g7 foo\n");
    MoveLogger logger;
    std::vector<Move> moves;
    ASSERT_FALSE(logger.loadGame(path, moves));
    std::remove(path.c_str());
}

TEST(MoveLogger, kopfzeileOhneBeideNamenIstUngueltig) {
    const std::string path = "/tmp/muehle_test_header.txt";
    // Nur ein Spielername vorhanden: die Kopfzeile gilt als unvollstaendig.
    writeFile(path, "Weiss: A\n");
    MoveLogger logger;
    std::string white;
    std::string black;
    ASSERT_FALSE(logger.readHeader(path, white, black));
    std::remove(path.c_str());
}

// --- Sprint G: Zusatzfeatures -----------------------------------------------

TEST(Undo, nimmtLetztenSetzzugZurueck) {
    Game game("Weiss", "Schwarz");
    place(game, "a1");  // W
    place(game, "a7");  // B, jetzt ist wieder Weiss am Zug
    ASSERT_EQ(static_cast<int>(game.history().size()), 2);
    ASSERT_EQ(game.currentPlayer().color(), Color::White);

    ASSERT_TRUE(game.undoLastMove());
    // Der schwarze Stein ist verschwunden, Schwarz ist wieder am Zug.
    ASSERT_EQ(static_cast<int>(game.history().size()), 1);
    ASSERT_EQ(game.board().colorAt(idx("a7")), Color::None);
    ASSERT_EQ(game.board().colorAt(idx("a1")), Color::White);
    ASSERT_EQ(game.currentPlayer().color(), Color::Black);
}

TEST(Undo, stelltGeschlosseneMuehleUndEntferntenSteinWiederHer) {
    Game game("Weiss", "Schwarz");
    place(game, "a1");        // W
    place(game, "a7");        // B
    place(game, "d1");        // W
    place(game, "a4");        // B
    place(game, "g1");        // W schliesst a1-d1-g1
    removeStone(game, "a7");  // Weiss entfernt a7, dann ist Schwarz am Zug
    ASSERT_EQ(game.board().colorAt(idx("a7")), Color::None);
    ASSERT_EQ(game.currentPlayer().color(), Color::Black);

    // Undo nimmt den gesamten Muehlen-Zug inklusive Entfernen zurueck.
    ASSERT_TRUE(game.undoLastMove());
    ASSERT_EQ(game.board().colorAt(idx("g1")), Color::None);  // Setzzug weg
    ASSERT_EQ(game.board().colorAt(idx("a7")), Color::Black); // Stein zurueck
    ASSERT_EQ(static_cast<int>(game.history().size()), 4);
    ASSERT_EQ(game.currentPlayer().color(), Color::White);
    ASSERT_FALSE(game.needsRemoval());
}

TEST(Undo, lehntOhneZugUndWaehrendEntfernenAb) {
    Game game("Weiss", "Schwarz");
    // Ohne jeden Zug gibt es nichts zurueckzunehmen.
    ASSERT_FALSE(game.undoLastMove());

    // Eine Muehle schliessen, ohne den Stein schon zu entfernen.
    place(game, "a1");  // W
    place(game, "a7");  // B
    place(game, "d1");  // W
    place(game, "a4");  // B
    place(game, "g1");  // W schliesst Muehle, Entfernen steht aus
    ASSERT_TRUE(game.needsRemoval());
    // Mitten im Zug (Entfernen offen) ist kein Undo moeglich.
    ASSERT_FALSE(game.undoLastMove());
}

TEST(Hint, listetAlleSetzzuegeUndSchrumpftMitJedemStein) {
    Game game("Weiss", "Schwarz");
    // Auf leerem Brett ist jedes der 24 Felder ein gueltiger Setzzug.
    ASSERT_EQ(static_cast<int>(game.legalMoves().size()), kFieldCount);
    place(game, "a1");
    // Ein Feld ist belegt, also bleiben 23 Setzzuege fuer Schwarz.
    ASSERT_EQ(static_cast<int>(game.legalMoves().size()), kFieldCount - 1);
    // In der Ziehphase muss der Hinweis mit der unabhaengig gezaehlten Liste
    // gueltiger Aktionen uebereinstimmen.
    Game moving("Weiss", "Schwarz");
    playMillFreeOpening(moving);
    ASSERT_TRUE(moving.currentPlayer().currentPhase() == Phase::Moving);
    ASSERT_EQ(static_cast<int>(moving.legalMoves().size()),
              static_cast<int>(legalActions(moving).size()));
    ASSERT_FALSE(moving.legalMoves().empty());
}

TEST(Hint, zeigtWaehrendEntfernenDieEntfernbarenSteine) {
    Game game("Weiss", "Schwarz");
    place(game, "a1");  // W
    place(game, "a7");  // B
    place(game, "d1");  // W
    place(game, "a4");  // B
    place(game, "g1");  // W schliesst Muehle
    ASSERT_TRUE(game.needsRemoval());
    std::vector<Move> hints = game.legalMoves();
    // Genauso viele Hinweise wie entfernbare Steine, und jeder Hinweis ist ein
    // Entfernen (kein Zielfeld, kein Quellfeld).
    ASSERT_EQ(static_cast<int>(hints.size()),
              static_cast<int>(game.removableStones().size()));
    ASSERT_FALSE(hints.empty());
    ASSERT_EQ(hints.front().to, -1);
    ASSERT_EQ(hints.front().from, -1);
    ASSERT_TRUE(hints.front().removed >= 0);
}

// Spielt eine deterministische Partie bis zum Ende, indem bevorzugt Muehlen
// geschlossen werden. Dieselbe Strategie wie im End-to-End-Test, hier wieder-
// verwendet, um eine beendete Partie fuer die Statistik zu erzeugen.
static void playToEnd(Game& game) {
    int ply = 0;
    const int kMaxPly = 3000;
    while (!game.isGameOver() && ply < kMaxPly) {
        if (game.needsRemoval()) {
            std::vector<Field> targets = game.removableStones();
            Move r;
            r.removed = targets.front();
            game.applyMove(r);
            ++ply;
            continue;
        }
        std::vector<Move> actions = legalActions(game);
        Move chosen = actions.front();
        for (const Move& cand : actions) {
            Game probe = game;
            probe.applyMove(cand);
            if (probe.needsRemoval()) {
                chosen = cand;
                break;
            }
        }
        game.applyMove(chosen);
        ++ply;
    }
}

TEST(Statistics, zaehltSiegeUndSortiertNachSiegen) {
    Statistics stats;
    // Anna gewinnt gegen Bert, Cara gewinnt gegen Anna, Bert/Anna bleibt offen.
    stats.addResult(GameResult{"Anna", "Bert", Color::White});
    stats.addResult(GameResult{"Anna", "Cara", Color::Black});
    stats.addResult(GameResult{"Bert", "Anna", Color::None});
    ASSERT_EQ(stats.totalGames(), 3);

    std::vector<Statistics::Entry> r = stats.ranking();
    ASSERT_EQ(static_cast<int>(r.size()), 3);
    // Anna und Cara haben je einen Sieg; bei Gleichstand entscheidet der Name.
    ASSERT_EQ(r[0].name, std::string("Anna"));
    ASSERT_EQ(r[0].wins, 1);
    ASSERT_EQ(r[0].losses, 1);
    ASSERT_EQ(r[0].games, 3);
    ASSERT_EQ(r[1].name, std::string("Cara"));
    ASSERT_EQ(r[1].wins, 1);
    // Bert hat keinen Sieg und steht hinten.
    ASSERT_EQ(r[2].name, std::string("Bert"));
    ASSERT_EQ(r[2].wins, 0);
    ASSERT_EQ(r[2].losses, 1);
    ASSERT_EQ(r[2].games, 2);
}

TEST(Statistics, wertetBeendetePartieAusProtokollAus) {
    const std::string path = "/tmp/muehle_test_stats.txt";
    // Eine vollstaendige Partie spielen und als Protokoll speichern.
    Game game("Anna", "Bert");
    playToEnd(game);
    ASSERT_TRUE(game.isGameOver());
    MoveLogger logger;
    ASSERT_TRUE(logger.saveSnapshot(path, game));

    GameResult result;
    ASSERT_TRUE(evaluateLog(path, result));
    ASSERT_EQ(result.whiteName, std::string("Anna"));
    ASSERT_EQ(result.blackName, std::string("Bert"));
    // Der aus dem Protokoll ermittelte Gewinner stimmt mit dem Spiel ueberein.
    ASSERT_TRUE(result.winner == game.winner());
    ASSERT_FALSE(result.winner == Color::None);
    std::remove(path.c_str());
}

TEST(Statistics, offenerZwischenstandHatKeinenGewinner) {
    const std::string path = "/tmp/muehle_test_stats_open.txt";
    // Nur wenige Setzzuege: die Partie ist noch lange nicht entschieden.
    Game game("Anna", "Bert");
    place(game, "a1");
    place(game, "a7");
    place(game, "d1");
    ASSERT_FALSE(game.isGameOver());
    MoveLogger logger;
    ASSERT_TRUE(logger.saveSnapshot(path, game));

    GameResult result;
    ASSERT_TRUE(evaluateLog(path, result));
    // Lesbar und gueltig, aber ohne Sieger: zaehlt nicht in die Sieg-Statistik.
    ASSERT_TRUE(result.winner == Color::None);
    std::remove(path.c_str());
}

TEST(Statistics, lehntBeschaedigtesProtokollAb) {
    const std::string path = "/tmp/muehle_test_stats_broken.txt";
    // Gueltige Kopfzeile, aber ein regelwidriger doppelter Setzzug.
    writeFile(path, "Weiss: A\nSchwarz: B\na1\na1\n");
    GameResult result;
    ASSERT_FALSE(evaluateLog(path, result));
    std::remove(path.c_str());
}

TEST(MoveTimer, summiertUndMitteltAufgenommeneZuege) {
    MoveTimer timer;
    timer.record(100);
    timer.record(300);
    timer.record(200);
    ASSERT_EQ(timer.count(), 3);
    ASSERT_EQ(timer.total(), 600);
    ASSERT_EQ(timer.average(), 200);   // 600 / 3
    ASSERT_EQ(timer.longest(), 300);
}

TEST(MoveTimer, leerUndUnplausibleWerte) {
    MoveTimer timer;
    // Ohne Zuege ist alles null, ohne Division durch null.
    ASSERT_EQ(timer.count(), 0);
    ASSERT_EQ(timer.total(), 0);
    ASSERT_EQ(timer.average(), 0);
    ASSERT_EQ(timer.longest(), 0);
    // Eine negative Messung wird verworfen, ein gueltiger Wert danach zaehlt.
    timer.record(-50);
    ASSERT_EQ(timer.count(), 0);
    timer.record(80);
    ASSERT_EQ(timer.count(), 1);
    ASSERT_EQ(timer.average(), 80);
}

TEST(BoxArt, waehltKnotenNachAnschluessen) {
    // up, down, left, right
    ASSERT_EQ(std::string(ConsoleRenderer::nodeGlyph(false, true, false, true)),
              std::string("┌"));  // obere linke Ecke
    ASSERT_EQ(std::string(ConsoleRenderer::nodeGlyph(true, false, true, false)),
              std::string("┘"));  // untere rechte Ecke
    ASSERT_EQ(std::string(ConsoleRenderer::nodeGlyph(true, true, true, true)),
              std::string("┼"));  // vollstaendige Kreuzung
    ASSERT_EQ(std::string(ConsoleRenderer::nodeGlyph(false, true, true, true)),
              std::string("┬"));  // T-Stueck nach unten
}

TEST(BoxArt, faelltBeiUngueltigenAnschluessenAufMittelpunktZurueck) {
    // Gar kein Anschluss oder nur eine einzelne Richtung kommt auf dem Brett
    // nicht vor und ergibt den neutralen Mittelpunkt.
    ASSERT_EQ(std::string(ConsoleRenderer::nodeGlyph(false, false, false, false)),
              std::string("·"));
    ASSERT_EQ(std::string(ConsoleRenderer::nodeGlyph(true, false, false, false)),
              std::string("·"));
}

TEST(CommandLine, erkenntLoggingFlagUndPfad) {
    // Ohne Argumente ist nichts aktiv.
    CommandLineOptions none = parseCommandLine({});
    ASSERT_FALSE(none.logging);
    ASSERT_FALSE(none.help);

    // Reines Flag schaltet das Logging ein und ueberlaesst den Namen der
    // Anwendung (logPath bleibt leer).
    CommandLineOptions on = parseCommandLine({"--log"});
    ASSERT_TRUE(on.logging);
    ASSERT_TRUE(on.logPath.empty());

    // Mit eigenem Pfad.
    CommandLineOptions path = parseCommandLine({"--log=data/partie.log"});
    ASSERT_TRUE(path.logging);
    ASSERT_EQ(path.logPath, std::string("data/partie.log"));

    // Hilfe.
    CommandLineOptions help = parseCommandLine({"-h"});
    ASSERT_TRUE(help.help);
}

TEST(CommandLine, meldetUnbekanntesArgument) {
    CommandLineOptions opts = parseCommandLine({"--unsinn", "--log"});
    // Das Logging wird trotzdem erkannt, das unbekannte Argument vermerkt.
    ASSERT_TRUE(opts.logging);
    ASSERT_TRUE(opts.unknownOption);
    ASSERT_EQ(opts.unknown, std::string("--unsinn"));
}

TEST(EventLog, inaktivSchreibtNichts) {
    EventLog log;
    ASSERT_FALSE(log.isActive());
    log.log("wird verworfen");
    ASSERT_EQ(log.entryCount(), 0);
}

TEST(EventLog, aktivSchreibtNummerierteZeilen) {
    const std::string path = "/tmp/muehle_test_eventlog.txt";
    std::remove(path.c_str());
    {
        EventLog log;
        ASSERT_TRUE(log.open(path));
        ASSERT_TRUE(log.isActive());
        log.log("erste Zeile");
        log.log("zweite Zeile");
        ASSERT_EQ(log.entryCount(), 2);
    }
    // Die Datei enthaelt Kopf und beide nummerierten Zeilen.
    std::ifstream in(path);
    std::string content((std::istreambuf_iterator<char>(in)),
                        std::istreambuf_iterator<char>());
    ASSERT_TRUE(content.find("0001  erste Zeile") != std::string::npos);
    ASSERT_TRUE(content.find("0002  zweite Zeile") != std::string::npos);
    std::remove(path.c_str());
}

TEST(MoveTimer, formatiertSekundenMitZweiNachkommastellen) {
    // Glatte und gerundete Werte.
    ASSERT_EQ(MoveTimer::formatSeconds(56160), std::string("56.16 s"));
    ASSERT_EQ(MoveTimer::formatSeconds(1000), std::string("1.00 s"));
    ASSERT_EQ(MoveTimer::formatSeconds(56166), std::string("56.17 s")); // gerundet
    // Fuehrende Null in den Nachkommastellen.
    ASSERT_EQ(MoveTimer::formatSeconds(2050), std::string("2.05 s"));
}

TEST(MoveTimer, formatiertRandwerte) {
    // Null und ein negativer (unplausibler) Wert ergeben sauber 0.00 s.
    ASSERT_EQ(MoveTimer::formatSeconds(0), std::string("0.00 s"));
    ASSERT_EQ(MoveTimer::formatSeconds(-100), std::string("0.00 s"));
}

int main() {
    return ::testing::runAll();
}
