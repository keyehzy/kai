#include "../optimizer.h"

#include <cstddef>

namespace kai {

using Type = Bytecode::Instruction::Type;

void BytecodeOptimizer::tail_call_optimization(
    std::vector<Bytecode::BasicBlock> &blocks) {
  for (auto &block : blocks) {
    auto &instrs = block.instructions;
    size_t i = 0;
    while (i + 1 < instrs.size()) {
      if (instrs[i]->type() != Type::Call || instrs[i + 1]->type() != Type::Return) {
        ++i;
        continue;
      }

      auto &call = derived_cast<Bytecode::Instruction::Call &>(*instrs[i]);
      const auto &ret =
          derived_cast<const Bytecode::Instruction::Return &>(*instrs[i + 1]);
      if (ret.reg != call.dst) {
        ++i;
        continue;
      }

      instrs[i] = std::make_unique<Bytecode::Instruction::TailCall>(
          call.label, std::move(call.arg_registers), std::move(call.param_registers));
      instrs.erase(instrs.begin() + static_cast<std::ptrdiff_t>(i + 1));
    }
  }
}

}  // namespace kai
