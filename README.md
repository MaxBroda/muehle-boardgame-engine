# Mühle-Engine (C++)

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
./build/muehle --log        # mit erweiterter Protokollierung (data/muehle.log)
./build/muehle --log=pfad   # Protokollierung in eine eigene Datei
./build/muehle --help       # Aufruf und Optionen anzeigen
./build/muehle_tests        # Tests direkt ausführen
ctest --test-dir build      # Tests über CTest ausführen
```

## Bedienung

Das Hauptmenü bietet neues Spiel, Spielstand fortsetzen, Protokoll wiedergeben,
spielübergreifende Statistik und Beenden. Während eines Zuges sind möglich:

- ein Feld zum Setzen (z.B. `d3`) oder ein Zug `von-nach` (z.B. `a1-a4`)
- `h` zeigt alle aktuell gültigen Züge an (Hinweis-Modus)
- `u` nimmt den letzten Zug zurück (Undo)
- `q` beendet die Partie, danach kann der Spielstand gesichert werden

Nach dem Spielende wird die Bedenkzeit je Spieler ausgewertet.

## Stand

Phase 2 mit den Sprints A bis G umgesetzt: vollständige Spiellogik, Protokoll mit
Speichern, Fortsetzen und Wiedergabe, Box-Drawing-Spielfeld sowie die
Zusatzfeatures Undo, Hinweis-Modus, Statistik, Zugdauer-Messung und optionales
Logging. Der weitere Plan und der aktuelle Fortschritt stehen in
`Uebergabe_Phase2.md`.
