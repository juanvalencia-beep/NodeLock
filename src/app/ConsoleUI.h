#pragma once

#include <iosfwd>
#include <string>

#include "nodelock/NodeLockEngine.h"

namespace nodelock {

class ConsoleUI {
public:
    ConsoleUI(const NodeLockEngine& engine, std::istream& in, std::ostream& out)
        : engine_(engine), in_(in), out_(out) {}

    void run();

    void showGraph() const;
    void showCounts() const;
    bool showValidation(const std::string& text) const;
    bool showSimulation(const std::string& text) const;
    bool showAnalysis(const std::string& text) const;

private:
    bool readLine(const std::string& prompt, std::string& line);
    void printBanner() const;
    bool patternScreen();  // returns false on end of input
    void printPatternReport(const std::string& text, Pattern& pattern, bool& valid) const;
    void printPatternGrid(const Pattern& pattern) const;
    bool parseValid(const std::string& text, Pattern& pattern) const;

    const NodeLockEngine& engine_;
    std::istream& in_;
    std::ostream& out_;
};

}  // namespace nodelock
