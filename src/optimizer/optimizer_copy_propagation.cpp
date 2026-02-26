#include "../optimizer.h"

#include <algorithm>
#include <unordered_map>

namespace kai {

using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;

void BytecodeOptimizer::copy_propagation(std::vector<Bytecode::BasicBlock> &blocks) {
  for (auto &block : blocks) {
    std::unordered_map<Register, Register> copies;

    auto resolve = [&](Register r) -> Register {
      auto it = copies.find(r);
      return it != copies.end() ? it->second : r;
    };

    auto invalidate = [&](Register dst) {
      std::erase_if(copies, [dst](const auto &p) { return p.second == dst; });
      copies.erase(dst);
    };

    for (auto &instr_ptr : block.instructions) {
      auto &instr = *instr_ptr;
      switch (instr.type()) {
        case Type::Move: {
          auto &m = derived_cast<Bytecode::Instruction::Move &>(instr);
          m.src = resolve(m.src);
          invalidate(m.dst);
          if (m.src != m.dst) {
            copies[m.dst] = m.src;
          }
          break;
        }
        case Type::Load: {
          auto &l = derived_cast<Bytecode::Instruction::Load &>(instr);
          invalidate(l.dst);
          break;
        }
        case Type::LessThan: {
          auto &lt = derived_cast<Bytecode::Instruction::LessThan &>(instr);
          lt.lhs = resolve(lt.lhs);
          lt.rhs = resolve(lt.rhs);
          invalidate(lt.dst);
          break;
        }
        case Type::LessThanImmediate: {
          auto &lti = derived_cast<Bytecode::Instruction::LessThanImmediate &>(instr);
          lti.lhs = resolve(lti.lhs);
          invalidate(lti.dst);
          break;
        }
        case Type::GreaterThan: {
          auto &gt = derived_cast<Bytecode::Instruction::GreaterThan &>(instr);
          gt.lhs = resolve(gt.lhs);
          gt.rhs = resolve(gt.rhs);
          invalidate(gt.dst);
          break;
        }
        case Type::GreaterThanImmediate: {
          auto &gti = derived_cast<Bytecode::Instruction::GreaterThanImmediate &>(instr);
          gti.lhs = resolve(gti.lhs);
          invalidate(gti.dst);
          break;
        }
        case Type::LessThanOrEqual: {
          auto &lte = derived_cast<Bytecode::Instruction::LessThanOrEqual &>(instr);
          lte.lhs = resolve(lte.lhs);
          lte.rhs = resolve(lte.rhs);
          invalidate(lte.dst);
          break;
        }
        case Type::LessThanOrEqualImmediate: {
          auto &ltei =
              derived_cast<Bytecode::Instruction::LessThanOrEqualImmediate &>(instr);
          ltei.lhs = resolve(ltei.lhs);
          invalidate(ltei.dst);
          break;
        }
        case Type::GreaterThanOrEqual: {
          auto &gte =
              derived_cast<Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          gte.lhs = resolve(gte.lhs);
          gte.rhs = resolve(gte.rhs);
          invalidate(gte.dst);
          break;
        }
        case Type::GreaterThanOrEqualImmediate: {
          auto &gtei =
              derived_cast<Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr);
          gtei.lhs = resolve(gtei.lhs);
          invalidate(gtei.dst);
          break;
        }
        case Type::Jump:
          break;
        case Type::JumpConditional: {
          auto &jc = derived_cast<Bytecode::Instruction::JumpConditional &>(instr);
          jc.cond = resolve(jc.cond);
          break;
        }
        case Type::Call: {
          auto &c = derived_cast<Bytecode::Instruction::Call &>(instr);
          for (auto &arg : c.arg_registers) {
            arg = resolve(arg);
          }
          invalidate(c.dst);
          break;
        }
        case Type::TailCall: {
          auto &tc = derived_cast<Bytecode::Instruction::TailCall &>(instr);
          for (auto &arg : tc.arg_registers) {
            arg = resolve(arg);
          }
          break;
        }
        case Type::Return: {
          auto &r = derived_cast<Bytecode::Instruction::Return &>(instr);
          r.reg = resolve(r.reg);
          break;
        }
        case Type::Equal: {
          auto &e = derived_cast<Bytecode::Instruction::Equal &>(instr);
          e.src1 = resolve(e.src1);
          e.src2 = resolve(e.src2);
          invalidate(e.dst);
          break;
        }
        case Type::EqualImmediate: {
          auto &ei = derived_cast<Bytecode::Instruction::EqualImmediate &>(instr);
          ei.src = resolve(ei.src);
          invalidate(ei.dst);
          break;
        }
        case Type::NotEqual: {
          auto &ne = derived_cast<Bytecode::Instruction::NotEqual &>(instr);
          ne.src1 = resolve(ne.src1);
          ne.src2 = resolve(ne.src2);
          invalidate(ne.dst);
          break;
        }
        case Type::NotEqualImmediate: {
          auto &nei = derived_cast<Bytecode::Instruction::NotEqualImmediate &>(instr);
          nei.src = resolve(nei.src);
          invalidate(nei.dst);
          break;
        }
        case Type::Add: {
          auto &a = derived_cast<Bytecode::Instruction::Add &>(instr);
          a.src1 = resolve(a.src1);
          a.src2 = resolve(a.src2);
          invalidate(a.dst);
          break;
        }
        case Type::AddImmediate: {
          auto &ai = derived_cast<Bytecode::Instruction::AddImmediate &>(instr);
          ai.src = resolve(ai.src);
          invalidate(ai.dst);
          break;
        }
        case Type::Subtract: {
          auto &s = derived_cast<Bytecode::Instruction::Subtract &>(instr);
          s.src1 = resolve(s.src1);
          s.src2 = resolve(s.src2);
          invalidate(s.dst);
          break;
        }
        case Type::SubtractImmediate: {
          auto &si = derived_cast<Bytecode::Instruction::SubtractImmediate &>(instr);
          si.src = resolve(si.src);
          invalidate(si.dst);
          break;
        }
        case Type::Multiply: {
          auto &m = derived_cast<Bytecode::Instruction::Multiply &>(instr);
          m.src1 = resolve(m.src1);
          m.src2 = resolve(m.src2);
          invalidate(m.dst);
          break;
        }
        case Type::MultiplyImmediate: {
          auto &mi = derived_cast<Bytecode::Instruction::MultiplyImmediate &>(instr);
          mi.src = resolve(mi.src);
          invalidate(mi.dst);
          break;
        }
        case Type::Divide: {
          auto &d = derived_cast<Bytecode::Instruction::Divide &>(instr);
          d.src1 = resolve(d.src1);
          d.src2 = resolve(d.src2);
          invalidate(d.dst);
          break;
        }
        case Type::DivideImmediate: {
          auto &di = derived_cast<Bytecode::Instruction::DivideImmediate &>(instr);
          di.src = resolve(di.src);
          invalidate(di.dst);
          break;
        }
        case Type::Modulo: {
          auto &mo = derived_cast<Bytecode::Instruction::Modulo &>(instr);
          mo.src1 = resolve(mo.src1);
          mo.src2 = resolve(mo.src2);
          invalidate(mo.dst);
          break;
        }
        case Type::ModuloImmediate: {
          auto &mi = derived_cast<Bytecode::Instruction::ModuloImmediate &>(instr);
          mi.src = resolve(mi.src);
          invalidate(mi.dst);
          break;
        }
        case Type::ArrayCreate: {
          auto &ac = derived_cast<Bytecode::Instruction::ArrayCreate &>(instr);
          for (auto &elem : ac.elements) {
            elem = resolve(elem);
          }
          invalidate(ac.dst);
          break;
        }
        case Type::ArrayLiteralCreate: {
          auto &alc = derived_cast<Bytecode::Instruction::ArrayLiteralCreate &>(instr);
          invalidate(alc.dst);
          break;
        }
        case Type::ArrayLoad: {
          auto &al = derived_cast<Bytecode::Instruction::ArrayLoad &>(instr);
          al.array = resolve(al.array);
          al.index = resolve(al.index);
          invalidate(al.dst);
          break;
        }
        case Type::ArrayLoadImmediate: {
          auto &al = derived_cast<Bytecode::Instruction::ArrayLoadImmediate &>(instr);
          al.array = resolve(al.array);
          invalidate(al.dst);
          break;
        }
        case Type::ArrayStore: {
          auto &as = derived_cast<Bytecode::Instruction::ArrayStore &>(instr);
          as.array = resolve(as.array);
          as.index = resolve(as.index);
          as.value = resolve(as.value);
          break;
        }
        case Type::StructCreate: {
          auto &sc = derived_cast<Bytecode::Instruction::StructCreate &>(instr);
          for (auto &[name, reg] : sc.fields) {
            reg = resolve(reg);
          }
          invalidate(sc.dst);
          break;
        }
        case Type::StructLiteralCreate: {
          auto &slc = derived_cast<Bytecode::Instruction::StructLiteralCreate &>(instr);
          invalidate(slc.dst);
          break;
        }
        case Type::StructLoad: {
          auto &sl = derived_cast<Bytecode::Instruction::StructLoad &>(instr);
          sl.object = resolve(sl.object);
          invalidate(sl.dst);
          break;
        }
        case Type::Negate: {
          auto &neg = derived_cast<Bytecode::Instruction::Negate &>(instr);
          neg.src = resolve(neg.src);
          invalidate(neg.dst);
          break;
        }
        case Type::LogicalNot: {
          auto &ln = derived_cast<Bytecode::Instruction::LogicalNot &>(instr);
          ln.src = resolve(ln.src);
          invalidate(ln.dst);
          break;
        }
      }
    }

    // Remove trivial moves (dst == src after substitution).
    std::erase_if(block.instructions, [](const auto &instr_ptr) {
      if (instr_ptr->type() != Type::Move) {
        return false;
      }
      const auto &m =
          derived_cast<const Bytecode::Instruction::Move &>(*instr_ptr);
      return m.dst == m.src;
    });
  }
}

}  // namespace kai
