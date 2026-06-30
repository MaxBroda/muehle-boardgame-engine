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
ein zweiter Mensch oder der Computer der Gegner ist. Gegen den Computer wird
zusätzlich die Schwierigkeit gewählt (Leicht, Mittel, Schwer), die sich aus
Suchtiefe und einer Fehlerquote ergibt. Der Computer spielt Schwarz, der Mensch
beginnt als Weiß. Während eines Zuges sind möglich:

- ein Feld zum Setzen (z.B. `d3`) oder ein Zug `von-nach` (z.B. `a1-a4`)
- `h` zeigt alle aktuell gültigen Züge an (Hinweis-Modus)
- `u` nimmt den letzten Zug zurück (Undo)
- `q` beendet die Partie, danach kann der Spielstand gesichert werden

Neben dem Brett steht eine Infospalte mit dem Spieler am Zug, der Bedenkzeit des
letzten Zugs beider Spieler und den Steinzahlen. Die aktuelle Phase wird
hervorgehoben unter dem Brett angezeigt, und bei jedem Phasenwechsel erscheint ein
kurzer Hinweis. Gesamtzeit, Durchschnitt und längster Zug folgen erst in der
Auswertung am Spielende. Eine beendete Partie wird automatisch als Protokoll
gesichert und fließt in die Statistik ein.

## KI-Gegner

Der Computergegner steckt in der Klasse `AiPlayer` (`include/AiPlayer.h`,
`src/AiPlayer.cpp`). Er enthält keine eigenen Spielregeln, sondern nutzt dieselbe
zentrale Prüfung wie ein menschlicher Zug: Er erzeugt Kandidatenzüge über
`Game::legalMoves`, spielt sie auf Kopien des Spiels durch und bewertet das
Ergebnis. Dadurch kann die KI nie gegen die Regeln verstoßen.

### Ablauf eines Computerzugs

Ist der Computer am Zug, wählt er selbst einen Halbzug. Schließt dieser Zug eine
Mühle, gehört das Entfernen eines gegnerischen Steins zum selben Zug und wird
gemeinsam ausgeführt und als ein Satz angezeigt, etwa „Computer zieht von b6 nach
d6 und entfernt den gegnerischen Stein auf f4“. Die Rechenzeit fließt wie bei
einem Menschen in die Bedenkzeit ein.

### Suche: Minimax mit Alpha-Beta

Die KI sucht den besten Zug über Minimax mit Alpha-Beta-Schnitt bis zu einer
festen Tiefe, gemessen in Halbzügen. An den Blättern der Suche wird die Stellung
aus Sicht der KI bewertet. Wer am Zug ist, ergibt sich aus dem Spielzustand: Ist
die KI am Zug, maximiert sie den Wert, ist der Gegner am Zug, minimiert er ihn.
Weil das Entfernen nach einer Mühle demselben Spieler gehört, wird es automatisch
der richtigen Seite zugerechnet, ohne Sonderfall. Der Alpha-Beta-Schnitt
überspringt Äste, die das Ergebnis nicht mehr verbessern können, und macht die
Suche schneller, ohne das Ergebnis zu verändern.

### Bewertung einer Stellung

Eine Stellung wird aus vier Bausteinen bewertet, jeweils als Differenz zwischen
der KI und dem Gegner:

- Material: Steine auf dem Brett plus Steine in der Hand. Der mit Abstand
  wichtigste Faktor.
- Fertige Mühlen: Steine, die in einer vollständigen Mühle stehen.
- Fast fertige Mühlen: Linien mit zwei eigenen Steinen und einem freien dritten
  Feld. Sie geben der Bewertung ein Gefälle, sodass die KI gezielt auf Mühlen
  hinarbeitet, statt bei gleichem Material ziellos hin und her zu ziehen.
- Beweglichkeit: Anzahl der möglichen Ziehschritte auf benachbarte freie Felder.
  Das belohnt es, die eigenen Optionen offen zu halten und den Gegner einzuengen.

Eine entschiedene Stellung überlagert alles andere: ein Sieg zählt sehr hoch, eine
Niederlage sehr niedrig. Die Bewertung ist spiegelbildlich, der Wert aus Sicht des
Gegners ist also genau das Negative.

Damit die KI in ausgeglichenen Lagen nicht denselben Stein sinnlos hin und her
schiebt, gibt es einen Anti-Pendel-Schutz: Stehen mehrere gleichwertige Züge zur
Wahl, macht die KI ihren letzten Zug nicht einfach rückgängig, solange es eine
gleichwertige Alternative gibt.

### Schwierigkeitsgrade

Gegen den Computer wird beim Spielstart eine von drei Stufen gewählt. Sie
unterscheiden sich in der Suchtiefe und in einer Fehlerquote. Die Fehlerquote ist
die Wahrscheinlichkeit, mit der die KI statt des besten Zugs absichtlich einen
zufälligen gültigen Zug spielt. So wird sie schwächer und für Menschen schlagbar,
ohne je einen regelwidrigen Zug zu machen.

| Stufe | Suchtiefe (Halbzüge) | Fehlerquote |
|---|---|---|
| Leicht | 1 | 50 % |
| Mittel | 3 | 15 % |
| Schwer | 4 | 0 % |

Leicht schaut nur einen Halbzug voraus und patzt oft, lässt also Mühlen zu.
Mittel plant einige Züge weit und leistet sich nur gelegentlich einen Fehler.
Schwer spielt mit voller Tiefe und ohne absichtliche Fehler. Es bleibt eine Suche
mit begrenzter Tiefe, also kein perfekter Spieler, aber ein zielstrebiger Gegner,
der einen Vorteil auch in einen Sieg verwandelt.

## Continuous Integration

Jeder Push und jeder Pull Request wird über eine GitHub-Actions-Pipeline gebaut
und getestet (`.github/workflows/ci.yml`). Der Branch `main` ist über ein Ruleset
geschützt und nur über einen Pull Request mit grüner Pipeline erreichbar.

## Stand

Phase 2 mit den Sprints A bis H umgesetzt: vollständige Spiellogik, Protokoll mit
Speichern, Fortsetzen und Wiedergabe, Box-Drawing-Spielfeld sowie die
Zusatzfeatures Undo, Hinweis-Modus, Statistik, Zugdauer-Messung, optionales
Logging und ein KI-Gegner über eine Minimax-Suche mit Alpha-Beta-Schnitt in drei
Schwierigkeitsstufen. Der weitere Plan und der aktuelle Fortschritt stehen in
`Uebergabe_Phase2.md`.
