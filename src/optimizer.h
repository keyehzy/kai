#pragma once
#include "bytecode.h"
#include <vector>

namespace kai {

class BytecodeOptimizer {
 public:
  void optimize(std::vector<Bytecode::BasicBlock> &blocks);

 private:
  // Pass 0: loop-invariant code motion.
  // Detects natural loops via back edges, then hoists pure instructions
  // whose operands are not modified anywhere in the loop to a pre-header
  // block that executes once.
  void loop_invariant_code_motion(std::vector<Bytecode::BasicBlock> &blocks);

  // Pass 1: within-block copy propagation.
  // Substitutes all register reads through Move chains, then removes
  // trivial moves (dst == src after substitution).
  void copy_propagation(std::vector<Bytecode::BasicBlock> &blocks);

  // Pass 2: global dead instruction elimination.
  // Removes instructions whose dst register is never read anywhere in
  // any block (pure computation with no observable effect).
  void dead_code_elimination(std::vector<Bytecode::BasicBlock> &blocks);

  // Pass 1.5: aggregate literal folding.
  // Rewrites ArrayCreate/StructCreate with register operands into
  // ArrayLiteralCreate/StructLiteralCreate when every operand register is
  // proven to come from a constant Load at that point in the block.
  void fold_aggregate_literals(std::vector<Bytecode::BasicBlock> &blocks);

  // Pass 3: tail-call optimization.
  // Rewrites:
  //   Call r_tmp, @f, args
  //   Return r_tmp
  // into:
  //   TailCall @f, args
  // so the interpreter can reuse the current frame.
  void tail_call_optimization(std::vector<Bytecode::BasicBlock> &blocks);

  // Pass 3.5: CFG cleanup.
  // - trims instructions after the first block terminator
  // - rewrites branch targets through jump-only trampoline chains
  // - removes unreferenced jump-only trampoline blocks
  void cfg_cleanup(std::vector<Bytecode::BasicBlock> &blocks);

  // Pass 4: peephole optimization.
  // Collapses two-instruction sequences where a pure producer writes to a
  // temporary register that is used only by an immediately-following Move:
  //   <immediate-op> r_tmp, r_src, K + Move r_var, r_tmp -> <immediate-op> r_var, r_src, K
  //   Load r_tmp, K                  + Move r_var, r_tmp -> Load r_var, K
  void peephole(std::vector<Bytecode::BasicBlock> &blocks);

  // Pass 5: register compaction.
  // Renumbers all referenced registers to a dense 0..N-1 range to eliminate
  // gaps left by earlier optimizations.
  void compact_registers(std::vector<Bytecode::BasicBlock> &blocks);
};

}  // namespace kai
