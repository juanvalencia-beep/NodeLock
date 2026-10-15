#include "ConsoleUI.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace nodelock {

namespace {


std::string withThousands(std::uint64_t value) {
    std::string digits = std::to_string(value);
    for (int pos = static_cast<int>(digits.size()) - 3; pos > 0; pos -= 3) {
        digits.insert(static_cast<std::size_t>(pos), ",");
    }
    return digits;
}

constexpr int kWidth = 48;
const std::string kDoubleRule(kWidth, '=');
const std::string kSingleRule(kWidth, '-');

std::string centered(const std::string& text) {
    const int pad = (kWidth - static_cast<int>(text.size())) / 2;
    return std::string(static_cast<std::size_t>(pad > 0 ? pad : 0), ' ') + text;
}

std::string arrowPath(const Pattern& pattern) {
    std::string out;
    for (std::size_t i = 0; i < pattern.length(); ++i) {
        if (i > 0) out += " -> ";
        out += std::to_string(pattern.nodes()[i]);
    }
    return out;
}

const char* lengthLabel(std::size_t length) {
    if (length <= 5) return "Short";
    if (length <= 7) return "Medium";
    return "Long";
}

}  // namespace

bool ConsoleUI::readLine(const std::string& prompt, std::string& line) {
    out_ << prompt;
    out_.flush();
    return static_cast<bool>(std::getline(in_, line));
}

void ConsoleUI::printBanner() const {
    out_ << "\n" << kDoubleRule << "\n"
         << centered("NODELOCK SECURITY") << "\n"
         << kDoubleRule << "\n\n"
         << centered("PATTERN SECURITY ANALYZER") << "\n\n";
    for (int row = 0; row < kGridSize; ++row) {
        std::string line;
        for (int col = 0; col < kGridSize; ++col) {
            if (col > 0) line += ' ';
            line += "[" + std::to_string(row * kGridSize + col + 1) + "]";
        }
        out_ << centered(line) << "\n";
    }
    out_ << "\n";
}

void ConsoleUI::run() {
    std::string choice;
    while (true) {
        printBanner();
        out_ << "[1] Analyze a pattern\n"
             << "[2] Count all valid patterns\n"
             << "[3] Show the graph\n"
             << "[0] Exit\n\n";
        if (!readLine("Choose an option: ", choice)) break;

        if (choice == "0") break;
        if (choice == "1") {
            if (!patternScreen()) break;
            continue;
        }
        if (choice == "2") { showCounts(); continue; }
        if (choice == "3") { showGraph(); continue; }
        out_ << "Unknown option '" << choice << "'. Please type 0-3.\n";
    }
    out_ << "Bye.\n";
}

void ConsoleUI::printPatternReport(const std::string& text, Pattern& pattern, bool& valid) const {
    valid = false;
    out_ << "\n" << kSingleRule << "\n"
         << centered("PATTERN ANALYSIS") << "\n"
         << kSingleRule << "\n\n";

    try {
        pattern = Pattern::parse(text);
    } catch (const std::invalid_argument& e) {
        out_ << "Input error:      " << e.what() << "\n\n" << kSingleRule << "\n";
        return;
    }

    const ValidationResult result = engine_.validator().validate(pattern);
    valid = result.valid();
    out_ << "Pattern:          " << arrowPath(pattern) << "\n"
         << "Validation:       " << (valid ? "VALID" : "INVALID") << "\n";
    for (const ValidationIssue& issue : result.issues) {
        out_ << "  - " << issue.message << "\n";
    }
    out_ << "Pattern length:   " << pattern.length() << " nodes\n\n";

    if (!valid) {
        out_ << "Security analysis: not available for an invalid pattern.\n\n"
             << kSingleRule << "\n";
        return;
    }

    const PatternAnalysis a = engine_.analyzer().analyze(pattern);
    out_ << "Security analysis:\n"
         << "  Length:         " << lengthLabel(pattern.length()) << "\n"
         << "  Complexity:     " << a.complexityLabel << " (" << a.complexityScore
         << "/100, structural heuristic)\n"
         << "  Predictability: Heuristic assessment\n"
         << "    Start:        " << PatternAnalyzer::toString(a.start) << "\n"
         << "    Moves:        " << a.straightMoves << " straight, " << a.diagonalMoves
         << " diagonal, " << a.knightMoves << " knight\n"
         << "    Turns:        " << a.directionChanges << ", crossings: " << a.crossings << "\n\n"
         << "Note: a valid pattern is not necessarily a secure one.\n\n"
         << kSingleRule << "\n";
}

bool ConsoleUI::patternScreen() {
    std::string text;
    if (!readLine("Enter your pattern: ", text)) return false;

    while (true) {
        Pattern pattern;
        bool valid = false;
        printPatternReport(text, pattern, valid);

        std::string choice;
        while (true) {
            out_ << "\n[1] Start search simulation\n"
                 << "[2] Enter another pattern\n"
                 << "[3] Show detailed analysis\n"
                 << "[0] Back\n\n";
            if (!readLine("Choose an option: ", choice)) return false;

            if (choice == "0") return true;
            if (choice == "2") break;
            if (choice == "1" || choice == "3") {
                if (!valid) {
                    out_ << "This option needs a valid pattern. Choose [2] to enter another one.\n";
                } else if (choice == "1") {
                    showSimulation(pattern.toString());
                } else {
                    showAnalysis(pattern.toString());
                }
                continue;
            }
            out_ << "Unknown option '" << choice << "'. Please type 0-3.\n";
        }

        out_ << "\n";
        if (!readLine("Enter your pattern: ", text)) return false;
    }
}

void ConsoleUI::showGraph() const {
    const PatternGraph& graph = engine_.graph();
    out_ << "\nGrid (id @ row,col):\n";
    for (int row = 0; row < kGridSize; ++row) {
        out_ << "   ";
        for (int col = 0; col < kGridSize; ++col) {
            const Node& n = graph.node(row * kGridSize + col + 1);
            out_ << "[" << n.id << " @ " << n.row << "," << n.col << "]  ";
        }
        out_ << "\n";
    }

    out_ << "\nGraph (adjacency): direct edges | conditional edges (needs node)\n";
    for (const Node& n : graph.nodes()) {
        out_ << "  " << n.id << ": direct {";
        bool firstItem = true;
        for (int other : graph.directNeighbors(n.id)) {
            out_ << (firstItem ? "" : ",") << other;
            firstItem = false;
        }
        out_ << "}  conditional {";
        firstItem = true;
        for (int other : graph.conditionalNeighbors(n.id)) {
            out_ << (firstItem ? "" : ", ") << other << " via " << graph.intermediate(n.id, other);
            firstItem = false;
        }
        out_ << "}\n";
    }
}

void ConsoleUI::showCounts() const {
    const auto start = std::chrono::steady_clock::now();
    const EnumerationStats stats = engine_.enumerator().countAll();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    out_ << "\nValid patterns by length (recursive backtracking):\n";
    for (int len = kMinPatternLength; len <= kMaxPatternLength; ++len) {
        out_ << "  length " << len << ": " << std::setw(9)
             << withThousands(stats.countsByLength[static_cast<std::size_t>(len)]) << "\n";
    }
    out_ << "  TOTAL    : " << std::setw(9) << withThousands(stats.total) << "\n"
         << "  Expected : " << std::setw(9) << withThousands(NodeLockEngine::kExpectedTotal)
         << (stats.total == NodeLockEngine::kExpectedTotal ? "  -> MATCH" : "  -> MISMATCH!")
         << "\n"
         << "  Recursive calls: " << withThousands(stats.recursiveCalls)
         << ", pruned moves: " << withThousands(stats.prunedMoves)
         << ", time: " << elapsed.count() << " ms\n";
}

bool ConsoleUI::parseValid(const std::string& text, Pattern& pattern) const {
    try {
        pattern = Pattern::parse(text);
    } catch (const std::invalid_argument& e) {
        out_ << "Input error: " << e.what() << "\n";
        return false;
    }
    const ValidationResult result = engine_.validator().validate(pattern);
    if (result.valid()) return true;
    out_ << "Pattern " << pattern.toString() << " is INVALID:\n";
    for (const ValidationIssue& issue : result.issues) {
        out_ << "  - " << issue.message << "\n";
    }
    return false;
}

void ConsoleUI::printPatternGrid(const Pattern& pattern) const {
    int step[kNodeCount + 1] = {};
    for (std::size_t i = 0; i < pattern.length(); ++i) {
        step[pattern.nodes()[i]] = static_cast<int>(i + 1);
    }
    out_ << "   Node ids      Drawing order\n";
    for (int row = 0; row < kGridSize; ++row) {
        out_ << "   ";
        for (int col = 0; col < kGridSize; ++col) out_ << row * kGridSize + col + 1 << "  ";
        out_ << "       ";
        for (int col = 0; col < kGridSize; ++col) {
            const int s = step[row * kGridSize + col + 1];
            if (s == 0) out_ << ".  ";
            else out_ << s << "  ";
        }
        out_ << "\n";
    }
}

bool ConsoleUI::showValidation(const std::string& text) const {
    Pattern pattern;
    if (!parseValid(text, pattern)) return false;
    out_ << "Pattern " << pattern.toString() << " is VALID (" << pattern.length()
         << " nodes).\n";
    printPatternGrid(pattern);
    out_ << "Note: valid only means it follows the rules; it says nothing about security.\n";
    return true;
}

bool ConsoleUI::showSimulation(const std::string& text) const {
    Pattern pattern;
    if (!parseValid(text, pattern)) return false;

    out_ << "\nLocal search simulation for test pattern " << pattern.toString() << "\n"
         << "(in-memory enumeration only; no device, account or system is contacted)\n";

    for (SearchOrder order : {SearchOrder::ShortestFirst, SearchOrder::LongestFirst}) {
        const SimulationResult r = engine_.simulator().run(pattern, order);
        out_ << "\n* " << SearchSimulator::describe(order) << "\n  First attempts: ";
        for (const Pattern& p : engine_.simulator().firstAttempts(order, 3)) {
            out_ << p.toString() << ", ";
        }
        out_ << "...\n";
        if (r.found) {
            const double percent = 100.0 * static_cast<double>(r.attempts) /
                                   static_cast<double>(r.totalCandidates);
            out_ << "  Found at attempt " << withThousands(r.attempts) << " of "
                 << withThousands(r.totalCandidates) << " (";
            if (percent < 0.01) {
                out_ << "<0.01";
            } else {
                out_ << std::fixed << std::setprecision(2) << percent;
                out_.unsetf(std::ios::floatfield);
            }
            out_ << "% of the space)\n";
        } else {
            out_ << "  Not found (unexpected for a valid pattern).\n";
        }
    }
    out_ << "\nThe attempt number depends entirely on the search ORDER. The same pattern\n"
            "can be found very early or very late; this is not a security rating.\n";
    return true;
}

bool ConsoleUI::showAnalysis(const std::string& text) const {
    Pattern pattern;
    if (!parseValid(text, pattern)) return false;
    const PatternAnalysis a = engine_.analyzer().analyze(pattern);

    out_ << "\nAnalysis of " << pattern.toString() << "\n";
    printPatternGrid(pattern);
    out_ << std::fixed << std::setprecision(2)
         << "  Validity            : VALID\n"
         << "  Length              : " << a.length << " nodes, " << a.segments << " segments\n"
         << "  Total distance      : " << a.totalDistance << " grid units\n"
         << "  Moves               : " << a.straightMoves << " straight, " << a.diagonalMoves
         << " diagonal, " << a.knightMoves << " knight\n"
         << "  Direction changes   : " << a.directionChanges << "\n"
         << "  Crossings           : " << a.crossings << "\n"
         << "  Pass-overs          : " << a.passOvers << "\n"
         << "  Start position      : " << PatternAnalyzer::toString(a.start) << "\n"
         << "  Bounding box        : " << a.boundingWidth << " x " << a.boundingHeight << "\n"
         << "  Patterns this length: " << withThousands(a.patternsOfSameLength) << " (~"
         << a.bitsOfSameLength << " bits if chosen uniformly at random)\n"
         << "  Structural score    : " << a.complexityScore << "/100 (" << a.complexityLabel
         << " estimated complexity)\n";
    out_.unsetf(std::ios::floatfield);

    if (!a.observations.empty()) {
        out_ << "  Observations:\n";
        for (const std::string& o : a.observations) out_ << "    - " << o << "\n";
    }
    out_ << "  Limitations:\n";
    for (const std::string& l : PatternAnalyzer::limitations()) out_ << "    - " << l << "\n";
    return true;
}

}  // namespace nodelock
