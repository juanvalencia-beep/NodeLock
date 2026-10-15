#include "nodelock/PatternAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <stdexcept>

namespace nodelock {

namespace {

// Sign of the cross product: >0 counter-clockwise, <0 clockwise, 0 collinear.
int orientation(const Node& a, const Node& b, const Node& c) {
    const int value = (b.col - a.col) * (c.row - a.row) - (b.row - a.row) * (c.col - a.col);
    return (value > 0) - (value < 0);
}

// True when the segments cross at a point interior to both.
bool properlyIntersect(const Node& p1, const Node& p2, const Node& q1, const Node& q2) {
    const int o1 = orientation(p1, p2, q1);
    const int o2 = orientation(p1, p2, q2);
    const int o3 = orientation(q1, q2, p1);
    const int o4 = orientation(q1, q2, p2);
    return o1 * o2 < 0 && o3 * o4 < 0;
}

int capAt(int value, int maxValue) { return std::min(value, maxValue); }

}  // namespace

std::string PatternAnalyzer::toString(StartPosition start) {
    switch (start) {
        case StartPosition::Corner: return "corner";
        case StartPosition::Edge: return "edge";
        case StartPosition::Center: return "center";
    }
    return "unknown";
}

std::vector<std::string> PatternAnalyzer::limitations() {
    return {
        "The score is a structural heuristic, not a measure of real-world security.",
        "Real users choose patterns non-uniformly; attackers try popular shapes first.",
        "The analysis ignores smudge attacks, shoulder surfing and device lockout policies.",
        "The position in one enumeration order says nothing about other attack orders.",
    };
}

int PatternAnalyzer::countCrossings(const std::vector<int>& nodes) const {
    int crossings = 0;
    const std::size_t segments = nodes.size() - 1;
    for (std::size_t i = 0; i < segments; ++i) {
        for (std::size_t j = i + 2; j < segments; ++j) {
            if (properlyIntersect(graph_.node(nodes[i]), graph_.node(nodes[i + 1]),
                                  graph_.node(nodes[j]), graph_.node(nodes[j + 1]))) {
                ++crossings;
            }
        }
    }
    return crossings;
}

PatternAnalysis PatternAnalyzer::analyze(const Pattern& pattern) const {
    const ValidationResult validation = validator_.validate(pattern);
    if (!validation.valid()) {
        throw std::invalid_argument("Cannot analyze an invalid pattern: " +
                                    validation.issues.front().message);
    }

    const std::vector<int>& nodes = pattern.nodes();
    PatternAnalysis a;
    a.length = static_cast<int>(nodes.size());
    a.segments = a.length - 1;

    const Node& first = graph_.node(nodes.front());
    const bool rowEdge = first.row == 0 || first.row == kGridSize - 1;
    const bool colEdge = first.col == 0 || first.col == kGridSize - 1;
    a.start = (rowEdge && colEdge) ? StartPosition::Corner
            : (rowEdge || colEdge) ? StartPosition::Edge
                                   : StartPosition::Center;

    int minRow = kGridSize, maxRow = -1, minCol = kGridSize, maxCol = -1;
    int prevDirRow = 0, prevDirCol = 0;
    VisitedSet visited{};
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const Node& cur = graph_.node(nodes[i]);
        minRow = std::min(minRow, cur.row); maxRow = std::max(maxRow, cur.row);
        minCol = std::min(minCol, cur.col); maxCol = std::max(maxCol, cur.col);

        if (i > 0) {
            const Node& prev = graph_.node(nodes[i - 1]);
            const int dr = cur.row - prev.row;
            const int dc = cur.col - prev.col;
            const int adr = std::abs(dr), adc = std::abs(dc);
            a.totalDistance += std::sqrt(static_cast<double>(dr * dr + dc * dc));

            if (adr == 0 || adc == 0) ++a.straightMoves;
            else if (adr == adc) ++a.diagonalMoves;
            else ++a.knightMoves;

            if (graph_.intermediate(prev.id, cur.id) != 0) ++a.passOvers;

            const int g = std::gcd(adr, adc);
            const int dirRow = dr / g, dirCol = dc / g;
            if (i > 1 && (dirRow != prevDirRow || dirCol != prevDirCol)) ++a.directionChanges;
            prevDirRow = dirRow; prevDirCol = dirCol;
        }
        visited[cur.id] = true;
    }
    a.boundingWidth = maxCol - minCol + 1;
    a.boundingHeight = maxRow - minRow + 1;
    a.crossings = countCrossings(nodes);

    a.patternsOfSameLength = counts_[static_cast<std::size_t>(a.length)];
    if (a.patternsOfSameLength > 0) {
        a.bitsOfSameLength = std::log2(static_cast<double>(a.patternsOfSameLength));
    }

    // Capped contributions, so no single feature (e.g. length) dominates.
    int score = 0;
    score += (a.length - kMinPatternLength) * 6;
    score += capAt(a.directionChanges, 5) * 5;
    score += capAt(a.knightMoves, 3) * 5;
    score += capAt(a.crossings, 3) * 5;
    score += capAt(a.passOvers, 2) * 5;
    if (a.start != StartPosition::Corner) score += 5;
    a.complexityScore = std::min(score, 100);
    a.complexityLabel = a.complexityScore < 35 ? "Low"
                      : a.complexityScore < 65 ? "Medium"
                                               : "High";

    if (a.start == StartPosition::Corner) {
        a.observations.push_back(
            "Starts at a corner. Studies of user-chosen patterns report that corners "
            "(especially the top-left) are the most common starting points.");
    }
    if (a.directionChanges <= 1 && a.knightMoves == 0 && a.crossings == 0) {
        a.observations.push_back(
            "Simple shape (a line, an L or similar): easy to guess and to observe.");
    }
    if (a.knightMoves > 0) {
        a.observations.push_back("Uses knight-like moves, which tend to be less common in user-chosen patterns.");
    }
    if (a.crossings > 0) {
        a.observations.push_back("Segments cross each other, which makes the shape less obvious.");
    }
    if (a.passOvers > 0) {
        a.observations.push_back("Passes over already visited nodes, a less common move.");
    }
    if (a.length >= 8 && a.complexityLabel == "Low") {
        a.observations.push_back(
            "Long but structurally simple: length alone does not make a pattern secure.");
    }
    if (a.boundingWidth < kGridSize || a.boundingHeight < kGridSize) {
        a.observations.push_back("Does not cover the whole grid width/height.");
    }
    return a;
}

}  // namespace nodelock
