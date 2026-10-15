# NodeLock - Progress Report 1 (Phase 1)

**Course:** Advanced Programming - Yachay Tech University
**Project:** NodeLock - Graph-Based Pattern Security Analyzer
**Phase:** 1 of 2 (C++17 console application and reusable engine)

## 1. Objectives of Phase 1

1. Model the 3x3 unlock grid as a graph with nodes, coordinates and edge conditions.
2. Read and validate user patterns with Android-like rules, explaining every violation.
3. Generate all valid patterns using recursion and backtracking, counting them by length
   and verifying the expected total of 389,112.
4. Simulate, locally and deterministically, an exhaustive search and show at which attempt
   a test pattern is reached, emphasising that the result depends on the search order.
5. Provide a basic geometric analysis that clearly separates *validity* from *estimated
   security* and documents its limitations.
6. Keep the engine independent from the console so it can be reused by a web interface.

## 2. Implemented components

| Component | Files | Responsibility | Status |
|-----------|-------|----------------|--------|
| `Node` | `include/nodelock/Node.h` | Node id + (row, col); grid constants | Done, tested |
| `PatternGraph` | `PatternGraph.h/.cpp` | 9 nodes; 10x10 intermediate-node adjacency matrix; `canMove` rule | Done, tested |
| `Pattern` | `Pattern.h/.cpp` | Sequence of ids; parsing (`1-2-3-6`, `1236`, ...) and formatting | Done, tested |
| `PatternValidator` | `PatternValidator.h/.cpp` | Length, range, repetition, intermediate-node rules; collects all issues | Done, tested |
| `PatternEnumerator` | `PatternEnumerator.h/.cpp` | Recursive DFS with backtracking; counts by length; visitor with early stop | Done, tested |
| `SearchSimulator` | `SearchSimulator.h/.cpp` | Attempt number of a test pattern in two documented orders | Done, tested |
| `PatternAnalyzer` | `PatternAnalyzer.h/.cpp` | Distance, move types, turns, crossings, pass-overs, start, bounding box, heuristic score | Done, tested |
| `NodeLockEngine` | `NodeLockEngine.h/.cpp` | Facade that owns and wires the components | Done, tested |
| `ConsoleUI` + `main` | `src/app/` | Interactive menu and one-shot command-line mode | Done, tested |
| Unit tests | `tests/test_main.cpp` | 107 checks across 9 test groups | All passing |

## 3. Verified results

Build: `g++ 15.2 -std=c++17 -Wall -Wextra -Wpedantic` with no warnings.

| Length | Patterns |
|--------|---------:|
| 4 | 1,624 |
| 5 | 7,152 |
| 6 | 26,016 |
| 7 | 72,912 |
| 8 | 140,704 |
| 9 | 140,704 |
| **Total** | **389,112** |

- The full enumeration makes 389,497 recursive calls (9 + 56 + 320 prefixes of length 1-3,
  plus 389,112 complete patterns) and rejects 14,200 moves because of the
  intermediate-node rule. It runs in about 20 ms on a laptop.
- Every one of the 389,112 generated patterns is accepted by the validator and no pattern
  is generated twice (cross-check between two independent implementations of the rules).
- Search simulation for `1-2-3-6`: attempt **3** in shortest-first order, attempt
  **387,491** in longest-first order - the same pattern, two very different numbers.

## 4. Design decisions

- **Graph representation:** complete graph K9 whose edges carry a condition, stored as an
  adjacency matrix `between[a][b]` (0 = direct edge, k = requires node k). The matrix is
  computed from coordinates, not hard-coded.
- **Separation of concerns:** the engine (`include/`, `src/core/`) has no console I/O; only
  `ConsoleUI` reads and writes streams. The UI receives `std::istream&`/`std::ostream&`,
  which makes it testable with string streams.
- **Error handling:** invalid input raises `std::invalid_argument`/`std::out_of_range` in
  the engine; the UI catches them and prints a clear message. Validation returns a list of
  issues instead of stopping at the first one.
- **No external dependencies:** only the C++17 standard library; builds with g++, make and
  Code::Blocks.

## 5. Known limitations

- The structural score is heuristic and not a real security metric.
- The simulator uses two fixed orders; real attackers use frequency-ordered dictionaries.
- Skipped intermediate nodes are not auto-inserted as Android does; the user must write them.
- The Code::Blocks project file was written by hand and has not yet been opened in the IDE.

## 6. Future work (Phase 2)

- Interactive web application where the user draws a pattern on a 3x3 canvas.
- Reuse the C++ engine (for example, compiled to WebAssembly or exposed by a small server)
  through the `NodeLockEngine` facade.
- Visualise the backtracking tree and the search progress.
- Add more search orders (e.g. frequency-based) and compare them.
