# Mühle-Engine (C++)

Konsolenbasierte Spiele-Engine für das Brettspiel Mühle. Teil des IU-Portfolios
DLBMINPAPCC01. Entwickelt in C++17, ausschließlich mit der C++-Standardbibliothek,
Build über CMake.

## Verzeichnisse

- `include/` Header der Klassen (Board, Player, Move, Game, MoveLogger, InputParser, ConsoleRenderer)
- `src/` Implementierungen und `main.cpp`
- `tests/` eigenes Test-Framework (`test_framework.h`) und Tests (`test_main.cpp`)
- `data/` Spielprotokolle und Spielstände

## Bauen und Ausführen

```sh
cmake -S . -B build
cmake --build build

./build/muehle          # Anwendung starten
./build/muehle_tests    # Tests direkt ausführen
ctest --test-dir build  # Tests über CTest ausführen
```

## Stand

Sprint A abgeschlossen: Projektgerüst, Klassen-Skelette, eigenes Test-Framework,
lauffähiger Build. Der weitere Plan und der aktuelle Fortschritt stehen in
`Uebergabe_Phase2.md`.
