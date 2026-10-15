# NodeLock - Graph-Based Pattern Security Analyzer

NodeLock is an educational C++17 project for the **Advanced Programming** course at
**Yachay Tech University**. It models the 3x3 Android-style unlock grid as a graph, validates
patterns, generates every valid pattern with **recursive backtracking**, simulates a local
exhaustive search and gives a basic, honest structural analysis of a pattern.

> **Phase 1 (this repository state):** console application + reusable engine.
> **Phase 2 (planned):** an interactive web application where users draw a pattern.

> **Ethics / scope.** Everything runs in memory on your computer. NodeLock never connects
> to phones, accounts or authentication systems. The "search simulation" just counts
> positions in a list it generates itself.

## Objectives

1. Represent the 3x3 grid as a graph with explicit nodes and coordinates.
2. Validate patterns with Android-like rules and explain why a pattern is invalid.
3. Generate all valid patterns with recursion and backtracking and verify the known
   total of **389,112**.
4. Show that the number of attempts needed to find a pattern depends on the search order.
5. Separate *validity* from *estimated security* and document the analysis limitations.

## Rules implemented

| Rule | Description |
|------|-------------|
| Length | 4 to 9 nodes |
| Range | Node ids are 1-9 (keypad layout, 1 = top-left) |
| No repetition | A node can be used only once |
| Intermediate node | A line may pass over a node only if it was already visited (e.g. `1-3` is invalid, `2-1-3` is valid). Skipped nodes are **not** inserted automatically; the user must write them. |

```
1 2 3
4 5 6
7 8 9
```

## Features

- Grid and graph view (direct and conditional edges).
- Pattern validation with *all* reasons listed (`1-3-3` -> too short, skips node 2, repeated 3).
- Backtracking count by length:

  | Length | 4 | 5 | 6 | 7 | 8 | 9 | **Total** |
  |---|---|---|---|---|---|---|---|
  | Patterns | 1,624 | 7,152 | 26,016 | 72,912 | 140,704 | 140,704 | **389,112** |

- Deterministic search simulation in two documented orders (shortest-first and longest-first,
  lexicographic inside each length).
- Pattern analysis: length, distance, move types, direction changes, crossings, pass-overs,
  start position, bounding box, size of the search space, heuristic score + limitations.

## Requirements

- A C++17 compiler: **g++ 7 or newer** (tested with g++ 15.2, MSYS2 UCRT64 on Windows 11).
- Optional: GNU make (`make` on Linux/macOS, `mingw32-make` on Windows/MinGW).
- Optional: Code::Blocks (a `NodeLock.cbp` project file is included).
- No external libraries.

## Project structure

```
include/nodelock/    Public engine API (no console I/O) - reusable in Phase 2
  Node.h               Node struct + grid constants
  PatternGraph.h       Graph: nodes + intermediate-node adjacency matrix
  Pattern.h            Sequence of node ids, parsing and formatting
  PatternValidator.h   Rule checking with explanations
  PatternEnumerator.h  Recursive backtracking generator / counter
  SearchSimulator.h    Local, deterministic search simulation
  PatternAnalyzer.h    Geometric features + heuristic score
  NodeLockEngine.h     Facade that owns and wires all components
src/core/            Engine implementation
src/app/             Console interface (ConsoleUI) and main()
tests/               Unit tests (self-contained, no framework)
docs/                Code explanation (Spanish) and progress report
```

## Build, test and run

### With make

```bash
# Linux / macOS
make            # builds bin/nodelock and bin/nodelock_tests
make test       # runs the unit tests
make run        # starts the interactive menu

# Windows (MinGW / MSYS2)
mingw32-make
mingw32-make test
mingw32-make run
```

### Without make (any shell)

```bash
g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/core/*.cpp src/app/ConsoleUI.cpp src/app/main.cpp -o bin/nodelock
g++ -std=c++17 -Wall -Wextra -O2 -Iinclude src/core/*.cpp src/app/ConsoleUI.cpp tests/test_main.cpp -o bin/nodelock_tests
./bin/nodelock_tests
./bin/nodelock
```

On Windows `cmd.exe`, use backslashes for the executable (`bin\nodelock.exe`). If your shell
does not expand `src/core/*.cpp`, list the `.cpp` files explicitly.

### With Code::Blocks

Open `NodeLock.cbp`, pick the **App** or **Tests** build target and press *Build and run*.
Make sure the compiler settings use C++17 (`-std=c++17` is already in the project file).

### Interactive mode

Run `bin/nodelock` with no arguments:

```
================================================
               NODELOCK SECURITY
================================================

           PATTERN SECURITY ANALYZER

                  [1] [2] [3]
                  [4] [5] [6]
                  [7] [8] [9]

[1] Analyze a pattern
[2] Count all valid patterns
[3] Show the graph
[0] Exit
```

Option 1 asks for a pattern and shows its analysis:

```
Pattern:          2 -> 9 -> 8 -> 4
Validation:       VALID
Pattern length:   4 nodes

Security analysis:
  Length:         Short
  Complexity:     Low (20/100, structural heuristic)
  Predictability: Heuristic assessment
    Start:        edge
    Moves:        1 straight, 1 diagonal, 1 knight
    Turns:        2, crossings: 0

[1] Start search simulation
[2] Enter another pattern
[3] Show detailed analysis
[0] Back
```

### Command-line mode

```bash
bin/nodelock                       # interactive menu
bin/nodelock --grid                # show grid and graph
bin/nodelock --count               # count all valid patterns
bin/nodelock --validate 1-3-3      # explain why a pattern is invalid
bin/nodelock --simulate 1-2-3-6    # local search simulation in two orders
bin/nodelock --analyze 1-2-3-6     # geometric analysis
```

Patterns can be written as `1-2-3-6`, `"1 2 3 6"`, `1,2,3,6` or `1236`.
Exit codes: `0` success, `1` invalid pattern, `2` usage error, `3` unexpected error.

## Example output

```
$ bin/nodelock --count
Valid patterns by length (recursive backtracking):
  length 4:     1,624
  length 5:     7,152
  length 6:    26,016
  length 7:    72,912
  length 8:   140,704
  length 9:   140,704
  TOTAL    :   389,112
  Expected :   389,112  -> MATCH
  Recursive calls: 389,497, pruned moves: 14,200, time: 22 ms

$ bin/nodelock --simulate 1-2-3-6
* Shortest-first: lengths 4->9, lexicographic order inside each length
  Found at attempt 3 of 389,112 (<0.01% of the space)
* Longest-first: lengths 9->4, lexicographic order inside each length
  Found at attempt 387,491 of 389,112 (99.58% of the space)
```

## Limitations of the analysis

- The structural score is a teaching heuristic, **not** a security guarantee.
- Real users do not choose patterns uniformly at random; attackers try common shapes first.
- Smudge attacks, shoulder surfing and device lockout policies are not modelled.
- A pattern's position in one enumeration order says nothing about other orders.

## Documentation

- [`docs/EXPLICACION_CODIGO.md`](docs/EXPLICACION_CODIGO.md) - detailed code explanation (Spanish).
- [`docs/AVANCE_1.md`](docs/AVANCE_1.md) - Phase 1 progress report.

## Authors

- Juan Valencia
- David Guanoluisa

Advanced Programming, Yachay Tech University.
