# NodeLock - build file for GNU make (Linux/macOS: `make`, Windows/MinGW: `mingw32-make`)

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iinclude

CORE_SRC  = $(wildcard src/core/*.cpp)
APP_SRC   = src/app/ConsoleUI.cpp src/app/main.cpp
TEST_SRC  = tests/test_main.cpp src/app/ConsoleUI.cpp
HEADERS   = $(wildcard include/nodelock/*.h) src/app/ConsoleUI.h

ifeq ($(OS),Windows_NT)
  EXE = .exe
endif
APP  = bin/nodelock$(EXE)
TEST = bin/nodelock_tests$(EXE)

.PHONY: all test run clean

all: $(APP) $(TEST)

$(APP): $(CORE_SRC) $(APP_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CORE_SRC) $(APP_SRC) -o $@

$(TEST): $(CORE_SRC) $(TEST_SRC) $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CORE_SRC) $(TEST_SRC) -o $@

test: $(TEST)
	./$(TEST)

run: $(APP)
	./$(APP)

clean:
	rm -f $(APP) $(TEST)
