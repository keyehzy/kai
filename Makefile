CXX      ?= clang++
CC       ?= clang
CXXFLAGS ?= -O3 -DNDEBUG -g3 -Wall -Wextra -std=c++20

COMMON_SRCS = src/ast.cpp src/bytecode.cpp src/optimizer.cpp src/parser.cpp

CLI_SRCS  = src/cli.cpp $(COMMON_SRCS)
CLI_BIN   = cli

TEST_SRCS = test/main.cpp test/test_*.cpp $(COMMON_SRCS)
TEST_BIN  = test/main

.PHONY: all test clean

all: $(CLI_BIN)

$(CLI_BIN): $(CLI_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(TEST_BIN): $(TEST_SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(CLI_BIN) $(TEST_BIN)
