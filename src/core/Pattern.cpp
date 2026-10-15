#include "nodelock/Pattern.h"

#include <cctype>
#include <stdexcept>

namespace nodelock {

Pattern Pattern::parse(const std::string& text) {
    std::vector<int> nodes;
    for (char ch : text) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (std::isdigit(c)) {
            nodes.push_back(ch - '0');
        } else if (std::isspace(c) || ch == '-' || ch == ',' || ch == '>') {
            continue;
        } else {
            throw std::invalid_argument(std::string("Unexpected character '") + ch +
                                        "'. Use node numbers 1-9, e.g. 1-2-3-6");
        }
    }
    if (nodes.empty()) {
        throw std::invalid_argument("The pattern is empty. Example: 1-2-3-6");
    }
    return Pattern(std::move(nodes));
}

std::string Pattern::toString() const {
    std::string out;
    for (std::size_t i = 0; i < nodes_.size(); ++i) {
        if (i > 0) out += '-';
        out += std::to_string(nodes_[i]);
    }
    return out;
}

}  // namespace nodelock
