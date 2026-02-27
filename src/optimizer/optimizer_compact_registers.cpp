#include "../optimizer.h"

#include <algorithm>
#include <unordered_map>

namespace kai {

using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;

void BytecodeOptimizer::compact_registers(std::vector<Bytecode::BasicBlock> &blocks) {
  std::vector<Register> regs;
  regs.reserve(64);

  const auto track = [&regs](Register r) { regs.push_back(r); };

  for (const auto &block : blocks) {
    for (const auto &instr_ptr : block.instructions) {
      const auto &instr = *instr_ptr;
      switch (instr.type()) {
        case Type::Move: {
          const auto &m = derived_cast<const Bytecode::Instruction::Move &>(instr);
          track(m.dst);
          track(m.src);
          break;
        }
        case Type::Load:
          track(derived_cast<const Bytecode::Instruction::Load &>(instr).dst);
          break;
        case Type::LessThan: {
          const auto &lt = derived_cast<const Bytecode::Instruction::LessThan &>(instr);
          track(lt.dst);
          track(lt.lhs);
          track(lt.rhs);
          break;
        }
        case Type::LessThanImmediate: {
          const auto &lti =
              derived_cast<const Bytecode::Instruction::LessThanImmediate &>(instr);
          track(lti.dst);
          track(lti.lhs);
          break;
        }
        case Type::GreaterThan: {
          const auto &gt =
              derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
          track(gt.dst);
          track(gt.lhs);
          track(gt.rhs);
          break;
        }
        case Type::GreaterThanImmediate: {
          const auto &gti =
              derived_cast<const Bytecode::Instruction::GreaterThanImmediate &>(instr);
          track(gti.dst);
          track(gti.lhs);
          break;
        }
        case Type::LessThanOrEqual: {
          const auto &lte =
              derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr);
          track(lte.dst);
          track(lte.lhs);
          track(lte.rhs);
          break;
        }
        case Type::LessThanOrEqualImmediate: {
          const auto &ltei =
              derived_cast<const Bytecode::Instruction::LessThanOrEqualImmediate &>(instr);
          track(ltei.dst);
          track(ltei.lhs);
          break;
        }
        case Type::GreaterThanOrEqual: {
          const auto &gte =
              derived_cast<const Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          track(gte.dst);
          track(gte.lhs);
          track(gte.rhs);
          break;
        }
        case Type::GreaterThanOrEqualImmediate: {
          const auto &gtei = derived_cast<
              const Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr);
          track(gtei.dst);
          track(gtei.lhs);
          break;
        }
        case Type::Jump:
          break;
        case Type::JumpConditional:
          track(derived_cast<const Bytecode::Instruction::JumpConditional &>(instr).cond);
          break;
        case Type::JumpEqualImmediate:
          track(derived_cast<const Bytecode::Instruction::JumpEqualImmediate &>(instr).src);
          break;
        case Type::JumpGreaterThanImmediate:
          track(
              derived_cast<const Bytecode::Instruction::JumpGreaterThanImmediate &>(instr).lhs);
          break;
        case Type::JumpLessThanOrEqual: {
          const auto &jump_lte =
              derived_cast<const Bytecode::Instruction::JumpLessThanOrEqual &>(instr);
          track(jump_lte.lhs);
          track(jump_lte.rhs);
          break;
        }
        case Type::Call: {
          const auto &c = derived_cast<const Bytecode::Instruction::Call &>(instr);
          track(c.dst);
          for (auto r : c.arg_registers) track(r);
          for (auto r : c.param_registers) track(r);
          break;
        }
        case Type::TailCall: {
          const auto &tc =
              derived_cast<const Bytecode::Instruction::TailCall &>(instr);
          for (auto r : tc.arg_registers) track(r);
          for (auto r : tc.param_registers) track(r);
          break;
        }
        case Type::Return:
          track(derived_cast<const Bytecode::Instruction::Return &>(instr).reg);
          break;
        case Type::Equal: {
          const auto &e = derived_cast<const Bytecode::Instruction::Equal &>(instr);
          track(e.dst);
          track(e.src1);
          track(e.src2);
          break;
        }
        case Type::EqualImmediate: {
          const auto &ei =
              derived_cast<const Bytecode::Instruction::EqualImmediate &>(instr);
          track(ei.dst);
          track(ei.src);
          break;
        }
        case Type::NotEqual: {
          const auto &ne =
              derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
          track(ne.dst);
          track(ne.src1);
          track(ne.src2);
          break;
        }
        case Type::NotEqualImmediate: {
          const auto &nei =
              derived_cast<const Bytecode::Instruction::NotEqualImmediate &>(instr);
          track(nei.dst);
          track(nei.src);
          break;
        }
        case Type::Add: {
          const auto &a = derived_cast<const Bytecode::Instruction::Add &>(instr);
          track(a.dst);
          track(a.src1);
          track(a.src2);
          break;
        }
        case Type::AddImmediate: {
          const auto &ai =
              derived_cast<const Bytecode::Instruction::AddImmediate &>(instr);
          track(ai.dst);
          track(ai.src);
          break;
        }
        case Type::Subtract: {
          const auto &s = derived_cast<const Bytecode::Instruction::Subtract &>(instr);
          track(s.dst);
          track(s.src1);
          track(s.src2);
          break;
        }
        case Type::SubtractImmediate: {
          const auto &si =
              derived_cast<const Bytecode::Instruction::SubtractImmediate &>(instr);
          track(si.dst);
          track(si.src);
          break;
        }
        case Type::Multiply: {
          const auto &m =
              derived_cast<const Bytecode::Instruction::Multiply &>(instr);
          track(m.dst);
          track(m.src1);
          track(m.src2);
          break;
        }
        case Type::MultiplyImmediate: {
          const auto &mi =
              derived_cast<const Bytecode::Instruction::MultiplyImmediate &>(instr);
          track(mi.dst);
          track(mi.src);
          break;
        }
        case Type::Divide: {
          const auto &d = derived_cast<const Bytecode::Instruction::Divide &>(instr);
          track(d.dst);
          track(d.src1);
          track(d.src2);
          break;
        }
        case Type::DivideImmediate: {
          const auto &di =
              derived_cast<const Bytecode::Instruction::DivideImmediate &>(instr);
          track(di.dst);
          track(di.src);
          break;
        }
        case Type::Modulo: {
          const auto &mo = derived_cast<const Bytecode::Instruction::Modulo &>(instr);
          track(mo.dst);
          track(mo.src1);
          track(mo.src2);
          break;
        }
        case Type::ModuloImmediate: {
          const auto &mi =
              derived_cast<const Bytecode::Instruction::ModuloImmediate &>(instr);
          track(mi.dst);
          track(mi.src);
          break;
        }
        case Type::ArrayCreate: {
          const auto &ac =
              derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr);
          track(ac.dst);
          for (auto elem : ac.elements) track(elem);
          break;
        }
        case Type::ArrayLiteralCreate: {
          const auto &alc =
              derived_cast<const Bytecode::Instruction::ArrayLiteralCreate &>(instr);
          track(alc.dst);
          break;
        }
        case Type::ArrayLoad: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          track(al.dst);
          track(al.array);
          track(al.index);
          break;
        }
        case Type::ArrayLoadImmediate: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoadImmediate &>(instr);
          track(al.dst);
          track(al.array);
          break;
        }
        case Type::ArrayStore: {
          const auto &as = derived_cast<const Bytecode::Instruction::ArrayStore &>(instr);
          track(as.array);
          track(as.index);
          track(as.value);
          break;
        }
        case Type::StructCreate: {
          const auto &sc =
              derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
          track(sc.dst);
          for (const auto &[name, reg] : sc.fields) track(reg);
          break;
        }
        case Type::StructLiteralCreate: {
          const auto &slc =
              derived_cast<const Bytecode::Instruction::StructLiteralCreate &>(instr);
          track(slc.dst);
          break;
        }
        case Type::StructLoad: {
          const auto &sl =
              derived_cast<const Bytecode::Instruction::StructLoad &>(instr);
          track(sl.dst);
          track(sl.object);
          break;
        }
        case Type::AddressOf: {
          const auto &address_of =
              derived_cast<const Bytecode::Instruction::AddressOf &>(instr);
          track(address_of.dst);
          track(address_of.src);
          break;
        }
        case Type::LoadIndirect: {
          const auto &load_indirect =
              derived_cast<const Bytecode::Instruction::LoadIndirect &>(instr);
          track(load_indirect.dst);
          track(load_indirect.pointer);
          break;
        }
        case Type::Negate: {
          const auto &n = derived_cast<const Bytecode::Instruction::Negate &>(instr);
          track(n.dst);
          track(n.src);
          break;
        }
        case Type::LogicalNot: {
          const auto &ln =
              derived_cast<const Bytecode::Instruction::LogicalNot &>(instr);
          track(ln.dst);
          track(ln.src);
          break;
        }
      }
    }
  }

  if (regs.empty()) {
    return;
  }

  std::sort(regs.begin(), regs.end());
  regs.erase(std::unique(regs.begin(), regs.end()), regs.end());

  bool already_dense = true;
  for (size_t i = 0; i < regs.size(); ++i) {
    if (regs[i] != static_cast<Register>(i)) {
      already_dense = false;
      break;
    }
  }
  if (already_dense) {
    return;
  }

  std::unordered_map<Register, Register> mapping;
  mapping.reserve(regs.size());
  for (size_t i = 0; i < regs.size(); ++i) {
    mapping[regs[i]] = static_cast<Register>(i);
  }

  const auto remap = [&mapping](Register r) {
    const auto it = mapping.find(r);
    return it != mapping.end() ? it->second : r;
  };

  for (auto &block : blocks) {
    for (auto &instr_ptr : block.instructions) {
      auto &instr = *instr_ptr;
      switch (instr.type()) {
        case Type::Move: {
          auto &m = derived_cast<Bytecode::Instruction::Move &>(instr);
          m.dst = remap(m.dst);
          m.src = remap(m.src);
          break;
        }
        case Type::Load: {
          auto &l = derived_cast<Bytecode::Instruction::Load &>(instr);
          l.dst = remap(l.dst);
          break;
        }
        case Type::LessThan: {
          auto &lt = derived_cast<Bytecode::Instruction::LessThan &>(instr);
          lt.dst = remap(lt.dst);
          lt.lhs = remap(lt.lhs);
          lt.rhs = remap(lt.rhs);
          break;
        }
        case Type::LessThanImmediate: {
          auto &lti = derived_cast<Bytecode::Instruction::LessThanImmediate &>(instr);
          lti.dst = remap(lti.dst);
          lti.lhs = remap(lti.lhs);
          break;
        }
        case Type::GreaterThan: {
          auto &gt = derived_cast<Bytecode::Instruction::GreaterThan &>(instr);
          gt.dst = remap(gt.dst);
          gt.lhs = remap(gt.lhs);
          gt.rhs = remap(gt.rhs);
          break;
        }
        case Type::GreaterThanImmediate: {
          auto &gti = derived_cast<Bytecode::Instruction::GreaterThanImmediate &>(instr);
          gti.dst = remap(gti.dst);
          gti.lhs = remap(gti.lhs);
          break;
        }
        case Type::LessThanOrEqual: {
          auto &lte = derived_cast<Bytecode::Instruction::LessThanOrEqual &>(instr);
          lte.dst = remap(lte.dst);
          lte.lhs = remap(lte.lhs);
          lte.rhs = remap(lte.rhs);
          break;
        }
        case Type::LessThanOrEqualImmediate: {
          auto &ltei =
              derived_cast<Bytecode::Instruction::LessThanOrEqualImmediate &>(instr);
          ltei.dst = remap(ltei.dst);
          ltei.lhs = remap(ltei.lhs);
          break;
        }
        case Type::GreaterThanOrEqual: {
          auto &gte =
              derived_cast<Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          gte.dst = remap(gte.dst);
          gte.lhs = remap(gte.lhs);
          gte.rhs = remap(gte.rhs);
          break;
        }
        case Type::GreaterThanOrEqualImmediate: {
          auto &gtei = derived_cast<Bytecode::Instruction::GreaterThanOrEqualImmediate &>(
              instr);
          gtei.dst = remap(gtei.dst);
          gtei.lhs = remap(gtei.lhs);
          break;
        }
        case Type::Jump:
          break;
        case Type::JumpConditional: {
          auto &jc = derived_cast<Bytecode::Instruction::JumpConditional &>(instr);
          jc.cond = remap(jc.cond);
          break;
        }
        case Type::JumpEqualImmediate: {
          auto &jump_equal_imm =
              derived_cast<Bytecode::Instruction::JumpEqualImmediate &>(instr);
          jump_equal_imm.src = remap(jump_equal_imm.src);
          break;
        }
        case Type::JumpGreaterThanImmediate: {
          auto &jump_greater_than_imm =
              derived_cast<Bytecode::Instruction::JumpGreaterThanImmediate &>(instr);
          jump_greater_than_imm.lhs = remap(jump_greater_than_imm.lhs);
          break;
        }
        case Type::JumpLessThanOrEqual: {
          auto &jump_lte = derived_cast<Bytecode::Instruction::JumpLessThanOrEqual &>(instr);
          jump_lte.lhs = remap(jump_lte.lhs);
          jump_lte.rhs = remap(jump_lte.rhs);
          break;
        }
        case Type::Call: {
          auto &c = derived_cast<Bytecode::Instruction::Call &>(instr);
          c.dst = remap(c.dst);
          for (auto &arg : c.arg_registers) {
            arg = remap(arg);
          }
          for (auto &param : c.param_registers) {
            param = remap(param);
          }
          break;
        }
        case Type::TailCall: {
          auto &tc = derived_cast<Bytecode::Instruction::TailCall &>(instr);
          for (auto &arg : tc.arg_registers) {
            arg = remap(arg);
          }
          for (auto &param : tc.param_registers) {
            param = remap(param);
          }
          break;
        }
        case Type::Return: {
          auto &r = derived_cast<Bytecode::Instruction::Return &>(instr);
          r.reg = remap(r.reg);
          break;
        }
        case Type::Equal: {
          auto &e = derived_cast<Bytecode::Instruction::Equal &>(instr);
          e.dst = remap(e.dst);
          e.src1 = remap(e.src1);
          e.src2 = remap(e.src2);
          break;
        }
        case Type::EqualImmediate: {
          auto &ei = derived_cast<Bytecode::Instruction::EqualImmediate &>(instr);
          ei.dst = remap(ei.dst);
          ei.src = remap(ei.src);
          break;
        }
        case Type::NotEqual: {
          auto &ne = derived_cast<Bytecode::Instruction::NotEqual &>(instr);
          ne.dst = remap(ne.dst);
          ne.src1 = remap(ne.src1);
          ne.src2 = remap(ne.src2);
          break;
        }
        case Type::NotEqualImmediate: {
          auto &nei = derived_cast<Bytecode::Instruction::NotEqualImmediate &>(instr);
          nei.dst = remap(nei.dst);
          nei.src = remap(nei.src);
          break;
        }
        case Type::Add: {
          auto &a = derived_cast<Bytecode::Instruction::Add &>(instr);
          a.dst = remap(a.dst);
          a.src1 = remap(a.src1);
          a.src2 = remap(a.src2);
          break;
        }
        case Type::AddImmediate: {
          auto &ai = derived_cast<Bytecode::Instruction::AddImmediate &>(instr);
          ai.dst = remap(ai.dst);
          ai.src = remap(ai.src);
          break;
        }
        case Type::Subtract: {
          auto &s = derived_cast<Bytecode::Instruction::Subtract &>(instr);
          s.dst = remap(s.dst);
          s.src1 = remap(s.src1);
          s.src2 = remap(s.src2);
          break;
        }
        case Type::SubtractImmediate: {
          auto &si = derived_cast<Bytecode::Instruction::SubtractImmediate &>(instr);
          si.dst = remap(si.dst);
          si.src = remap(si.src);
          break;
        }
        case Type::Multiply: {
          auto &m = derived_cast<Bytecode::Instruction::Multiply &>(instr);
          m.dst = remap(m.dst);
          m.src1 = remap(m.src1);
          m.src2 = remap(m.src2);
          break;
        }
        case Type::MultiplyImmediate: {
          auto &mi = derived_cast<Bytecode::Instruction::MultiplyImmediate &>(instr);
          mi.dst = remap(mi.dst);
          mi.src = remap(mi.src);
          break;
        }
        case Type::Divide: {
          auto &d = derived_cast<Bytecode::Instruction::Divide &>(instr);
          d.dst = remap(d.dst);
          d.src1 = remap(d.src1);
          d.src2 = remap(d.src2);
          break;
        }
        case Type::DivideImmediate: {
          auto &di = derived_cast<Bytecode::Instruction::DivideImmediate &>(instr);
          di.dst = remap(di.dst);
          di.src = remap(di.src);
          break;
        }
        case Type::Modulo: {
          auto &mo = derived_cast<Bytecode::Instruction::Modulo &>(instr);
          mo.dst = remap(mo.dst);
          mo.src1 = remap(mo.src1);
          mo.src2 = remap(mo.src2);
          break;
        }
        case Type::ModuloImmediate: {
          auto &mi = derived_cast<Bytecode::Instruction::ModuloImmediate &>(instr);
          mi.dst = remap(mi.dst);
          mi.src = remap(mi.src);
          break;
        }
        case Type::ArrayCreate: {
          auto &ac = derived_cast<Bytecode::Instruction::ArrayCreate &>(instr);
          ac.dst = remap(ac.dst);
          for (auto &elem : ac.elements) {
            elem = remap(elem);
          }
          break;
        }
        case Type::ArrayLiteralCreate: {
          auto &alc = derived_cast<Bytecode::Instruction::ArrayLiteralCreate &>(instr);
          alc.dst = remap(alc.dst);
          break;
        }
        case Type::ArrayLoad: {
          auto &al = derived_cast<Bytecode::Instruction::ArrayLoad &>(instr);
          al.dst = remap(al.dst);
          al.array = remap(al.array);
          al.index = remap(al.index);
          break;
        }
        case Type::ArrayLoadImmediate: {
          auto &al = derived_cast<Bytecode::Instruction::ArrayLoadImmediate &>(instr);
          al.dst = remap(al.dst);
          al.array = remap(al.array);
          break;
        }
        case Type::ArrayStore: {
          auto &as = derived_cast<Bytecode::Instruction::ArrayStore &>(instr);
          as.array = remap(as.array);
          as.index = remap(as.index);
          as.value = remap(as.value);
          break;
        }
        case Type::StructCreate: {
          auto &sc = derived_cast<Bytecode::Instruction::StructCreate &>(instr);
          sc.dst = remap(sc.dst);
          for (auto &[name, reg] : sc.fields) {
            reg = remap(reg);
          }
          break;
        }
        case Type::StructLiteralCreate: {
          auto &slc = derived_cast<Bytecode::Instruction::StructLiteralCreate &>(instr);
          slc.dst = remap(slc.dst);
          break;
        }
        case Type::StructLoad: {
          auto &sl = derived_cast<Bytecode::Instruction::StructLoad &>(instr);
          sl.dst = remap(sl.dst);
          sl.object = remap(sl.object);
          break;
        }
        case Type::AddressOf: {
          auto &address_of = derived_cast<Bytecode::Instruction::AddressOf &>(instr);
          address_of.dst = remap(address_of.dst);
          address_of.src = remap(address_of.src);
          break;
        }
        case Type::LoadIndirect: {
          auto &load_indirect = derived_cast<Bytecode::Instruction::LoadIndirect &>(instr);
          load_indirect.dst = remap(load_indirect.dst);
          load_indirect.pointer = remap(load_indirect.pointer);
          break;
        }
        case Type::Negate: {
          auto &n = derived_cast<Bytecode::Instruction::Negate &>(instr);
          n.dst = remap(n.dst);
          n.src = remap(n.src);
          break;
        }
        case Type::LogicalNot: {
          auto &ln = derived_cast<Bytecode::Instruction::LogicalNot &>(instr);
          ln.dst = remap(ln.dst);
          ln.src = remap(ln.src);
          break;
        }
      }
    }
  }
}

}  // namespace kai
