#include "../optimizer.h"

#include <algorithm>
#include <unordered_set>

namespace kai {

using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;

void BytecodeOptimizer::dead_code_elimination(
    std::vector<Bytecode::BasicBlock> &blocks) {
  std::unordered_set<Register> live;
  std::unordered_set<Register> address_taken;

  // Collect all registers that are read (source operands) across all blocks.
  for (const auto &block : blocks) {
    for (const auto &instr_ptr : block.instructions) {
      const auto &instr = *instr_ptr;
      switch (instr.type()) {
        case Type::Move: {
          const auto &m =
              derived_cast<const Bytecode::Instruction::Move &>(instr);
          live.insert(m.src);
          break;
        }
        case Type::Load:
          break;
        case Type::LessThan: {
          const auto &lt =
              derived_cast<const Bytecode::Instruction::LessThan &>(instr);
          live.insert(lt.lhs);
          live.insert(lt.rhs);
          break;
        }
        case Type::LessThanImmediate: {
          const auto &lti =
              derived_cast<const Bytecode::Instruction::LessThanImmediate &>(instr);
          live.insert(lti.lhs);
          break;
        }
        case Type::GreaterThan: {
          const auto &gt =
              derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
          live.insert(gt.lhs);
          live.insert(gt.rhs);
          break;
        }
        case Type::GreaterThanImmediate: {
          const auto &gti =
              derived_cast<const Bytecode::Instruction::GreaterThanImmediate &>(instr);
          live.insert(gti.lhs);
          break;
        }
        case Type::LessThanOrEqual: {
          const auto &lte =
              derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr);
          live.insert(lte.lhs);
          live.insert(lte.rhs);
          break;
        }
        case Type::LessThanOrEqualImmediate: {
          const auto &ltei =
              derived_cast<const Bytecode::Instruction::LessThanOrEqualImmediate &>(instr);
          live.insert(ltei.lhs);
          break;
        }
        case Type::GreaterThanOrEqual: {
          const auto &gte = derived_cast<
              const Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          live.insert(gte.lhs);
          live.insert(gte.rhs);
          break;
        }
        case Type::GreaterThanOrEqualImmediate: {
          const auto &gtei = derived_cast<
              const Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr);
          live.insert(gtei.lhs);
          break;
        }
        case Type::Jump:
          break;
        case Type::JumpConditional: {
          const auto &jc =
              derived_cast<const Bytecode::Instruction::JumpConditional &>(instr);
          live.insert(jc.cond);
          break;
        }
        case Type::JumpEqualImmediate: {
          const auto &jump_equal_imm =
              derived_cast<const Bytecode::Instruction::JumpEqualImmediate &>(instr);
          live.insert(jump_equal_imm.src);
          break;
        }
        case Type::JumpGreaterThanImmediate: {
          const auto &jump_greater_than_imm =
              derived_cast<const Bytecode::Instruction::JumpGreaterThanImmediate &>(instr);
          live.insert(jump_greater_than_imm.lhs);
          break;
        }
        case Type::JumpLessThanOrEqual: {
          const auto &jump_less_than_or_equal =
              derived_cast<const Bytecode::Instruction::JumpLessThanOrEqual &>(instr);
          live.insert(jump_less_than_or_equal.lhs);
          live.insert(jump_less_than_or_equal.rhs);
          break;
        }
        case Type::Call: {
          const auto &c =
              derived_cast<const Bytecode::Instruction::Call &>(instr);
          for (const auto &arg : c.arg_registers) {
            live.insert(arg);
          }
          break;
        }
        case Type::TailCall: {
          const auto &tc =
              derived_cast<const Bytecode::Instruction::TailCall &>(instr);
          for (const auto &arg : tc.arg_registers) {
            live.insert(arg);
          }
          break;
        }
        case Type::Return: {
          const auto &r =
              derived_cast<const Bytecode::Instruction::Return &>(instr);
          live.insert(r.reg);
          break;
        }
        case Type::Equal: {
          const auto &e =
              derived_cast<const Bytecode::Instruction::Equal &>(instr);
          live.insert(e.src1);
          live.insert(e.src2);
          break;
        }
        case Type::EqualImmediate: {
          const auto &ei =
              derived_cast<const Bytecode::Instruction::EqualImmediate &>(instr);
          live.insert(ei.src);
          break;
        }
        case Type::NotEqual: {
          const auto &ne =
              derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
          live.insert(ne.src1);
          live.insert(ne.src2);
          break;
        }
        case Type::NotEqualImmediate: {
          const auto &nei =
              derived_cast<const Bytecode::Instruction::NotEqualImmediate &>(instr);
          live.insert(nei.src);
          break;
        }
        case Type::Add: {
          const auto &a =
              derived_cast<const Bytecode::Instruction::Add &>(instr);
          live.insert(a.src1);
          live.insert(a.src2);
          break;
        }
        case Type::AddImmediate: {
          const auto &ai =
              derived_cast<const Bytecode::Instruction::AddImmediate &>(instr);
          live.insert(ai.src);
          break;
        }
        case Type::Subtract: {
          const auto &s =
              derived_cast<const Bytecode::Instruction::Subtract &>(instr);
          live.insert(s.src1);
          live.insert(s.src2);
          break;
        }
        case Type::SubtractImmediate: {
          const auto &si =
              derived_cast<const Bytecode::Instruction::SubtractImmediate &>(instr);
          live.insert(si.src);
          break;
        }
        case Type::Multiply: {
          const auto &m =
              derived_cast<const Bytecode::Instruction::Multiply &>(instr);
          live.insert(m.src1);
          live.insert(m.src2);
          break;
        }
        case Type::MultiplyImmediate: {
          const auto &mi =
              derived_cast<const Bytecode::Instruction::MultiplyImmediate &>(instr);
          live.insert(mi.src);
          break;
        }
        case Type::Divide: {
          const auto &d =
              derived_cast<const Bytecode::Instruction::Divide &>(instr);
          live.insert(d.src1);
          live.insert(d.src2);
          break;
        }
        case Type::DivideImmediate: {
          const auto &di =
              derived_cast<const Bytecode::Instruction::DivideImmediate &>(instr);
          live.insert(di.src);
          break;
        }
        case Type::Modulo: {
          const auto &mo =
              derived_cast<const Bytecode::Instruction::Modulo &>(instr);
          live.insert(mo.src1);
          live.insert(mo.src2);
          break;
        }
        case Type::ModuloImmediate: {
          const auto &mi =
              derived_cast<const Bytecode::Instruction::ModuloImmediate &>(instr);
          live.insert(mi.src);
          break;
        }
        case Type::ArrayCreate: {
          const auto &ac =
              derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr);
          for (const auto &elem : ac.elements) {
            live.insert(elem);
          }
          break;
        }
        case Type::ArrayLiteralCreate:
          break;
        case Type::ArrayLoad: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          live.insert(al.array);
          live.insert(al.index);
          break;
        }
        case Type::ArrayLoadImmediate: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoadImmediate &>(instr);
          live.insert(al.array);
          break;
        }
        case Type::ArrayStore: {
          const auto &as =
              derived_cast<const Bytecode::Instruction::ArrayStore &>(instr);
          live.insert(as.array);
          live.insert(as.index);
          live.insert(as.value);
          break;
        }
        case Type::StructCreate: {
          const auto &sc =
              derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
          for (const auto &[name, reg] : sc.fields) {
            live.insert(reg);
          }
          break;
        }
        case Type::StructLiteralCreate:
          break;
        case Type::StructLoad: {
          const auto &sl =
              derived_cast<const Bytecode::Instruction::StructLoad &>(instr);
          live.insert(sl.object);
          break;
        }
        case Type::AddressOf: {
          const auto &address_of =
              derived_cast<const Bytecode::Instruction::AddressOf &>(instr);
          live.insert(address_of.src);
          address_taken.insert(address_of.src);
          break;
        }
        case Type::LoadIndirect: {
          const auto &load_indirect =
              derived_cast<const Bytecode::Instruction::LoadIndirect &>(instr);
          live.insert(load_indirect.pointer);
          break;
        }
        case Type::Negate: {
          const auto &neg =
              derived_cast<const Bytecode::Instruction::Negate &>(instr);
          live.insert(neg.src);
          break;
        }
        case Type::LogicalNot: {
          const auto &ln =
              derived_cast<const Bytecode::Instruction::LogicalNot &>(instr);
          live.insert(ln.src);
          break;
        }
      }
    }
  }

  // Remove instructions whose destination register is never read.
  for (auto &block : blocks) {
    std::erase_if(block.instructions, [&](const auto &instr_ptr) {
      const auto &instr = *instr_ptr;
      // Never remove control flow or side-effecting instructions.
      switch (instr.type()) {
        case Type::Jump:
        case Type::JumpConditional:
        case Type::JumpEqualImmediate:
        case Type::JumpGreaterThanImmediate:
        case Type::JumpLessThanOrEqual:
        case Type::Return:
        case Type::Call:
        case Type::TailCall:
        case Type::ArrayStore:
          return false;
        default:
          break;
      }
      // For instructions with a dst register, remove if dst is never read.
      switch (instr.type()) {
        case Type::Move: {
          const auto &m =
              derived_cast<const Bytecode::Instruction::Move &>(instr);
          if (address_taken.contains(m.dst)) {
            return false;
          }
          return live.find(m.dst) == live.end();
        }
        case Type::Load: {
          const auto &l =
              derived_cast<const Bytecode::Instruction::Load &>(instr);
          if (address_taken.contains(l.dst)) {
            return false;
          }
          return live.find(l.dst) == live.end();
        }
        case Type::LessThan: {
          const auto &lt =
              derived_cast<const Bytecode::Instruction::LessThan &>(instr);
          if (address_taken.contains(lt.dst)) {
            return false;
          }
          return live.find(lt.dst) == live.end();
        }
        case Type::LessThanImmediate: {
          const auto &lti =
              derived_cast<const Bytecode::Instruction::LessThanImmediate &>(instr);
          if (address_taken.contains(lti.dst)) {
            return false;
          }
          return live.find(lti.dst) == live.end();
        }
        case Type::GreaterThan: {
          const auto &gt =
              derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
          if (address_taken.contains(gt.dst)) {
            return false;
          }
          return live.find(gt.dst) == live.end();
        }
        case Type::GreaterThanImmediate: {
          const auto &gti =
              derived_cast<const Bytecode::Instruction::GreaterThanImmediate &>(instr);
          if (address_taken.contains(gti.dst)) {
            return false;
          }
          return live.find(gti.dst) == live.end();
        }
        case Type::LessThanOrEqual: {
          const auto &lte =
              derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr);
          if (address_taken.contains(lte.dst)) {
            return false;
          }
          return live.find(lte.dst) == live.end();
        }
        case Type::LessThanOrEqualImmediate: {
          const auto &ltei =
              derived_cast<const Bytecode::Instruction::LessThanOrEqualImmediate &>(instr);
          if (address_taken.contains(ltei.dst)) {
            return false;
          }
          return live.find(ltei.dst) == live.end();
        }
        case Type::GreaterThanOrEqual: {
          const auto &gte = derived_cast<
              const Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          if (address_taken.contains(gte.dst)) {
            return false;
          }
          return live.find(gte.dst) == live.end();
        }
        case Type::GreaterThanOrEqualImmediate: {
          const auto &gtei = derived_cast<
              const Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr);
          if (address_taken.contains(gtei.dst)) {
            return false;
          }
          return live.find(gtei.dst) == live.end();
        }
        case Type::Equal: {
          const auto &e =
              derived_cast<const Bytecode::Instruction::Equal &>(instr);
          if (address_taken.contains(e.dst)) {
            return false;
          }
          return live.find(e.dst) == live.end();
        }
        case Type::EqualImmediate: {
          const auto &ei =
              derived_cast<const Bytecode::Instruction::EqualImmediate &>(instr);
          if (address_taken.contains(ei.dst)) {
            return false;
          }
          return live.find(ei.dst) == live.end();
        }
        case Type::NotEqual: {
          const auto &ne =
              derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
          if (address_taken.contains(ne.dst)) {
            return false;
          }
          return live.find(ne.dst) == live.end();
        }
        case Type::NotEqualImmediate: {
          const auto &nei =
              derived_cast<const Bytecode::Instruction::NotEqualImmediate &>(instr);
          if (address_taken.contains(nei.dst)) {
            return false;
          }
          return live.find(nei.dst) == live.end();
        }
        case Type::Add: {
          const auto &a =
              derived_cast<const Bytecode::Instruction::Add &>(instr);
          if (address_taken.contains(a.dst)) {
            return false;
          }
          return live.find(a.dst) == live.end();
        }
        case Type::AddImmediate: {
          const auto &ai =
              derived_cast<const Bytecode::Instruction::AddImmediate &>(instr);
          if (address_taken.contains(ai.dst)) {
            return false;
          }
          return live.find(ai.dst) == live.end();
        }
        case Type::Subtract: {
          const auto &s =
              derived_cast<const Bytecode::Instruction::Subtract &>(instr);
          if (address_taken.contains(s.dst)) {
            return false;
          }
          return live.find(s.dst) == live.end();
        }
        case Type::SubtractImmediate: {
          const auto &si =
              derived_cast<const Bytecode::Instruction::SubtractImmediate &>(instr);
          if (address_taken.contains(si.dst)) {
            return false;
          }
          return live.find(si.dst) == live.end();
        }
        case Type::Multiply: {
          const auto &m =
              derived_cast<const Bytecode::Instruction::Multiply &>(instr);
          if (address_taken.contains(m.dst)) {
            return false;
          }
          return live.find(m.dst) == live.end();
        }
        case Type::MultiplyImmediate: {
          const auto &mi =
              derived_cast<const Bytecode::Instruction::MultiplyImmediate &>(instr);
          if (address_taken.contains(mi.dst)) {
            return false;
          }
          return live.find(mi.dst) == live.end();
        }
        case Type::Divide: {
          const auto &d =
              derived_cast<const Bytecode::Instruction::Divide &>(instr);
          if (address_taken.contains(d.dst)) {
            return false;
          }
          return live.find(d.dst) == live.end();
        }
        case Type::DivideImmediate: {
          const auto &di =
              derived_cast<const Bytecode::Instruction::DivideImmediate &>(instr);
          if (address_taken.contains(di.dst)) {
            return false;
          }
          return live.find(di.dst) == live.end();
        }
        case Type::Modulo: {
          const auto &mo =
              derived_cast<const Bytecode::Instruction::Modulo &>(instr);
          if (address_taken.contains(mo.dst)) {
            return false;
          }
          return live.find(mo.dst) == live.end();
        }
        case Type::ModuloImmediate: {
          const auto &mi =
              derived_cast<const Bytecode::Instruction::ModuloImmediate &>(instr);
          if (address_taken.contains(mi.dst)) {
            return false;
          }
          return live.find(mi.dst) == live.end();
        }
        case Type::ArrayCreate: {
          const auto &ac =
              derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr);
          if (address_taken.contains(ac.dst)) {
            return false;
          }
          return live.find(ac.dst) == live.end();
        }
        case Type::ArrayLiteralCreate: {
          const auto &alc =
              derived_cast<const Bytecode::Instruction::ArrayLiteralCreate &>(instr);
          if (address_taken.contains(alc.dst)) {
            return false;
          }
          return live.find(alc.dst) == live.end();
        }
        case Type::ArrayLoad: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          if (address_taken.contains(al.dst)) {
            return false;
          }
          return live.find(al.dst) == live.end();
        }
        case Type::ArrayLoadImmediate: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoadImmediate &>(instr);
          if (address_taken.contains(al.dst)) {
            return false;
          }
          return live.find(al.dst) == live.end();
        }
        case Type::StructCreate: {
          const auto &sc =
              derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
          if (address_taken.contains(sc.dst)) {
            return false;
          }
          return live.find(sc.dst) == live.end();
        }
        case Type::StructLiteralCreate: {
          const auto &slc =
              derived_cast<const Bytecode::Instruction::StructLiteralCreate &>(instr);
          if (address_taken.contains(slc.dst)) {
            return false;
          }
          return live.find(slc.dst) == live.end();
        }
        case Type::StructLoad: {
          const auto &sl =
              derived_cast<const Bytecode::Instruction::StructLoad &>(instr);
          if (address_taken.contains(sl.dst)) {
            return false;
          }
          return live.find(sl.dst) == live.end();
        }
        case Type::AddressOf: {
          const auto &address_of =
              derived_cast<const Bytecode::Instruction::AddressOf &>(instr);
          if (address_taken.contains(address_of.dst)) {
            return false;
          }
          return live.find(address_of.dst) == live.end();
        }
        case Type::LoadIndirect: {
          const auto &load_indirect =
              derived_cast<const Bytecode::Instruction::LoadIndirect &>(instr);
          if (address_taken.contains(load_indirect.dst)) {
            return false;
          }
          return live.find(load_indirect.dst) == live.end();
        }
        case Type::Negate: {
          const auto &neg =
              derived_cast<const Bytecode::Instruction::Negate &>(instr);
          if (address_taken.contains(neg.dst)) {
            return false;
          }
          return live.find(neg.dst) == live.end();
        }
        case Type::LogicalNot: {
          const auto &ln =
              derived_cast<const Bytecode::Instruction::LogicalNot &>(instr);
          if (address_taken.contains(ln.dst)) {
            return false;
          }
          return live.find(ln.dst) == live.end();
        }
        default:
          return false;
      }
    });
  }
}

}  // namespace kai
