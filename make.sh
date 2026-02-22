#!/usr/bin/env bash

set -e
set -x
set -o pipefail

CXXFLAGS="${CXXFLAGS:--O0 -g3 -Wall -Wextra -std=c++20}"
CXX="${CXX:-clang++}"
CC="${CC:-clang}"

${CXX} ${CXXFLAGS} -o main main.cpp ast.cpp bytecode.cpp
./main
