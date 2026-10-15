#include "nodelock/PatternValidator.h"

#include <algorithm>

namespace nodelock {

bool ValidationResult::has(ValidationError code) const {
    return std::any_of(issues.begin(), issues.end(),
                       [code](const ValidationIssue& issue) { return issue.code == code; });
}

ValidationResult PatternValidator::validate(const Pattern& pattern) const {
    ValidationResult result;
    const std::vector<int>& nodes = pattern.nodes();
    const int length = static_cast<int>(nodes.size());

    if (length < kMinPatternLength) {
        result.issues.push_back({ValidationError::TooShort,
                                 "Pattern has " + std::to_string(length) +
                                     " node(s); at least 4 are required."});
    }
    if (length > kMaxPatternLength) {
        result.issues.push_back({ValidationError::TooLong,
                                 "Pattern has " + std::to_string(length) +
                                     " nodes; at most 9 are allowed."});
    }

    VisitedSet visited{};
    for (int i = 0; i < length; ++i) {
        const int current = nodes[static_cast<std::size_t>(i)];

        if (!PatternGraph::isValidId(current)) {
            result.issues.push_back({ValidationError::NodeOutOfRange,
                                     "Node " + std::to_string(current) +
                                         " does not exist (valid nodes are 1-9)."});
            continue;
        }
        if (visited[current]) {
            result.issues.push_back({ValidationError::RepeatedNode,
                                     "Node " + std::to_string(current) +
                                         " is used more than once."});
            continue;
        }
        if (i > 0) {
            const int previous = nodes[static_cast<std::size_t>(i - 1)];
            if (PatternGraph::isValidId(previous)) {
                const int mid = graph_.intermediate(previous, current);
                if (mid != 0 && !visited[mid]) {
                    result.issues.push_back(
                        {ValidationError::SkipsUnvisitedNode,
                         "Move " + std::to_string(previous) + " -> " + std::to_string(current) +
                             " passes over node " + std::to_string(mid) +
                             ", which has not been visited yet. Write " +
                             std::to_string(previous) + "-" + std::to_string(mid) + "-" +
                             std::to_string(current) + " instead."});
                }
            }
        }
        visited[current] = true;
    }
    return result;
}

}  // namespace nodelock
