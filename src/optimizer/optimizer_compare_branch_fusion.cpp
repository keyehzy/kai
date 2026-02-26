#include "../optimizer.h"

#include <cstddef>
#include <unordered_map>

namespace kai {

using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;

namespace {

std::unordered_map<Register, size_t> compute_use_count(
    const std::vector<Bytecode::BasicBlock> &blocks) {
  std::unordered_map<Register, size_t> use_count;
  const auto use = [&](Register r) { ++use_count[r]; };

  for (const auto &block : blocks) {
    for (const auto &instr_ptr : block.instructions) {
      const auto &instr = *instr_ptr;
      switch (instr.type()) {
        case Type::Move:
          use(derived_cast<const Bytecode::Instruction::Move &>(instr).src);
          break;
        case Type::Load:
          break;
        case Type::LessThan: {
          const auto &lt = derived_cast<const Bytecode::Instruction::LessThan &>(instr);
          use(lt.lhs);
          use(lt.rhs);
          break;
        }
        case Type::LessThanImmediate:
          use(derived_cast<const Bytecode::Instruction::LessThanImmediate &>(instr).lhs);
          break;
        case Type::GreaterThan: {
          const auto &gt = derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
          use(gt.lhs);
          use(gt.rhs);
          break;
        }
        case Type::GreaterThanImmediate:
          use(derived_cast<const Bytecode::Instruction::GreaterThanImmediate &>(instr).lhs);
          break;
        case Type::LessThanOrEqual: {
          const auto &lte =
              derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr);
          use(lte.lhs);
          use(lte.rhs);
          break;
        }
        case Type::LessThanOrEqualImmediate:
          use(derived_cast<const Bytecode::Instruction::LessThanOrEqualImmediate &>(instr)
                  .lhs);
          break;
        case Type::GreaterThanOrEqual: {
          const auto &gte =
              derived_cast<const Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          use(gte.lhs);
          use(gte.rhs);
          break;
        }
        case Type::GreaterThanOrEqualImmediate:
          use(derived_cast<const Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr)
                  .lhs);
          break;
        case Type::Jump:
          break;
        case Type::JumpConditional:
          use(derived_cast<const Bytecode::Instruction::JumpConditional &>(instr).cond);
          break;
        case Type::JumpEqualImmediate:
          use(derived_cast<const Bytecode::Instruction::JumpEqualImmediate &>(instr).src);
          break;
        case Type::JumpGreaterThanImmediate:
          use(derived_cast<const Bytecode::Instruction::JumpGreaterThanImmediate &>(instr)
                  .lhs);
          break;
        case Type::JumpLessThanOrEqual: {
          const auto &jump_lte =
              derived_cast<const Bytecode::Instruction::JumpLessThanOrEqual &>(instr);
          use(jump_lte.lhs);
          use(jump_lte.rhs);
          break;
        }
        case Type::Call: {
          const auto &call = derived_cast<const Bytecode::Instruction::Call &>(instr);
          for (const auto reg : call.arg_registers) {
            use(reg);
          }
          break;
        }
        case Type::TailCall: {
          const auto &tail_call =
              derived_cast<const Bytecode::Instruction::TailCall &>(instr);
          for (const auto reg : tail_call.arg_registers) {
            use(reg);
          }
          break;
        }
        case Type::Return:
          use(derived_cast<const Bytecode::Instruction::Return &>(instr).reg);
          break;
        case Type::Equal: {
          const auto &equal = derived_cast<const Bytecode::Instruction::Equal &>(instr);
          use(equal.src1);
          use(equal.src2);
          break;
        }
        case Type::EqualImmediate:
          use(derived_cast<const Bytecode::Instruction::EqualImmediate &>(instr).src);
          break;
        case Type::NotEqual: {
          const auto &not_equal = derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
          use(not_equal.src1);
          use(not_equal.src2);
          break;
        }
        case Type::NotEqualImmediate:
          use(derived_cast<const Bytecode::Instruction::NotEqualImmediate &>(instr).src);
          break;
        case Type::Add: {
          const auto &add = derived_cast<const Bytecode::Instruction::Add &>(instr);
          use(add.src1);
          use(add.src2);
          break;
        }
        case Type::AddImmediate:
          use(derived_cast<const Bytecode::Instruction::AddImmediate &>(instr).src);
          break;
        case Type::Subtract: {
          const auto &subtract = derived_cast<const Bytecode::Instruction::Subtract &>(instr);
          use(subtract.src1);
          use(subtract.src2);
          break;
        }
        case Type::SubtractImmediate:
          use(derived_cast<const Bytecode::Instruction::SubtractImmediate &>(instr).src);
          break;
        case Type::Multiply: {
          const auto &multiply = derived_cast<const Bytecode::Instruction::Multiply &>(instr);
          use(multiply.src1);
          use(multiply.src2);
          break;
        }
        case Type::MultiplyImmediate:
          use(derived_cast<const Bytecode::Instruction::MultiplyImmediate &>(instr).src);
          break;
        case Type::Divide: {
          const auto &divide = derived_cast<const Bytecode::Instruction::Divide &>(instr);
          use(divide.src1);
          use(divide.src2);
          break;
        }
        case Type::DivideImmediate:
          use(derived_cast<const Bytecode::Instruction::DivideImmediate &>(instr).src);
          break;
        case Type::Modulo: {
          const auto &modulo = derived_cast<const Bytecode::Instruction::Modulo &>(instr);
          use(modulo.src1);
          use(modulo.src2);
          break;
        }
        case Type::ModuloImmediate:
          use(derived_cast<const Bytecode::Instruction::ModuloImmediate &>(instr).src);
          break;
        case Type::ArrayCreate: {
          const auto &array_create =
              derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr);
          for (const auto reg : array_create.elements) {
            use(reg);
          }
          break;
        }
        case Type::ArrayLiteralCreate:
          break;
        case Type::ArrayLoad: {
          const auto &array_load = derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          use(array_load.array);
          use(array_load.index);
          break;
        }
        case Type::ArrayLoadImmediate:
          use(derived_cast<const Bytecode::Instruction::ArrayLoadImmediate &>(instr).array);
          break;
        case Type::ArrayStore: {
          const auto &array_store =
              derived_cast<const Bytecode::Instruction::ArrayStore &>(instr);
          use(array_store.array);
          use(array_store.index);
          use(array_store.value);
          break;
        }
        case Type::StructCreate: {
          const auto &struct_create =
              derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
          for (const auto &[field_name, field_reg] : struct_create.fields) {
            (void)field_name;
            use(field_reg);
          }
          break;
        }
        case Type::StructLiteralCreate:
          break;
        case Type::StructLoad:
          use(derived_cast<const Bytecode::Instruction::StructLoad &>(instr).object);
          break;
        case Type::Negate:
          use(derived_cast<const Bytecode::Instruction::Negate &>(instr).src);
          break;
        case Type::LogicalNot:
          use(derived_cast<const Bytecode::Instruction::LogicalNot &>(instr).src);
          break;
      }
    }
  }

  return use_count;
}

}  // namespace

void BytecodeOptimizer::fuse_compare_branches(std::vector<Bytecode::BasicBlock> &blocks) {
  const auto use_count = compute_use_count(blocks);

  for (auto &block : blocks) {
    auto &instructions = block.instructions;
    size_t i = 0;
    while (i + 1 < instructions.size()) {
      if (instructions[i + 1]->type() != Type::JumpConditional) {
        ++i;
        continue;
      }

      const auto &jump_cond =
          derived_cast<const Bytecode::Instruction::JumpConditional &>(*instructions[i + 1]);
      const auto use_it = use_count.find(jump_cond.cond);
      if (use_it == use_count.end() || use_it->second != 1) {
        ++i;
        continue;
      }

      std::unique_ptr<Bytecode::Instruction> fused;
      switch (instructions[i]->type()) {
        case Type::EqualImmediate: {
          const auto &equal_imm =
              derived_cast<const Bytecode::Instruction::EqualImmediate &>(*instructions[i]);
          if (equal_imm.dst == jump_cond.cond) {
            fused = std::make_unique<Bytecode::Instruction::JumpEqualImmediate>(
                equal_imm.src, equal_imm.value, jump_cond.label1, jump_cond.label2);
          }
          break;
        }
        case Type::GreaterThanImmediate: {
          const auto &greater_than_imm = derived_cast<
              const Bytecode::Instruction::GreaterThanImmediate &>(*instructions[i]);
          if (greater_than_imm.dst == jump_cond.cond) {
            fused = std::make_unique<Bytecode::Instruction::JumpGreaterThanImmediate>(
                greater_than_imm.lhs, greater_than_imm.value, jump_cond.label1,
                jump_cond.label2);
          }
          break;
        }
        case Type::LessThanOrEqual: {
          const auto &less_than_or_equal =
              derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(*instructions[i]);
          if (less_than_or_equal.dst == jump_cond.cond) {
            fused = std::make_unique<Bytecode::Instruction::JumpLessThanOrEqual>(
                less_than_or_equal.lhs, less_than_or_equal.rhs, jump_cond.label1,
                jump_cond.label2);
          }
          break;
        }
        default:
          break;
      }

      if (!fused) {
        ++i;
        continue;
      }

      instructions[i] = std::move(fused);
      instructions.erase(instructions.begin() + static_cast<std::ptrdiff_t>(i + 1));
      ++i;
    }
  }
}

}  // namespace kai
