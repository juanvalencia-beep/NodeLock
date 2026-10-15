#pragma once

#include <array>
#include <vector>

#include "nodelock/Node.h"

namespace nodelock {

using VisitedSet = std::array<bool, kNodeCount + 1>;  // index 0 unused

// Complete graph of 9 nodes. between_[a][b] is the node lying between a and b
// (0 if none): the move a -> b is only allowed once that node was visited.
class PatternGraph {
public:
    PatternGraph();

    const std::vector<Node>& nodes() const { return nodes_; }
    const Node& node(int id) const;  // throws std::out_of_range

    static bool isValidId(int id) { return id >= 1 && id <= kNodeCount; }

    int intermediate(int from, int to) const;
    bool canMove(int from, int to, const VisitedSet& visited) const;

    std::vector<int> directNeighbors(int id) const;
    std::vector<int> conditionalNeighbors(int id) const;

private:
    std::vector<Node> nodes_;
    std::array<std::array<int, kNodeCount + 1>, kNodeCount + 1> between_{};
};

}  // namespace nodelock
