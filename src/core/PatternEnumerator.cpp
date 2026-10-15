#include "nodelock/PatternEnumerator.h"

#include <stdexcept>

namespace nodelock {

EnumerationStats PatternEnumerator::countAll() const {
    EnumerationStats stats;
    std::vector<int> path;
    path.reserve(kMaxPatternLength);
    VisitedSet visited{};

    for (int start = 1; start <= kNodeCount; ++start) {
        path.push_back(start);
        visited[start] = true;
        countRecursive(path, visited, stats);
        visited[start] = false;
        path.pop_back();
    }

    for (int len = kMinPatternLength; len <= kMaxPatternLength; ++len) {
        stats.total += stats.countsByLength[static_cast<std::size_t>(len)];
    }
    return stats;
}

void PatternEnumerator::countRecursive(std::vector<int>& path, VisitedSet& visited,
                                       EnumerationStats& stats) const {
    ++stats.recursiveCalls;
    const std::size_t depth = path.size();

    // Every path with 4 or more nodes is a valid pattern.
    if (depth >= static_cast<std::size_t>(kMinPatternLength)) {
        ++stats.countsByLength[depth];
    }
    if (depth == static_cast<std::size_t>(kMaxPatternLength)) return;  // base case

    const int last = path.back();
    for (int next = 1; next <= kNodeCount; ++next) {
        if (!graph_.canMove(last, next, visited)) {
            if (!visited[next]) ++stats.prunedMoves;
            continue;  // prune
        }
        path.push_back(next);  // choose
        visited[next] = true;
        countRecursive(path, visited, stats);  // explore
        visited[next] = false;  // backtrack
        path.pop_back();
    }
}

bool PatternEnumerator::forEachPatternOfLength(int length, const Visitor& visit) const {
    if (length < kMinPatternLength || length > kMaxPatternLength) {
        throw std::invalid_argument("Pattern length must be between 4 and 9");
    }
    std::vector<int> path;
    path.reserve(static_cast<std::size_t>(length));
    VisitedSet visited{};

    for (int start = 1; start <= kNodeCount; ++start) {
        path.push_back(start);
        visited[start] = true;
        const bool keepGoing = visitRecursive(path, visited, length, visit);
        visited[start] = false;
        path.pop_back();
        if (!keepGoing) return false;
    }
    return true;
}

bool PatternEnumerator::visitRecursive(std::vector<int>& path, VisitedSet& visited,
                                       int targetLength, const Visitor& visit) const {
    if (static_cast<int>(path.size()) == targetLength) {
        return visit(path);
    }
    const int last = path.back();
    for (int next = 1; next <= kNodeCount; ++next) {
        if (!graph_.canMove(last, next, visited)) continue;
        path.push_back(next);
        visited[next] = true;
        const bool keepGoing = visitRecursive(path, visited, targetLength, visit);
        visited[next] = false;
        path.pop_back();
        if (!keepGoing) return false;
    }
    return true;
}

}  // namespace nodelock
