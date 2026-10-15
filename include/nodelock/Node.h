#pragma once

namespace nodelock {

constexpr int kGridSize = 3;
constexpr int kNodeCount = 9;
constexpr int kMinPatternLength = 4;
constexpr int kMaxPatternLength = 9;

// Keypad layout:  1 2 3 / 4 5 6 / 7 8 9
struct Node {
    int id;   // 1..9
    int row;  // 0..2
    int col;  // 0..2
};

}  // namespace nodelock
