#include "optimizer.h"

namespace kai {

void BytecodeOptimizer::optimize(std::vector<Bytecode::BasicBlock> &blocks) {
  // Pass -1: constant-condition simplification.
  simplify_constant_conditions(blocks);

  // Pass 0: loop-invariant code motion.
  loop_invariant_code_motion(blocks);

  // Pass 1: within-block copy propagation.
  copy_propagation(blocks);

  // Pass 1.5: aggregate literal folding.
  fold_aggregate_literals(blocks);

  // Pass 2: global dead instruction elimination.
  dead_code_elimination(blocks);

  // Pass 3: tail-call optimization.
  tail_call_optimization(blocks);

  // Pass 3.5: CFG cleanup.
  cfg_cleanup(blocks);

  // Pass 4: peephole optimization.
  peephole(blocks);

  // Pass 5: register compaction.
  compact_registers(blocks);
}

}  // namespace kai
