#include "optimizer.h"

namespace kai {

void BytecodeOptimizer::optimize(std::vector<Bytecode::BasicBlock> &blocks) {
  loop_invariant_code_motion(blocks);
  copy_propagation(blocks);
  fold_aggregate_literals(blocks);
  dead_code_elimination(blocks);
  tail_call_optimization(blocks);
  cfg_cleanup(blocks);
  peephole(blocks);
  compact_registers(blocks);
}

}  // namespace kai
