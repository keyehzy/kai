#!/usr/bin/env bash

set -e
set -x
set -o pipefail

CXXFLAGS="-O0 -g3 -Wall -Wextra -std=c++20"
CXX="/opt/homebrew/opt/llvm/bin/clang++"
CC="/opt/homebrew/opt/llvm/bin/clang"

${CXX} ${CXXFLAGS} -o main main.cpp ast.cpp
