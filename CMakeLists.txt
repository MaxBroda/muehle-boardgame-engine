# Mühle-Engine - Build-Konfiguration
# Modernes, target-basiertes CMake. Baut drei Dinge:
#   1. muehle_lib   - die Spiellogik als statische Bibliothek (von App und Tests genutzt)
#   2. muehle       - die ausführbare Konsolenanwendung
#   3. muehle_tests - das separate Testbinary mit dem eigenen Test-Framework

cmake_minimum_required(VERSION 3.16)

project(MuehleEngine
    VERSION 0.1.0
    DESCRIPTION "Spiele-Engine fuer das Brettspiel Muehle"
    LANGUAGES CXX)

# C++17, verbindlich, ohne Compiler-spezifische Erweiterungen.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ohne explizite Wahl bauen wir im Debug-Modus (sinnvoll waehrend der Entwicklung).
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Warnungen einschalten. Wir wollen sauberen Code, also lieber viel sehen.
add_compile_options(-Wall -Wextra)

# --- 1. Bibliothek mit der gesamten Spiellogik -------------------------------
# Alle Quellen ausser main.cpp landen hier, damit die Tests dieselbe Logik
# verwenden wie die Anwendung.
add_library(muehle_lib
    src/Board.cpp
    src/Player.cpp
    src/Game.cpp
    src/MoveLogger.cpp
    src/InputParser.cpp
    src/ConsoleRenderer.cpp)

# Die Header liegen unter include/ und sind oeffentlich, damit App und Tests
# sie ueber #include "Board.h" finden.
target_include_directories(muehle_lib PUBLIC include)

# --- 2. Konsolenanwendung ----------------------------------------------------
add_executable(muehle src/main.cpp)
target_link_libraries(muehle PRIVATE muehle_lib)

# --- 3. Testbinary -----------------------------------------------------------
add_executable(muehle_tests tests/test_main.cpp)
target_link_libraries(muehle_tests PRIVATE muehle_lib)
# Das Test-Framework ist ein einzelner Header unter tests/.
target_include_directories(muehle_tests PRIVATE tests)

# Tests bei CTest registrieren, damit "ctest" sie ausfuehren kann.
enable_testing()
add_test(NAME unit_tests COMMAND muehle_tests)
