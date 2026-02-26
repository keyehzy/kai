#include "../optimizer.h"

#include <algorithm>
#include <unordered_set>

namespace kai {

using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;

void BytecodeOptimizer::dead_code_elimination(
    std::vector<Bytecode::BasicBlock> &blocks) {
  std::unordered_set<Register> live;

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
          return live.find(m.dst) == live.end();
        }
        case Type::Load: {
          const auto &l =
              derived_cast<const Bytecode::Instruction::Load &>(instr);
          return live.find(l.dst) == live.end();
        }
        case Type::LessThan: {
          const auto &lt =
              derived_cast<const Bytecode::Instruction::LessThan &>(instr);
          return live.find(lt.dst) == live.end();
        }
        case Type::LessThanImmediate: {
          const auto &lti =
              derived_cast<const Bytecode::Instruction::LessThanImmediate &>(instr);
          return live.find(lti.dst) == live.end();
        }
        case Type::GreaterThan: {
          const auto &gt =
              derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
          return live.find(gt.dst) == live.end();
        }
        case Type::GreaterThanImmediate: {
          const auto &gti =
              derived_cast<const Bytecode::Instruction::GreaterThanImmediate &>(instr);
          return live.find(gti.dst) == live.end();
        }
        case Type::LessThanOrEqual: {
          const auto &lte =
              derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr);
          return live.find(lte.dst) == live.end();
        }
        case Type::LessThanOrEqualImmediate: {
          const auto &ltei =
              derived_cast<const Bytecode::Instruction::LessThanOrEqualImmediate &>(instr);
          return live.find(ltei.dst) == live.end();
        }
        case Type::GreaterThanOrEqual: {
          const auto &gte = derived_cast<
              const Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          return live.find(gte.dst) == live.end();
        }
        case Type::GreaterThanOrEqualImmediate: {
          const auto &gtei = derived_cast<
              const Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr);
          return live.find(gtei.dst) == live.end();
        }
        case Type::Equal: {
          const auto &e =
              derived_cast<const Bytecode::Instruction::Equal &>(instr);
          return live.find(e.dst) == live.end();
        }
        case Type::EqualImmediate: {
          const auto &ei =
              derived_cast<const Bytecode::Instruction::EqualImmediate &>(instr);
          return live.find(ei.dst) == live.end();
        }
        case Type::NotEqual: {
          const auto &ne =
              derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
          return live.find(ne.dst) == live.end();
        }
        case Type::NotEqualImmediate: {
          const auto &nei =
              derived_cast<const Bytecode::Instruction::NotEqualImmediate &>(instr);
          return live.find(nei.dst) == live.end();
        }
        case Type::Add: {
          const auto &a =
              derived_cast<const Bytecode::Instruction::Add &>(instr);
          return live.find(a.dst) == live.end();
        }
        case Type::AddImmediate: {
          const auto &ai =
              derived_cast<const Bytecode::Instruction::AddImmediate &>(instr);
          return live.find(ai.dst) == live.end();
        }
        case Type::Subtract: {
          const auto &s =
              derived_cast<const Bytecode::Instruction::Subtract &>(instr);
          return live.find(s.dst) == live.end();
        }
        case Type::SubtractImmediate: {
          const auto &si =
              derived_cast<const Bytecode::Instruction::SubtractImmediate &>(instr);
          return live.find(si.dst) == live.end();
        }
        case Type::Multiply: {
          const auto &m =
              derived_cast<const Bytecode::Instruction::Multiply &>(instr);
          return live.find(m.dst) == live.end();
        }
        case Type::MultiplyImmediate: {
          const auto &mi =
              derived_cast<const Bytecode::Instruction::MultiplyImmediate &>(instr);
          return live.find(mi.dst) == live.end();
        }
        case Type::Divide: {
          const auto &d =
              derived_cast<const Bytecode::Instruction::Divide &>(instr);
          return live.find(d.dst) == live.end();
        }
        case Type::DivideImmediate: {
          const auto &di =
              derived_cast<const Bytecode::Instruction::DivideImmediate &>(instr);
          return live.find(di.dst) == live.end();
        }
        case Type::Modulo: {
          const auto &mo =
              derived_cast<const Bytecode::Instruction::Modulo &>(instr);
          return live.find(mo.dst) == live.end();
        }
        case Type::ModuloImmediate: {
          const auto &mi =
              derived_cast<const Bytecode::Instruction::ModuloImmediate &>(instr);
          return live.find(mi.dst) == live.end();
        }
        case Type::ArrayCreate: {
          const auto &ac =
              derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr);
          return live.find(ac.dst) == live.end();
        }
        case Type::ArrayLiteralCreate: {
          const auto &alc =
              derived_cast<const Bytecode::Instruction::ArrayLiteralCreate &>(instr);
          return live.find(alc.dst) == live.end();
        }
        case Type::ArrayLoad: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          return live.find(al.dst) == live.end();
        }
        case Type::ArrayLoadImmediate: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoadImmediate &>(instr);
          return live.find(al.dst) == live.end();
        }
        case Type::StructCreate: {
          const auto &sc =
              derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
          return live.find(sc.dst) == live.end();
        }
        case Type::StructLiteralCreate: {
          const auto &slc =
              derived_cast<const Bytecode::Instruction::StructLiteralCreate &>(instr);
          return live.find(slc.dst) == live.end();
        }
        case Type::StructLoad: {
          const auto &sl =
              derived_cast<const Bytecode::Instruction::StructLoad &>(instr);
          return live.find(sl.dst) == live.end();
        }
        case Type::Negate: {
          const auto &neg =
              derived_cast<const Bytecode::Instruction::Negate &>(instr);
          return live.find(neg.dst) == live.end();
        }
        case Type::LogicalNot: {
          const auto &ln =
              derived_cast<const Bytecode::Instruction::LogicalNot &>(instr);
          return live.find(ln.dst) == live.end();
        }
        default:
          return false;
      }
    });
  }
}

}  // namespace kai
