#pragma once

#include <string>
#include <vector>

namespace muehle {

// Ausgewertete Kommandozeile. Bewusst klein: das Programm kennt nur das
// optionale Logging-Flag und eine Hilfe.
struct CommandLineOptions {
    bool logging = false;        // erweiterte Protokollierung gewuenscht?
    std::string logPath;         // Zieldatei des Ereignislogs; leer = automatisch
    bool help = false;           // Hilfe anzeigen und beenden?
    bool unknownOption = false;  // unbekanntes Argument gefunden?
    std::string unknown;         // das erste unbekannte Argument
};

// Wertet die Argumente (ohne den Programmnamen) aus. Erkennt:
//   --log            erweiterte Protokollierung, Dateiname wird automatisch
//                    vergeben (logPath bleibt leer)
//   --log=<datei>    erweiterte Protokollierung in eine eigene Datei
//   -h, --help       Hilfe
// Alles andere wird als unbekanntes Argument vermerkt. Als freie, vom
// Dateisystem unabhaengige Funktion gehalten, damit sie leicht testbar ist.
CommandLineOptions parseCommandLine(const std::vector<std::string>& args);

} // namespace muehle
