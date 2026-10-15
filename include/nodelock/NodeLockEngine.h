#pragma once

#include "nodelock/Pattern.h"
#include "nodelock/PatternAnalyzer.h"
#include "nodelock/PatternEnumerator.h"
#include "nodelock/PatternGraph.h"
#include "nodelock/PatternValidator.h"
#include "nodelock/SearchSimulator.h"

namespace nodelock {

// Owns and connects every component. Performs no input/output.
class NodeLockEngine {
public:
    NodeLockEngine();

    NodeLockEngine(const NodeLockEngine&) = delete;
    NodeLockEngine& operator=(const NodeLockEngine&) = delete;

    const PatternGraph& graph() const { return graph_; }
    const PatternValidator& validator() const { return validator_; }
    const PatternEnumerator& enumerator() const { return enumerator_; }
    const SearchSimulator& simulator() const { return simulator_; }
    const PatternAnalyzer& analyzer() const { return analyzer_; }
    const EnumerationStats& stats() const { return stats_; }

    static constexpr std::uint64_t kExpectedTotal = 389112;

private:
    // Members are built in this order, so dependencies exist first.
    PatternGraph graph_;
    PatternValidator validator_;
    PatternEnumerator enumerator_;
    EnumerationStats stats_;
    SearchSimulator simulator_;
    PatternAnalyzer analyzer_;
};

}  // namespace nodelock
