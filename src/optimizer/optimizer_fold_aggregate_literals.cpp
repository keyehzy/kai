#include "../optimizer.h"
#include "optimizer_internal.h"

#include <unordered_map>

namespace kai {

using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;

void BytecodeOptimizer::fold_aggregate_literals(
    std::vector<Bytecode::BasicBlock> &blocks) {
  for (auto &block : blocks) {
    std::unordered_map<Register, Bytecode::Value> constant_loads;
    for (auto &instr_ptr : block.instructions) {
      auto &instr = *instr_ptr;
      if (instr.type() == Type::Load) {
        const auto &load = derived_cast<const Bytecode::Instruction::Load &>(instr);
        constant_loads[load.dst] = load.value;
        continue;
      }

      if (instr.type() == Type::ArrayCreate) {
        const auto &array_create =
            derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr);
        std::vector<Bytecode::Value> elements;
        elements.reserve(array_create.elements.size());
        bool all_constant_loads = true;
        for (const auto element_reg : array_create.elements) {
          const auto it = constant_loads.find(element_reg);
          if (it == constant_loads.end()) {
            all_constant_loads = false;
            break;
          }
          elements.push_back(it->second);
        }
        if (all_constant_loads) {
          instr_ptr = std::make_unique<Bytecode::Instruction::ArrayLiteralCreate>(
              array_create.dst, std::move(elements));
          constant_loads.erase(array_create.dst);
          continue;
        }
      } else if (instr.type() == Type::ArrayLoad) {
        const auto &array_load =
            derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
        if (const auto it = constant_loads.find(array_load.index);
            it != constant_loads.end()) {
          instr_ptr = std::make_unique<Bytecode::Instruction::ArrayLoadImmediate>(
              array_load.dst, array_load.array, it->second);
          constant_loads.erase(array_load.dst);
          continue;
        }
      } else if (instr.type() == Type::StructCreate) {
        const auto &struct_create =
            derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
        std::vector<std::pair<std::string, Bytecode::Value>> fields;
        fields.reserve(struct_create.fields.size());
        bool all_constant_loads = true;
        for (const auto &[field_name, field_reg] : struct_create.fields) {
          const auto it = constant_loads.find(field_reg);
          if (it == constant_loads.end()) {
            all_constant_loads = false;
            break;
          }
          fields.emplace_back(field_name, it->second);
        }
        if (all_constant_loads) {
          instr_ptr = std::make_unique<Bytecode::Instruction::StructLiteralCreate>(
              struct_create.dst, std::move(fields));
          constant_loads.erase(struct_create.dst);
          continue;
        }
      }

      if (const auto dst_opt = get_dst_reg(instr); dst_opt) {
        constant_loads.erase(*dst_opt);
      }
    }
  }
}

}  // namespace kai
