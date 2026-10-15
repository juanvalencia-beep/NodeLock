#include <cstdint>
#include <functional>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../src/app/ConsoleUI.h"
#include "nodelock/NodeLockEngine.h"

using namespace nodelock;

namespace {

// Minimal test framework: CHECK records failures instead of aborting.
int g_checks = 0;
int g_failures = 0;

#define CHECK(condition)                                                          \
    do {                                                                          \
        ++g_checks;                                                               \
        if (!(condition)) {                                                       \
            ++g_failures;                                                         \
            std::cerr << "  FAILED: " #condition " (" << __FILE__ << ":" << __LINE__ \
                      << ")\n";                                                   \
        }                                                                         \
    } while (0)

#define CHECK_THROWS(expression, ExceptionType) \
    do {                                        \
        bool thrown = false;                    \
        try { (void)(expression); }             \
        catch (const ExceptionType&) { thrown = true; } \
        CHECK(thrown && #expression " should throw " #ExceptionType); \
    } while (0)

const NodeLockEngine& engine() {
    static const NodeLockEngine instance;
    return instance;
}

bool isValid(const std::string& text) {
    return engine().validator().validate(Pattern::parse(text)).valid();
}

ValidationResult validate(const std::string& text) {
    return engine().validator().validate(Pattern::parse(text));
}

void testGraph() {
    const PatternGraph& g = engine().graph();
    CHECK(g.nodes().size() == 9);
    CHECK(g.node(1).row == 0 && g.node(1).col == 0);
    CHECK(g.node(5).row == 1 && g.node(5).col == 1);
    CHECK(g.node(9).row == 2 && g.node(9).col == 2);
    CHECK_THROWS(g.node(0), std::out_of_range);
    CHECK_THROWS(g.node(10), std::out_of_range);

    CHECK(g.intermediate(1, 3) == 2 && g.intermediate(3, 1) == 2);
    CHECK(g.intermediate(1, 9) == 5 && g.intermediate(7, 3) == 5);
    CHECK(g.intermediate(4, 6) == 5 && g.intermediate(2, 8) == 5);
    CHECK(g.intermediate(1, 7) == 4 && g.intermediate(3, 9) == 6);
    CHECK(g.intermediate(1, 2) == 0);
    CHECK(g.intermediate(1, 6) == 0);
    CHECK(g.intermediate(1, 8) == 0);

    CHECK(g.conditionalNeighbors(1).size() == 3);
    CHECK(g.conditionalNeighbors(5).empty());
    CHECK(g.directNeighbors(5).size() == 8);
    CHECK(g.conditionalNeighbors(2).size() == 1);

    VisitedSet visited{};
    visited[1] = true;
    CHECK(!g.canMove(1, 3, visited));
    visited[2] = true;
    CHECK(g.canMove(1, 3, visited));
    CHECK(!g.canMove(1, 2, visited));
}

void testParsing() {
    const std::vector<int> expected{1, 2, 3, 6};
    CHECK(Pattern::parse("1-2-3-6").nodes() == expected);
    CHECK(Pattern::parse("1 2 3 6").nodes() == expected);
    CHECK(Pattern::parse("1,2,3,6").nodes() == expected);
    CHECK(Pattern::parse("1>2>3>6").nodes() == expected);
    CHECK(Pattern::parse("1236").nodes() == expected);
    CHECK(Pattern::parse(" 1236 ").toString() == "1-2-3-6");
    CHECK_THROWS(Pattern::parse(""), std::invalid_argument);
    CHECK_THROWS(Pattern::parse("   "), std::invalid_argument);
    CHECK_THROWS(Pattern::parse("12a4"), std::invalid_argument);
}

void testValidation() {
    CHECK(isValid("1-2-3-6"));
    CHECK(isValid("1-6-3-4"));
    CHECK(isValid("2-1-3-6"));
    CHECK(isValid("5-1-9-6"));
    CHECK(isValid("1-2-3-4-5-6-7-8-9"));

    CHECK(validate("1-2-3").has(ValidationError::TooShort));
    CHECK(validate("1-2-3-4-5-6-7-8-9-1").has(ValidationError::TooLong));
    CHECK(validate("1-2-1-4").has(ValidationError::RepeatedNode));
    CHECK(validate("1-3-6-9").has(ValidationError::SkipsUnvisitedNode));
    CHECK(validate("1-9-6-3").has(ValidationError::SkipsUnvisitedNode));
    CHECK(validate("0-1-2-3").has(ValidationError::NodeOutOfRange));

    const ValidationResult multi = validate("1-3-1");
    CHECK(multi.has(ValidationError::TooShort));
    CHECK(multi.has(ValidationError::SkipsUnvisitedNode));
    CHECK(multi.has(ValidationError::RepeatedNode));
    CHECK(!multi.issues.front().message.empty());
}

void testCounts() {
    const EnumerationStats& s = engine().stats();
    CHECK(s.countsByLength[4] == 1624);
    CHECK(s.countsByLength[5] == 7152);
    CHECK(s.countsByLength[6] == 26016);
    CHECK(s.countsByLength[7] == 72912);
    CHECK(s.countsByLength[8] == 140704);
    CHECK(s.countsByLength[9] == 140704);
    CHECK(s.total == 389112);
    CHECK(s.total == NodeLockEngine::kExpectedTotal);
    CHECK(s.recursiveCalls > s.total);
}

void testEnumerationAgreesWithValidator() {
    std::uint64_t generated = 0;
    std::uint64_t invalid = 0;
    for (int len = kMinPatternLength; len <= kMaxPatternLength; ++len) {
        std::set<std::vector<int>> unique;
        engine().enumerator().forEachPatternOfLength(len, [&](const std::vector<int>& p) {
            ++generated;
            if (!engine().validator().validate(Pattern(p)).valid()) ++invalid;
            unique.insert(p);
            return true;
        });
        CHECK(unique.size() == engine().stats().countsByLength[static_cast<std::size_t>(len)]);
    }
    CHECK(generated == 389112);
    CHECK(invalid == 0);
}

void testEnumerationOrderAndEarlyStop() {
    std::vector<std::vector<int>> firstThree;
    const bool completed =
        engine().enumerator().forEachPatternOfLength(4, [&](const std::vector<int>& p) {
            firstThree.push_back(p);
            return firstThree.size() < 3;
        });
    CHECK(!completed);
    CHECK(firstThree.size() == 3);
    CHECK(firstThree[0] == (std::vector<int>{1, 2, 3, 4}));
    CHECK(firstThree[1] == (std::vector<int>{1, 2, 3, 5}));
    CHECK(firstThree[2] == (std::vector<int>{1, 2, 3, 6}));
    CHECK_THROWS(engine().enumerator().forEachPatternOfLength(3, [](const std::vector<int>&) {
        return true;
    }), std::invalid_argument);
}

void testSimulation() {
    const SearchSimulator& sim = engine().simulator();

    SimulationResult r = sim.run(Pattern::parse("1-2-3-4"), SearchOrder::ShortestFirst);
    CHECK(r.found && r.attempts == 1 && r.totalCandidates == 389112);

    r = sim.run(Pattern::parse("1-2-3-6"), SearchOrder::ShortestFirst);
    CHECK(r.found && r.attempts == 3);

    r = sim.run(Pattern::parse("1-2-3-4-5"), SearchOrder::ShortestFirst);
    CHECK(r.found && r.attempts == 1625);

    r = sim.run(Pattern::parse("1-2-3-4"), SearchOrder::LongestFirst);
    CHECK(r.found && r.attempts == 387489);

    r = sim.run(Pattern::parse("1-2-3-4-5-6-7-8-9"), SearchOrder::LongestFirst);
    CHECK(r.found && r.attempts == 1);

    CHECK_THROWS(sim.run(Pattern::parse("1-3-5-7"), SearchOrder::ShortestFirst),
                 std::invalid_argument);

    const std::vector<Pattern> first = sim.firstAttempts(SearchOrder::LongestFirst, 2);
    CHECK(first.size() == 2);
    CHECK(first[0].toString() == "1-2-3-4-5-6-7-8-9");
    CHECK(sim.firstAttempts(SearchOrder::ShortestFirst, 0).empty());
}

void testAnalysis() {
    const PatternAnalyzer& an = engine().analyzer();

    PatternAnalysis a = an.analyze(Pattern::parse("1-2-3-6"));
    CHECK(a.length == 4 && a.segments == 3);
    CHECK(a.straightMoves == 3 && a.diagonalMoves == 0 && a.knightMoves == 0);
    CHECK(a.directionChanges == 1);
    CHECK(a.crossings == 0 && a.passOvers == 0);
    CHECK(a.start == StartPosition::Corner);
    CHECK(a.totalDistance > 2.99 && a.totalDistance < 3.01);
    CHECK(a.patternsOfSameLength == 1624);
    CHECK(a.complexityLabel == "Low");

    a = an.analyze(Pattern::parse("1-6-3-4"));
    CHECK(a.knightMoves == 2);
    CHECK(a.crossings == 1);

    a = an.analyze(Pattern::parse("2-1-3-6"));
    CHECK(a.passOvers == 1);
    CHECK(a.start == StartPosition::Edge);
    CHECK(a.directionChanges == 2);

    a = an.analyze(Pattern::parse("5-1-9-6"));
    CHECK(a.start == StartPosition::Center);

    a = an.analyze(Pattern::parse("1-2-3-6-5-4-7-8-9"));
    CHECK(a.length == 9);
    CHECK(a.complexityLabel != "High");

    CHECK_THROWS(an.analyze(Pattern::parse("1-3-2-4")), std::invalid_argument);
    CHECK(!PatternAnalyzer::limitations().empty());
}

void testConsoleUI() {
    // Main menu -> analyze invalid -> retry valid -> simulate -> details -> back -> exit.
    std::istringstream in("1\n1-3-6-9\n1\n2\n2984\n1\n3\n0\n9\n0\n");
    std::ostringstream out;
    ConsoleUI ui(engine(), in, out);
    ui.run();
    const std::string text = out.str();
    CHECK(text.find("NODELOCK SECURITY") != std::string::npos);
    CHECK(text.find("[4] [5] [6]") != std::string::npos);
    CHECK(text.find("Validation:       INVALID") != std::string::npos);
    CHECK(text.find("passes over node 2") != std::string::npos);
    CHECK(text.find("This option needs a valid pattern") != std::string::npos);
    CHECK(text.find("Pattern:          2 -> 9 -> 8 -> 4") != std::string::npos);
    CHECK(text.find("Validation:       VALID") != std::string::npos);
    CHECK(text.find("Length:         Short") != std::string::npos);
    CHECK(text.find("Found at attempt") != std::string::npos);
    CHECK(text.find("Structural score") != std::string::npos);
    CHECK(text.find("Unknown option '9'") != std::string::npos);
    CHECK(text.find("Bye.") != std::string::npos);

    std::istringstream eof("1\n");
    std::ostringstream out2;
    ConsoleUI ui2(engine(), eof, out2);
    ui2.run();
    CHECK(out2.str().find("Bye.") != std::string::npos);
}

}  // namespace

int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> tests = {
        {"graph", testGraph},
        {"parsing", testParsing},
        {"validation", testValidation},
        {"backtracking counts", testCounts},
        {"enumeration agrees with validator", testEnumerationAgreesWithValidator},
        {"enumeration order and early stop", testEnumerationOrderAndEarlyStop},
        {"search simulation", testSimulation},
        {"pattern analysis", testAnalysis},
        {"console UI", testConsoleUI},
    };
    for (const auto& [name, test] : tests) {
        const int before = g_failures;
        try {
            test();
        } catch (const std::exception& e) {
            ++g_failures;
            std::cerr << "  UNEXPECTED EXCEPTION: " << e.what() << "\n";
        }
        std::cout << (g_failures == before ? "[PASS] " : "[FAIL] ") << name << "\n";
    }
    std::cout << "\n" << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    return g_failures == 0 ? 0 : 1;
}
