#include "nodelock/SearchSimulator.h"

#include <stdexcept>

namespace nodelock {

std::vector<int> SearchSimulator::lengthsInOrder(SearchOrder order) {
    if (order == SearchOrder::LongestFirst) return {9, 8, 7, 6, 5, 4};
    return {4, 5, 6, 7, 8, 9};
}

std::string SearchSimulator::describe(SearchOrder order) {
    switch (order) {
        case SearchOrder::ShortestFirst:
            return "Shortest-first: lengths 4->9, lexicographic order inside each length";
        case SearchOrder::LongestFirst:
            return "Longest-first: lengths 9->4, lexicographic order inside each length";
    }
    return "Unknown order";
}

SimulationResult SearchSimulator::run(const Pattern& target, SearchOrder order) const {
    const ValidationResult validation = validator_.validate(target);
    if (!validation.valid()) {
        throw std::invalid_argument("Cannot simulate an invalid pattern: " +
                                    validation.issues.front().message);
    }

    SimulationResult result;
    result.order = order;
    const std::vector<int>& wanted = target.nodes();

    for (int length : lengthsInOrder(order)) {
        enumerator_.forEachPatternOfLength(length, [&](const std::vector<int>& candidate) {
            ++result.totalCandidates;
            if (!result.found) {
                ++result.attempts;
                if (candidate == wanted) result.found = true;
            }
            return true;
        });
    }
    return result;
}

std::vector<Pattern> SearchSimulator::firstAttempts(SearchOrder order, std::size_t count) const {
    std::vector<Pattern> out;
    if (count == 0) return out;
    for (int length : lengthsInOrder(order)) {
        const bool completed = enumerator_.forEachPatternOfLength(
            length, [&](const std::vector<int>& candidate) {
                out.emplace_back(candidate);
                return out.size() < count;
            });
        if (!completed) break;
    }
    return out;
}

}  // namespace nodelock
