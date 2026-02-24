#pragma once
#include "bytecode.h"
#include <vector>

namespace kai {
namespace bytecode {

class BytecodeOptimizer {
 public:
  void optimize(std::vector<Bytecode::BasicBlock> &blocks);

 private:
  // Pass 1: within-block copy propagation.
  // Substitutes all register reads through Move chains, then removes
  // trivial moves (dst == src after substitution).
  void copy_propagation(std::vector<Bytecode::BasicBlock> &blocks);

  // Pass 2: global dead instruction elimination.
  // Removes instructions whose dst register is never read anywhere in
  // any block (pure computation with no observable effect).
  void dead_code_elimination(std::vector<Bytecode::BasicBlock> &blocks);

  // Pass 0: loop-invariant code motion.
  // Detects natural loops via back edges, then hoists pure instructions
  // whose operands are not modified anywhere in the loop to a pre-header
  // block that executes once.
  void loop_invariant_code_motion(std::vector<Bytecode::BasicBlock> &blocks);
};

}  // namespace bytecode
}  // namespace kai
