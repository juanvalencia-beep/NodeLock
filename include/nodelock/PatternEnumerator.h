#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>

#include "nodelock/PatternGraph.h"

namespace nodelock {

using LengthCounts = std::array<std::uint64_t, kMaxPatternLength + 1>;

struct EnumerationStats {
    LengthCounts countsByLength{};
    std::uint64_t total = 0;
    std::uint64_t recursiveCalls = 0;
    std::uint64_t prunedMoves = 0;
};

// Depth-first search with backtracking. Candidates are tried in ascending
// order, so patterns of one length come out in lexicographic order.
class PatternEnumerator {
public:
    // Return false from the visitor to stop the search.
    using Visitor = std::function<bool(const std::vector<int>&)>;

    explicit PatternEnumerator(const PatternGraph& graph) : graph_(graph) {}

    EnumerationStats countAll() const;
    bool forEachPatternOfLength(int length, const Visitor& visit) const;

private:
    void countRecursive(std::vector<int>& path, VisitedSet& visited,
                        EnumerationStats& stats) const;
    bool visitRecursive(std::vector<int>& path, VisitedSet& visited, int targetLength,
                        const Visitor& visit) const;

    const PatternGraph& graph_;
};

}  // namespace nodelock
