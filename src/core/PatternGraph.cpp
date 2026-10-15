#include "nodelock/PatternGraph.h"

#include <stdexcept>
#include <string>

namespace nodelock {

PatternGraph::PatternGraph() {
    nodes_.reserve(kNodeCount);
    for (int row = 0; row < kGridSize; ++row) {
        for (int col = 0; col < kGridSize; ++col) {
            nodes_.push_back(Node{row * kGridSize + col + 1, row, col});
        }
    }

    // A node lies between a and b when both row and column differences are even.
    for (const Node& a : nodes_) {
        for (const Node& b : nodes_) {
            if (a.id == b.id) continue;
            const int dr = b.row - a.row;
            const int dc = b.col - a.col;
            if (dr % 2 == 0 && dc % 2 == 0) {
                const int midRow = (a.row + b.row) / 2;
                const int midCol = (a.col + b.col) / 2;
                between_[a.id][b.id] = midRow * kGridSize + midCol + 1;
            }
        }
    }
}

const Node& PatternGraph::node(int id) const {
    if (!isValidId(id)) {
        throw std::out_of_range("Node id " + std::to_string(id) + " is outside 1..9");
    }
    return nodes_[static_cast<std::size_t>(id - 1)];
}

int PatternGraph::intermediate(int from, int to) const {
    if (!isValidId(from) || !isValidId(to)) return 0;
    return between_[from][to];
}

bool PatternGraph::canMove(int from, int to, const VisitedSet& visited) const {
    if (!isValidId(from) || !isValidId(to) || from == to) return false;
    if (visited[to]) return false;
    const int mid = between_[from][to];
    return mid == 0 || visited[mid];
}

std::vector<int> PatternGraph::directNeighbors(int id) const {
    std::vector<int> result;
    for (int other = 1; other <= kNodeCount; ++other) {
        if (other != id && intermediate(id, other) == 0) result.push_back(other);
    }
    return result;
}

std::vector<int> PatternGraph::conditionalNeighbors(int id) const {
    std::vector<int> result;
    for (int other = 1; other <= kNodeCount; ++other) {
        if (other != id && intermediate(id, other) != 0) result.push_back(other);
    }
    return result;
}

}  // namespace nodelock
