#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "nodelock/Pattern.h"
#include "nodelock/PatternEnumerator.h"
#include "nodelock/PatternGraph.h"
#include "nodelock/PatternValidator.h"

namespace nodelock {

enum class StartPosition { Corner, Edge, Center };

struct PatternAnalysis {
    int length = 0;
    int segments = 0;
    double totalDistance = 0.0;
    int straightMoves = 0;
    int diagonalMoves = 0;
    int knightMoves = 0;
    int directionChanges = 0;
    int crossings = 0;
    int passOvers = 0;
    StartPosition start = StartPosition::Corner;
    int boundingWidth = 0;
    int boundingHeight = 0;

    std::uint64_t patternsOfSameLength = 0;
    double bitsOfSameLength = 0.0;

    int complexityScore = 0;
    std::string complexityLabel;
    std::vector<std::string> observations;
};

// Heuristic only: a valid pattern is not necessarily a secure one.
class PatternAnalyzer {
public:
    PatternAnalyzer(const PatternGraph& graph, const PatternValidator& validator,
                    const LengthCounts& counts)
        : graph_(graph), validator_(validator), counts_(counts) {}

    PatternAnalysis analyze(const Pattern& pattern) const;  // throws if invalid

    static std::string toString(StartPosition start);

    static std::vector<std::string> limitations();

private:
    int countCrossings(const std::vector<int>& nodes) const;

    const PatternGraph& graph_;
    const PatternValidator& validator_;
    LengthCounts counts_;
};

}  // namespace nodelock
