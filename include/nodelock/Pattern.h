#pragma once

#include <string>
#include <vector>

namespace nodelock {

class Pattern {
public:
    Pattern() = default;
    explicit Pattern(std::vector<int> nodes) : nodes_(std::move(nodes)) {}

    // Accepts "1-2-3-6", "1 2 3 6", "1,2,3,6" or "1236".
    // Throws std::invalid_argument on empty input or unexpected characters.
    static Pattern parse(const std::string& text);

    const std::vector<int>& nodes() const { return nodes_; }
    std::size_t length() const { return nodes_.size(); }
    bool empty() const { return nodes_.empty(); }

    std::string toString() const;

    bool operator==(const Pattern& other) const { return nodes_ == other.nodes_; }
    bool operator!=(const Pattern& other) const { return !(*this == other); }

private:
    std::vector<int> nodes_;
};

}  // namespace nodelock
