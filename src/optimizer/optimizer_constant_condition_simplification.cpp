#include "../optimizer.h"
#include "optimizer_internal.h"

#include <unordered_map>

namespace kai {

using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;

void BytecodeOptimizer::simplify_constant_conditions(
    std::vector<Bytecode::BasicBlock> &blocks) {
  for (auto &block : blocks) {
    std::unordered_map<Register, Bytecode::Value> constants;
    for (auto &instr_ptr : block.instructions) {
      auto &instr = *instr_ptr;
      switch (instr.type()) {
        case Type::Load: {
          const auto &load = derived_cast<const Bytecode::Instruction::Load &>(instr);
          constants[load.dst] = load.value;
          break;
        }
        case Type::Move: {
          const auto &move = derived_cast<const Bytecode::Instruction::Move &>(instr);
          if (const auto it = constants.find(move.src); it != constants.end()) {
            constants[move.dst] = it->second;
          } else {
            constants.erase(move.dst);
          }
          break;
        }
        case Type::JumpConditional: {
          const auto &jump_conditional =
              derived_cast<const Bytecode::Instruction::JumpConditional &>(instr);
          if (const auto it = constants.find(jump_conditional.cond);
              it != constants.end()) {
            const auto target = it->second != 0 ? jump_conditional.label1
                                                : jump_conditional.label2;
            instr_ptr = std::make_unique<Bytecode::Instruction::Jump>(target);
          }
          break;
        }
        default:
          if (const auto dst_opt = get_dst_reg(instr); dst_opt) {
            constants.erase(*dst_opt);
          }
          break;
      }
    }
  }
}

}  // namespace kai
