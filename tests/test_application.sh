#!/usr/bin/env bash
# Anwendungstests (End-to-End) fuer die Muehle-Engine.
# Jeder Test startet das Programm mit vorbereiteten Eingaben und prueft
# die Ausgabe auf erwartete Schluesselwoerter.
#
# Aufruf:  bash tests/test_application.sh [build/muehle]

set -uo pipefail

MUEHLE="${1:-build/muehle}"
PASS=0
FAIL=0
DATA_BAK=""
DATA_CREATED=0

red()   { printf '\033[1;31m%s\033[0m\n' "$*"; }
green() { printf '\033[1;32m%s\033[0m\n' "$*"; }
cyan()  { printf '\033[1;36m%s\033[0m\n' "$*"; }

assert_contains() {
    local label="$1" output="$2" pattern="$3"
    if echo "$output" | grep -qi "$pattern"; then
        green "  PASS  $label"
        ((PASS++))
    else
        red   "  FAIL  $label  (erwartet: '$pattern')"
        ((FAIL++))
    fi
}

assert_not_contains() {
    local label="$1" output="$2" pattern="$3"
    if echo "$output" | grep -qi "$pattern"; then
        red   "  FAIL  $label  (unerwartet: '$pattern')"
        ((FAIL++))
    else
        green "  PASS  $label"
        ((PASS++))
    fi
}

setup_data_dir() {
    if [ -d data ]; then
        DATA_BAK="data.bak.$$"
        mv data "$DATA_BAK"
    fi
    mkdir -p data
    DATA_CREATED=1
}

teardown_data_dir() {
    # Nur aufraeumen, wenn dieser Lauf das Testverzeichnis selbst angelegt hat.
    # Ohne diese Bremse wuerde der EXIT-Trap nach dem letzten Test das bereits
    # zurueckgesicherte echte data-Verzeichnis samt Spielstaenden loeschen.
    if [ "$DATA_CREATED" -eq 0 ]; then
        return
    fi
    rm -rf data
    if [ -n "$DATA_BAK" ] && [ -d "$DATA_BAK" ]; then
        mv "$DATA_BAK" data
        DATA_BAK=""
    fi
    DATA_CREATED=0
}

trap teardown_data_dir EXIT

# ---------- T01: Hauptmenue und Beenden ----------
test_hauptmenue() {
    cyan "T01: Hauptmenue und Beenden"
    local out
    out=$(echo "5" | "$MUEHLE" 2>&1)
    assert_contains "Willkommensmeldung"    "$out" "Willkommen bei Muehle"
    assert_contains "Menue wird angezeigt"  "$out" "MUEHLE"
    assert_contains "Option Neues Spiel"    "$out" "Neues Spiel"
    assert_contains "Option Beenden"        "$out" "Beenden"
    assert_contains "Verabschiedung"        "$out" "Auf Wiedersehen"
}

# ---------- T02: Ungueltige Menueeingabe ----------
test_ungueltige_menueeingabe() {
    cyan "T02: Ungueltige Menueeingabe"
    local out
    out=$(printf 'x\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Fehlermeldung" "$out" "Bitte 1 bis 5"
}

# ---------- T03: Hilfe-Flag ----------
test_hilfe_flag() {
    cyan "T03: Kommandozeile --help"
    local out
    out=$("$MUEHLE" --help 2>&1)
    assert_contains "Hilfetext"     "$out" "log"
    assert_contains "Hilfe-Option"  "$out" "help"
}

# ---------- T04: Unbekanntes Argument ----------
test_unbekanntes_argument() {
    cyan "T04: Unbekanntes Argument"
    local out
    out=$(echo "5" | "$MUEHLE" --xyz 2>&1)
    assert_contains "Warnung" "$out" "Unbekanntes Argument"
}

# ---------- T05: Neue Partie starten (Mensch vs. Mensch) ----------
test_neue_partie_mensch() {
    cyan "T05: Neue Partie Mensch vs. Mensch — Starten und Abbrechen"
    local out
    out=$(printf '1\n1\nAlice\nBob\nq\nn\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Spieler am Zug"   "$out" "Am Zug"
    assert_contains "Setzphase"        "$out" "Setzphase"
    assert_contains "Zurueck"          "$out" "Hauptmenue"
}

# ---------- T06: Neue Partie gegen Computer ----------
test_neue_partie_computer() {
    cyan "T06: Neue Partie gegen Computer"
    local out
    out=$(printf '1\n2\n1\nMax\nq\nn\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Schwierigkeit"        "$out" "Schwierigkeit"
    assert_contains "Computer-Level"       "$out" "Leicht"
    assert_contains "Computer ist Gegner"  "$out" "Computer"
}

# ---------- T07: Setzzug und Spielerwechsel ----------
test_setzzug() {
    cyan "T07: Setzzug ausfuehren"
    local out
    out=$(printf '1\n1\nA\nB\nd1\nq\nn\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Spielerwechsel" "$out" "Am Zug.*B"
}

# ---------- T08: Ungueltige Eingabe im Spiel ----------
test_ungueltige_eingabe() {
    cyan "T08: Ungueltige Eingabe waehrend Partie"
    local out
    out=$(printf '1\n1\nA\nB\nz9\nq\nn\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Fehlermeldung" "$out" "nicht verstanden"
}

# ---------- T09: Besetztes Feld ablehnen ----------
test_besetztes_feld() {
    cyan "T09: Besetztes Feld ablehnen"
    local out
    out=$(printf '1\n1\nA\nB\nd1\nd1\nq\nn\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Feld belegt" "$out" "belegt"
}

# ---------- T10: Hinweis-Modus ----------
test_hinweis() {
    cyan "T10: Hinweis-Modus"
    local out
    out=$(printf '1\n1\nA\nB\nh\nq\nn\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Zuege angezeigt" "$out" "Moegliche Zuege"
}

# ---------- T11: Undo ----------
test_undo() {
    cyan "T11: Undo eines Zugs"
    local out
    out=$(printf '1\n1\nA\nB\nd1\nu\nq\nn\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Undo bestaetigt" "$out" "zurueckgenommen"
}

# ---------- T12: Undo ohne Zug ----------
test_undo_leer() {
    cyan "T12: Undo ohne vorherigen Zug"
    local out
    out=$(printf '1\n1\nA\nB\nu\nq\nn\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Kein Zug" "$out" "Kein Zug"
}

# ---------- T13: Speichern und Fortsetzen ----------
test_speichern_fortsetzen() {
    cyan "T13: Speichern und Fortsetzen"
    setup_data_dir

    # Partie starten, einen Zug machen, speichern
    printf '1\n1\nAlice\nBob\nd1\nq\nj\ntest_save\n5\n' | "$MUEHLE" 2>&1 >/dev/null

    assert_contains "Datei existiert" "$(ls data/)" "test_save"

    # Fortsetzen
    local out
    out=$(printf '2\n1\nq\nn\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Spielstand geladen" "$out" "geladen"

    teardown_data_dir
}

# ---------- T14: Protokoll-Wiedergabe ----------
test_wiedergabe() {
    cyan "T14: Protokoll-Wiedergabe"
    setup_data_dir

    # Partie mit ein paar Zuegen speichern
    printf '1\n1\nAlice\nBob\nd1\na1\nq\nj\nreplay_test\n5\n' | "$MUEHLE" 2>&1 >/dev/null

    # Wiedergabe am Stueck
    local out
    out=$(printf '3\n1\n1\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Partie offen" "$out" "offen\|Ende des Protokolls"

    teardown_data_dir
}

# ---------- T15: Statistik ohne Partien ----------
test_statistik_leer() {
    cyan "T15: Statistik ohne Partien"
    setup_data_dir

    local out
    out=$(printf '4\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Keine Partien" "$out" "Keine"

    teardown_data_dir
}

# ---------- T16: Komplette Partie bis Spielende ----------
test_komplette_partie() {
    cyan "T16: Komplette Partie bis Spielende"
    setup_data_dir

    # Deterministische Zugfolge: Weiss baut Muehlen, Schwarz setzt daneben.
    # Weiss: a1, d1, g1 (Muehle Reihe 1) -> entferne z.B. a7
    # Dann weitere Zuege bis ein Spieler < 3 Steine hat.
    # Kuerzeste Strategie: Weiss schliesst drei Muehlen und entfernt je einen Stein,
    # dann gewinnt Weiss sobald Schwarz < 3 Steine hat.
    local input
    input=$(cat <<'MOVES'
1
1
W
S
a1
a7
d1
d7
g1
a7
a4
d6
d2
b6
g4
c5
a7
g7
g1
e5
d3
f6
a1-a7
d7-g7
g1-d1
g7-d7
a7-a4
d7-g7
d1-g1
g7-d7
a4-a1
d7-g7
g1-d1
g7-d7
a1-a4
d7-g7
d1-g1
g7-d7
q
n
5
MOVES
)
    local out
    out=$(echo "$input" | "$MUEHLE" 2>&1) || true
    # Wir pruefen nur, dass das Programm ohne Absturz durchlaeuft
    assert_contains "Programm laeuft" "$out" "Willkommen"
    assert_not_contains "Kein Absturz" "$out" "Segmentation\|abort\|core dump"

    teardown_data_dir
}

# ---------- T17: KI-Partie (alle Schwierigkeitsgrade) ----------
test_ki_schwierigkeitsgrade() {
    cyan "T17: KI-Schwierigkeitsgrade"
    for level in 1 2 3; do
        local label out
        case $level in
            1) label="Leicht" ;;
            2) label="Mittel" ;;
            3) label="Schwer" ;;
        esac
        out=$(printf "1\n2\n${level}\nTester\nq\nn\n5\n" | "$MUEHLE" 2>&1)
        assert_contains "KI $label" "$out" "$label"
    done
}

# ---------- T18: Logging-Flag ----------
test_logging() {
    cyan "T18: Logging-Flag"
    setup_data_dir

    local out
    out=$(echo "5" | "$MUEHLE" --log 2>&1)
    assert_contains "Log aktiv" "$out" "Protokollierung aktiv"

    local logfile
    logfile=$(find data -name 'log_*.log' 2>/dev/null | head -1)
    assert_contains "Logdatei erstellt" "${logfile:-leer}" "log_"

    teardown_data_dir
}

# ---------- T19: Fortsetzen ohne Dateien ----------
test_fortsetzen_leer() {
    cyan "T19: Fortsetzen ohne gespeicherte Dateien"
    setup_data_dir

    local out
    out=$(printf '2\n\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Keine Dateien" "$out" "Keine Dateien\|Abgebrochen"

    teardown_data_dir
}

# ---------- T20: Wiedergabe ohne Dateien ----------
test_wiedergabe_leer() {
    cyan "T20: Wiedergabe ohne Dateien"
    setup_data_dir

    local out
    out=$(printf '3\n\n5\n' | "$MUEHLE" 2>&1)
    assert_contains "Keine Dateien" "$out" "Keine Dateien\|Abgebrochen"

    teardown_data_dir
}

# ========== Ausfuehrung ==========
cyan "========================================"
cyan "  Muehle Anwendungstests (End-to-End)"
cyan "========================================"
echo ""

test_hauptmenue
test_ungueltige_menueeingabe
test_hilfe_flag
test_unbekanntes_argument
test_neue_partie_mensch
test_neue_partie_computer
test_setzzug
test_ungueltige_eingabe
test_besetztes_feld
test_hinweis
test_undo
test_undo_leer
test_speichern_fortsetzen
test_wiedergabe
test_statistik_leer
test_komplette_partie
test_ki_schwierigkeitsgrade
test_logging
test_fortsetzen_leer
test_wiedergabe_leer

echo ""
cyan "========================================"
TOTAL=$((PASS + FAIL))
echo "Ergebnis: $PASS/$TOTAL bestanden"
if [ "$FAIL" -gt 0 ]; then
    red "$FAIL Test(s) fehlgeschlagen."
    exit 1
else
    green "Alle Tests bestanden."
    exit 0
fi
