#!/usr/bin/env bash

set -e
set -x
set -o pipefail

CXXFLAGS="${CXXFLAGS:--O0 -g3 -Wall -Wextra -std=c++20}"
CXX="${CXX:-clang++}"
CC="${CC:-clang}"

# Build cli
${CXX} ${CXXFLAGS} -o cli cli.cpp ast.cpp bytecode.cpp parser.cpp

# Build tests
${CXX} ${CXXFLAGS} -o test/main test/main.cpp test/test_*.cpp ast.cpp bytecode.cpp parser.cpp
./test/main
