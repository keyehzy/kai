#!/usr/bin/env bash

set -e
set -x
set -o pipefail

CXXFLAGS="-O0 -g -Wall -Wextra"
CXX="/opt/homebrew/opt/llvm/bin/clang++"
CC="/opt/homebrew/opt/llvm/bin/clang"

${CXX} ${CXXFLAGS} -o main main.cpp
