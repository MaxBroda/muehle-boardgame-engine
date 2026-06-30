# Mühle-Engine (C++)

[![CI](https://github.com/MaxBroda/muehle-boardgame-engine/actions/workflows/ci.yml/badge.svg)](https://github.com/MaxBroda/muehle-boardgame-engine/actions/workflows/ci.yml)

Konsolenbasierte Spiele-Engine für das Brettspiel Mühle. Teil des IU-Portfolios
DLBMINPAPCC01. Entwickelt in C++17, ausschließlich mit der C++-Standardbibliothek,
Build über CMake.

## Verzeichnisse

- `include/` Header der Klassen (Board, Player, Move, Game, MoveLogger, InputParser, ConsoleRenderer, Statistics, MoveTimer, CommandLine, EventLog)
- `src/` Implementierungen und `main.cpp`
- `tests/` eigenes Test-Framework (`test_framework.h`) und Tests (`test_main.cpp`)
- `data/` Spielprotokolle und Spielstände

## Bauen und Ausführen

```sh
cmake -S . -B build
cmake --build build

./build/muehle              # Anwendung starten
./build/muehle --log        # erweiterte Protokollierung (Datei automatisch in data/)
./build/muehle --log=pfad   # Protokollierung in eine eigene Datei
./build/muehle --help       # Aufruf und Optionen anzeigen
./build/muehle_tests        # Tests direkt ausführen
ctest --test-dir build      # Tests über CTest ausführen
```

## Bedienung

Das Hauptmenü bietet neues Spiel, Spielstand fortsetzen, Protokoll wiedergeben,
spielübergreifende Statistik und Beenden. Beim neuen Spiel lässt sich wählen, ob
ein zweiter Mensch oder der Computer der Gegner ist. Der Computer spielt Schwarz,
der Mensch beginnt als Weiß. Während eines Zuges sind möglich:

- ein Feld zum Setzen (z.B. `d3`) oder ein Zug `von-nach` (z.B. `a1-a4`)
- `h` zeigt alle aktuell gültigen Züge an (Hinweis-Modus)
- `u` nimmt den letzten Zug zurück (Undo)
- `q` beendet die Partie, danach kann der Spielstand gesichert werden

Während des Spiels steht die Bedenkzeit beider Spieler neben dem Brett (Gesamt
und letzter Zug), nach dem Spielende folgt die Auswertung. Eine beendete Partie
wird automatisch als Protokoll gesichert und fließt in die Statistik ein.

## Continuous Integration

Jeder Push und jeder Pull Request wird über eine GitHub-Actions-Pipeline gebaut
und getestet (`.github/workflows/ci.yml`). Der Branch `main` ist über ein Ruleset
geschützt und nur über einen Pull Request mit grüner Pipeline erreichbar.

## Stand

Phase 2 mit den Sprints A bis H umgesetzt: vollständige Spiellogik, Protokoll mit
Speichern, Fortsetzen und Wiedergabe, Box-Drawing-Spielfeld sowie die
Zusatzfeatures Undo, Hinweis-Modus, Statistik, Zugdauer-Messung, optionales
Logging und ein KI-Gegner über eine Minimax-Suche mit Alpha-Beta-Schnitt. Der
weitere Plan und der aktuelle Fortschritt stehen in `Uebergabe_Phase2.md`.
