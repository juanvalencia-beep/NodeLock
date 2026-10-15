#pragma once

#include <string>
#include <vector>

#include "nodelock/Pattern.h"
#include "nodelock/PatternGraph.h"

namespace nodelock {

enum class ValidationError {
    TooShort,
    TooLong,
    NodeOutOfRange,
    RepeatedNode,
    SkipsUnvisitedNode,
};

struct ValidationIssue {
    ValidationError code;
    std::string message;
};

struct ValidationResult {
    std::vector<ValidationIssue> issues;
    bool valid() const { return issues.empty(); }
    bool has(ValidationError code) const;
};

// Rules: 4-9 nodes, ids 1-9, no repeats, no jumping over an unvisited node.
class PatternValidator {
public:
    explicit PatternValidator(const PatternGraph& graph) : graph_(graph) {}

    // Reports every issue found, not only the first one.
    ValidationResult validate(const Pattern& pattern) const;

private:
    const PatternGraph& graph_;
};

}  // namespace nodelock
