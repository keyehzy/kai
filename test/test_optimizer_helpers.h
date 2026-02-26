#pragma once

#include "catch.hpp"
#include "../src/bytecode.h"
#include "../src/optimizer.h"
#include "test_bytecode_cases.h"

using namespace kai;
using Type = Bytecode::Instruction::Type;

inline size_t total_instr_count(const std::vector<Bytecode::BasicBlock> &blocks) {
  size_t n = 0;
  for (const auto &b : blocks) n += b.instructions.size();
  return n;
}

inline bool has_instruction_type(const std::vector<Bytecode::BasicBlock> &blocks, Type type) {
  for (const auto &block : blocks) {
    for (const auto &instr : block.instructions) {
      if (instr->type() == type) {
        return true;
      }
    }
  }
  return false;
}
