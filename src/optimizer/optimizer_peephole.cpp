#include "../optimizer.h"
#include "optimizer_internal.h"

#include <cassert>
#include <unordered_map>

namespace kai {

using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;

namespace {

// Counts how many times each register appears as a source operand across all blocks.
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
          const auto &gt =
              derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
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
          const auto &c = derived_cast<const Bytecode::Instruction::Call &>(instr);
          for (auto r : c.arg_registers) use(r);
          break;
        }
        case Type::TailCall: {
          const auto &tc = derived_cast<const Bytecode::Instruction::TailCall &>(instr);
          for (auto r : tc.arg_registers) use(r);
          break;
        }
        case Type::Return:
          use(derived_cast<const Bytecode::Instruction::Return &>(instr).reg);
          break;
        case Type::Equal: {
          const auto &e = derived_cast<const Bytecode::Instruction::Equal &>(instr);
          use(e.src1);
          use(e.src2);
          break;
        }
        case Type::EqualImmediate:
          use(derived_cast<const Bytecode::Instruction::EqualImmediate &>(instr).src);
          break;
        case Type::NotEqual: {
          const auto &ne =
              derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
          use(ne.src1);
          use(ne.src2);
          break;
        }
        case Type::NotEqualImmediate:
          use(derived_cast<const Bytecode::Instruction::NotEqualImmediate &>(instr).src);
          break;
        case Type::Add: {
          const auto &a = derived_cast<const Bytecode::Instruction::Add &>(instr);
          use(a.src1);
          use(a.src2);
          break;
        }
        case Type::AddImmediate:
          use(derived_cast<const Bytecode::Instruction::AddImmediate &>(instr).src);
          break;
        case Type::Subtract: {
          const auto &s =
              derived_cast<const Bytecode::Instruction::Subtract &>(instr);
          use(s.src1);
          use(s.src2);
          break;
        }
        case Type::SubtractImmediate:
          use(derived_cast<const Bytecode::Instruction::SubtractImmediate &>(instr).src);
          break;
        case Type::Multiply: {
          const auto &m =
              derived_cast<const Bytecode::Instruction::Multiply &>(instr);
          use(m.src1);
          use(m.src2);
          break;
        }
        case Type::MultiplyImmediate:
          use(derived_cast<const Bytecode::Instruction::MultiplyImmediate &>(instr).src);
          break;
        case Type::Divide: {
          const auto &d =
              derived_cast<const Bytecode::Instruction::Divide &>(instr);
          use(d.src1);
          use(d.src2);
          break;
        }
        case Type::DivideImmediate:
          use(derived_cast<const Bytecode::Instruction::DivideImmediate &>(instr).src);
          break;
        case Type::Modulo: {
          const auto &mo =
              derived_cast<const Bytecode::Instruction::Modulo &>(instr);
          use(mo.src1);
          use(mo.src2);
          break;
        }
        case Type::ModuloImmediate:
          use(derived_cast<const Bytecode::Instruction::ModuloImmediate &>(instr).src);
          break;
        case Type::ArrayCreate: {
          const auto &ac =
              derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr);
          for (auto r : ac.elements) use(r);
          break;
        }
        case Type::ArrayLiteralCreate:
          break;
        case Type::ArrayLoad: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          use(al.array);
          use(al.index);
          break;
        }
        case Type::ArrayLoadImmediate: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoadImmediate &>(instr);
          use(al.array);
          break;
        }
        case Type::ArrayStore: {
          const auto &as_ =
              derived_cast<const Bytecode::Instruction::ArrayStore &>(instr);
          use(as_.array);
          use(as_.index);
          use(as_.value);
          break;
        }
        case Type::StructCreate: {
          const auto &sc =
              derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
          for (const auto &[name, r] : sc.fields) use(r);
          break;
        }
        case Type::StructLiteralCreate:
          break;
        case Type::StructLoad:
          use(derived_cast<const Bytecode::Instruction::StructLoad &>(instr).object);
          break;
        case Type::AddressOf:
          use(derived_cast<const Bytecode::Instruction::AddressOf &>(instr).src);
          break;
        case Type::LoadIndirect:
          use(derived_cast<const Bytecode::Instruction::LoadIndirect &>(instr).pointer);
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

void BytecodeOptimizer::peephole(std::vector<Bytecode::BasicBlock> &blocks) {
  // Build a global count of how many times each register is read as a source
  // operand.  This is used to prove that a temporary register is used only
  // once (by the immediately-following Move) before we eliminate the Move.
  const auto use_count = compute_use_count(blocks);

  const auto is_foldable_producer = [](Type t) {
    switch (t) {
      case Type::Load:
      case Type::LessThan:
      case Type::LessThanImmediate:
      case Type::GreaterThan:
      case Type::GreaterThanImmediate:
      case Type::LessThanOrEqual:
      case Type::LessThanOrEqualImmediate:
      case Type::GreaterThanOrEqual:
      case Type::GreaterThanOrEqualImmediate:
      case Type::Equal:
      case Type::EqualImmediate:
      case Type::NotEqual:
      case Type::NotEqualImmediate:
      case Type::Add:
      case Type::AddImmediate:
      case Type::Subtract:
      case Type::SubtractImmediate:
      case Type::Multiply:
      case Type::MultiplyImmediate:
      case Type::Divide:
      case Type::DivideImmediate:
      case Type::Modulo:
      case Type::ModuloImmediate:
      case Type::AddressOf:
      case Type::LoadIndirect:
        return true;
      default:
        return false;
    }
  };

  const auto rewrite_dst = [](Bytecode::Instruction &instr, Type t, Register dst) {
    switch (t) {
      case Type::Load:
        derived_cast<Bytecode::Instruction::Load &>(instr).dst = dst;
        break;
      case Type::LessThan:
        derived_cast<Bytecode::Instruction::LessThan &>(instr).dst = dst;
        break;
      case Type::LessThanImmediate:
        derived_cast<Bytecode::Instruction::LessThanImmediate &>(instr).dst = dst;
        break;
      case Type::GreaterThan:
        derived_cast<Bytecode::Instruction::GreaterThan &>(instr).dst = dst;
        break;
      case Type::GreaterThanImmediate:
        derived_cast<Bytecode::Instruction::GreaterThanImmediate &>(instr).dst = dst;
        break;
      case Type::LessThanOrEqual:
        derived_cast<Bytecode::Instruction::LessThanOrEqual &>(instr).dst = dst;
        break;
      case Type::LessThanOrEqualImmediate:
        derived_cast<Bytecode::Instruction::LessThanOrEqualImmediate &>(instr).dst = dst;
        break;
      case Type::GreaterThanOrEqual:
        derived_cast<Bytecode::Instruction::GreaterThanOrEqual &>(instr).dst = dst;
        break;
      case Type::GreaterThanOrEqualImmediate:
        derived_cast<Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr).dst = dst;
        break;
      case Type::Equal:
        derived_cast<Bytecode::Instruction::Equal &>(instr).dst = dst;
        break;
      case Type::EqualImmediate:
        derived_cast<Bytecode::Instruction::EqualImmediate &>(instr).dst = dst;
        break;
      case Type::NotEqual:
        derived_cast<Bytecode::Instruction::NotEqual &>(instr).dst = dst;
        break;
      case Type::NotEqualImmediate:
        derived_cast<Bytecode::Instruction::NotEqualImmediate &>(instr).dst = dst;
        break;
      case Type::Add:
        derived_cast<Bytecode::Instruction::Add &>(instr).dst = dst;
        break;
      case Type::AddImmediate:
        derived_cast<Bytecode::Instruction::AddImmediate &>(instr).dst = dst;
        break;
      case Type::Subtract:
        derived_cast<Bytecode::Instruction::Subtract &>(instr).dst = dst;
        break;
      case Type::SubtractImmediate:
        derived_cast<Bytecode::Instruction::SubtractImmediate &>(instr).dst = dst;
        break;
      case Type::Multiply:
        derived_cast<Bytecode::Instruction::Multiply &>(instr).dst = dst;
        break;
      case Type::MultiplyImmediate:
        derived_cast<Bytecode::Instruction::MultiplyImmediate &>(instr).dst = dst;
        break;
      case Type::Divide:
        derived_cast<Bytecode::Instruction::Divide &>(instr).dst = dst;
        break;
      case Type::DivideImmediate:
        derived_cast<Bytecode::Instruction::DivideImmediate &>(instr).dst = dst;
        break;
      case Type::Modulo:
        derived_cast<Bytecode::Instruction::Modulo &>(instr).dst = dst;
        break;
      case Type::ModuloImmediate:
        derived_cast<Bytecode::Instruction::ModuloImmediate &>(instr).dst = dst;
        break;
      case Type::AddressOf:
        derived_cast<Bytecode::Instruction::AddressOf &>(instr).dst = dst;
        break;
      case Type::LoadIndirect:
        derived_cast<Bytecode::Instruction::LoadIndirect &>(instr).dst = dst;
        break;
      default:
        assert(false);
    }
  };

  for (auto &block : blocks) {
    auto &instrs = block.instructions;
    size_t i = 0;
    while (i + 1 < instrs.size()) {
      const auto t = instrs[i]->type();
      // Only fold patterns whose producing instruction is pure and writes a dst.
      if (!is_foldable_producer(t)) {
        ++i;
        continue;
      }
      // The immediately-following instruction must be a Move.
      if (instrs[i + 1]->type() != Type::Move) {
        ++i;
        continue;
      }
      const auto &mv =
          derived_cast<const Bytecode::Instruction::Move &>(*instrs[i + 1]);
      // The Move must consume exactly the register produced by instrs[i].
      const auto tmp_opt = get_dst_reg(*instrs[i]);
      if (!tmp_opt || mv.src != *tmp_opt) {
        ++i;
        continue;
      }
      const Register r_tmp = *tmp_opt;
      // Safety: r_tmp must be used exactly once (by this Move and nothing else).
      const auto uc_it = use_count.find(r_tmp);
      if (uc_it == use_count.end() || uc_it->second != 1) {
        ++i;
        continue;
      }
      // Redirect the producing instruction's destination to mv.dst and
      // remove the now-redundant Move.
      const Register r_var = mv.dst;
      rewrite_dst(*instrs[i], t, r_var);
      instrs.erase(instrs.begin() + static_cast<std::ptrdiff_t>(i + 1));
      // Do not advance i: re-check this position for chained folding.
    }
  }
}

}  // namespace kai
