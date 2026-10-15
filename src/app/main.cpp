#include <exception>
#include <iostream>
#include <string>

#include "ConsoleUI.h"
#include "nodelock/NodeLockEngine.h"

namespace {

void printUsage(std::ostream& out) {
    out << "Usage: nodelock [--grid | --count | --validate P | --simulate P | --analyze P]\n"
           "  P is a pattern such as 1-2-3-6, \"1 2 3 6\" or 1236.\n"
           "  Without arguments, an interactive menu is shown.\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const nodelock::NodeLockEngine engine;
        nodelock::ConsoleUI ui(engine, std::cin, std::cout);

        if (argc == 1) {
            ui.run();
            return 0;
        }

        const std::string command = argv[1];
        if (command == "--help" || command == "-h") { printUsage(std::cout); return 0; }
        if (command == "--grid" && argc == 2) { ui.showGraph(); return 0; }
        if (command == "--count" && argc == 2) { ui.showCounts(); return 0; }

        if (argc >= 3) {
            std::string pattern;
            for (int i = 2; i < argc; ++i) pattern += std::string(argv[i]) + " ";
            if (command == "--validate") return ui.showValidation(pattern) ? 0 : 1;
            if (command == "--simulate") return ui.showSimulation(pattern) ? 0 : 1;
            if (command == "--analyze") return ui.showAnalysis(pattern) ? 0 : 1;
        }
        printUsage(std::cerr);
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 3;
    }
}
