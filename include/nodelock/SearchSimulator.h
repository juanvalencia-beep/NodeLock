#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "nodelock/Pattern.h"
#include "nodelock/PatternEnumerator.h"
#include "nodelock/PatternValidator.h"

namespace nodelock {

// Lexicographic order inside each length.
enum class SearchOrder {
    ShortestFirst,  // lengths 4..9
    LongestFirst,   // lengths 9..4
};

struct SimulationResult {
    bool found = false;
    std::uint64_t attempts = 0;
    std::uint64_t totalCandidates = 0;
    SearchOrder order = SearchOrder::ShortestFirst;
};

// Local, in-memory simulation only. No device or system is involved.
class SearchSimulator {
public:
    SearchSimulator(const PatternEnumerator& enumerator, const PatternValidator& validator)
        : enumerator_(enumerator), validator_(validator) {}

    SimulationResult run(const Pattern& target, SearchOrder order) const;  // throws if invalid
    std::vector<Pattern> firstAttempts(SearchOrder order, std::size_t count) const;

    static std::string describe(SearchOrder order);
    static std::vector<int> lengthsInOrder(SearchOrder order);

private:
    const PatternEnumerator& enumerator_;
    const PatternValidator& validator_;
};

}  // namespace nodelock
