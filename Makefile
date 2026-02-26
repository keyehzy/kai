CXX      ?= clang++
CC       ?= clang
CXXFLAGS ?= -O0 -g3 -Wall -Wextra -std=c++20
CPPFLAGS ?= 

OPTIMIZER_SRCS = src/optimizer.cpp src/optimizer/*.cpp
COMMON_SRCS = src/ast.cpp src/bytecode.cpp src/error_reporter.cpp $(OPTIMIZER_SRCS) src/parser.cpp src/shape.cpp src/typechecker.cpp

CLI_SRCS  = src/cli.cpp $(COMMON_SRCS)
CLI_BIN   = cli

TEST_SRCS = test/main.cpp test/test_*.cpp $(COMMON_SRCS)
TEST_BIN  = test/main

.PHONY: all test clean

all: $(CLI_BIN) $(TEST_BIN)

$(CLI_BIN): $(CLI_SRCS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^

$(TEST_BIN): $(TEST_SRCS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(CLI_BIN) $(TEST_BIN)
