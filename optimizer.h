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

  // Pass 3: peephole optimization.
  // Collapses two-instruction sequences where a pure producer writes to a
  // temporary register that is used only by an immediately-following Move:
  //   AddImmediate r_tmp, r_src, K  +  Move r_var, r_tmp  →  AddImmediate r_var, r_src, K
  //   Load r_tmp, K                 +  Move r_var, r_tmp  →  Load r_var, K
  void peephole(std::vector<Bytecode::BasicBlock> &blocks);

  // Pass 4: register compaction.
  // Renumbers all referenced registers to a dense 0..N-1 range to eliminate
  // gaps left by earlier optimizations.
  void compact_registers(std::vector<Bytecode::BasicBlock> &blocks);
};

}  // namespace bytecode
}  // namespace kai
