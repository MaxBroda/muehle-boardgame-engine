#include "CommandLine.h"

namespace muehle {

namespace {

// Praefix-Vergleich: beginnt text mit prefix?
bool startsWith(const std::string& text, const std::string& prefix) {
    return text.size() >= prefix.size() &&
           text.compare(0, prefix.size(), prefix) == 0;
}

} // namespace

CommandLineOptions parseCommandLine(const std::vector<std::string>& args) {
    CommandLineOptions options;
    const std::string logWithPath = "--log=";
    for (const std::string& arg : args) {
        if (arg == "-h" || arg == "--help") {
            options.help = true;
        } else if (arg == "--log") {
            options.logging = true;
        } else if (startsWith(arg, logWithPath)) {
            options.logging = true;
            std::string path = arg.substr(logWithPath.size());
            // Ein leerer Pfad (--log=) faellt auf den automatischen Namen zurueck.
            if (!path.empty()) {
                options.logPath = path;
            }
        } else {
            // Erstes unbekanntes Argument merken, Rest ignorieren.
            if (!options.unknownOption) {
                options.unknownOption = true;
                options.unknown = arg;
            }
        }
    }
    return options;
}

} // namespace muehle
