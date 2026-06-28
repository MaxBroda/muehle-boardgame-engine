#pragma once

// Minimales eigenes Test-Framework, ein einzelner Header, ohne externe
// Abhaengigkeiten. Es bietet:
//   - TEST(suite, name)  zum Definieren und automatischen Registrieren eines Tests
//   - ASSERT_TRUE / ASSERT_FALSE / ASSERT_EQ / ASSERT_THROWS  als Zusicherungen
//   - testing::runAll()  als Runner, der alle Tests ausfuehrt und das Ergebnis
//     im Format "X von Y Tests bestanden" ausgibt.
//
// Eine fehlgeschlagene Zusicherung wirft eine AssertionError. Der Runner faengt
// sie ab, zaehlt den Test als fehlgeschlagen und macht mit dem naechsten weiter.

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace testing {

// Wird bei einer fehlgeschlagenen Zusicherung geworfen.
struct AssertionError {
    std::string message;
};

// Ein registrierter Testfall: Name plus auszufuehrende Funktion.
struct TestCase {
    std::string name;
    std::function<void()> fn;
};

// Globale Liste aller registrierten Tests. Als Funktion mit statischer
// Variable, damit die Reihenfolge der Initialisierung sicher ist.
inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

// Fuegt einen Test der Registrierung hinzu. Rueckgabewert nur, damit der Aufruf
// als statische Initialisierung verwendet werden kann.
inline int registerTest(const std::string& name, std::function<void()> fn) {
    registry().push_back({name, std::move(fn)});
    return 0;
}

// Fuehrt alle registrierten Tests aus. Liefert 0, wenn alle bestehen, sonst 1.
inline int runAll() {
    int passed = 0;
    int failed = 0;

    for (const auto& test : registry()) {
        try {
            test.fn();
            std::cout << "[ OK ] " << test.name << "\n";
            ++passed;
        } catch (const AssertionError& e) {
            std::cout << "[FAIL] " << test.name << " - " << e.message << "\n";
            ++failed;
        } catch (const std::exception& e) {
            std::cout << "[FAIL] " << test.name
                      << " - unerwartete Ausnahme: " << e.what() << "\n";
            ++failed;
        } catch (...) {
            std::cout << "[FAIL] " << test.name
                      << " - unbekannte Ausnahme\n";
            ++failed;
        }
    }

    std::cout << "\n" << passed << " von " << (passed + failed)
              << " Tests bestanden.\n";
    return failed == 0 ? 0 : 1;
}

} // namespace testing

// Definiert einen Test und registriert ihn automatisch. Verwendung:
//   TEST(Board, istAnfangsLeer) { ASSERT_EQ(...); }
#define TEST(suite, name)                                                      \
    static void suite##_##name();                                              \
    static int suite##_##name##_registered =                                   \
        ::testing::registerTest(#suite "." #name, suite##_##name);             \
    static void suite##_##name()

// Zusicherung: Bedingung muss wahr sein.
#define ASSERT_TRUE(cond)                                                      \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::ostringstream _os;                                            \
            _os << "ASSERT_TRUE fehlgeschlagen: " #cond " (Zeile "             \
                << __LINE__ << ")";                                            \
            throw ::testing::AssertionError{_os.str()};                        \
        }                                                                      \
    } while (0)

// Zusicherung: Bedingung muss falsch sein.
#define ASSERT_FALSE(cond) ASSERT_TRUE(!(cond))

// Zusicherung: zwei Werte muessen gleich sein (per operator==).
#define ASSERT_EQ(a, b)                                                        \
    do {                                                                       \
        auto _va = (a);                                                        \
        auto _vb = (b);                                                        \
        if (!(_va == _vb)) {                                                   \
            std::ostringstream _os;                                            \
            _os << "ASSERT_EQ fehlgeschlagen: " #a " == " #b " (Zeile "        \
                << __LINE__ << ")";                                            \
            throw ::testing::AssertionError{_os.str()};                        \
        }                                                                      \
    } while (0)

// Zusicherung: der Ausdruck muss eine Ausnahme des angegebenen Typs werfen.
#define ASSERT_THROWS(expr, ExceptionType)                                     \
    do {                                                                       \
        bool _thrown = false;                                                  \
        try {                                                                  \
            (void)(expr);                                                      \
        } catch (const ExceptionType&) {                                       \
            _thrown = true;                                                    \
        } catch (...) {                                                        \
            std::ostringstream _os;                                            \
            _os << "ASSERT_THROWS: falscher Ausnahmetyp bei " #expr " (Zeile " \
                << __LINE__ << ")";                                            \
            throw ::testing::AssertionError{_os.str()};                        \
        }                                                                      \
        if (!_thrown) {                                                        \
            std::ostringstream _os;                                            \
            _os << "ASSERT_THROWS: keine Ausnahme bei " #expr " (Zeile "       \
                << __LINE__ << ")";                                            \
            throw ::testing::AssertionError{_os.str()};                        \
        }                                                                      \
    } while (0)
