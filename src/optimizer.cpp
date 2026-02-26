#include "optimizer.h"

#include <algorithm>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace kai {
namespace bytecode {

using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;

namespace {

static std::optional<Register> get_dst_reg(const Bytecode::Instruction &instr) {
  switch (instr.type()) {
    case Type::Move:
      return derived_cast<const Bytecode::Instruction::Move &>(instr).dst;
    case Type::Load:
      return derived_cast<const Bytecode::Instruction::Load &>(instr).dst;
    case Type::LessThan:
      return derived_cast<const Bytecode::Instruction::LessThan &>(instr).dst;
    case Type::LessThanImmediate:
      return derived_cast<const Bytecode::Instruction::LessThanImmediate &>(instr).dst;
    case Type::GreaterThan:
      return derived_cast<const Bytecode::Instruction::GreaterThan &>(instr).dst;
    case Type::GreaterThanImmediate:
      return derived_cast<const Bytecode::Instruction::GreaterThanImmediate &>(instr).dst;
    case Type::LessThanOrEqual:
      return derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr).dst;
    case Type::LessThanOrEqualImmediate:
      return derived_cast<const Bytecode::Instruction::LessThanOrEqualImmediate &>(instr).dst;
    case Type::GreaterThanOrEqual:
      return derived_cast<const Bytecode::Instruction::GreaterThanOrEqual &>(instr).dst;
    case Type::GreaterThanOrEqualImmediate:
      return derived_cast<const Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr)
          .dst;
    case Type::Equal:
      return derived_cast<const Bytecode::Instruction::Equal &>(instr).dst;
    case Type::EqualImmediate:
      return derived_cast<const Bytecode::Instruction::EqualImmediate &>(instr).dst;
    case Type::NotEqual:
      return derived_cast<const Bytecode::Instruction::NotEqual &>(instr).dst;
    case Type::NotEqualImmediate:
      return derived_cast<const Bytecode::Instruction::NotEqualImmediate &>(instr).dst;
    case Type::Add:
      return derived_cast<const Bytecode::Instruction::Add &>(instr).dst;
    case Type::AddImmediate:
      return derived_cast<const Bytecode::Instruction::AddImmediate &>(instr).dst;
    case Type::Subtract:
      return derived_cast<const Bytecode::Instruction::Subtract &>(instr).dst;
    case Type::SubtractImmediate:
      return derived_cast<const Bytecode::Instruction::SubtractImmediate &>(instr).dst;
    case Type::Multiply:
      return derived_cast<const Bytecode::Instruction::Multiply &>(instr).dst;
    case Type::MultiplyImmediate:
      return derived_cast<const Bytecode::Instruction::MultiplyImmediate &>(instr).dst;
    case Type::Divide:
      return derived_cast<const Bytecode::Instruction::Divide &>(instr).dst;
    case Type::DivideImmediate:
      return derived_cast<const Bytecode::Instruction::DivideImmediate &>(instr).dst;
    case Type::Modulo:
      return derived_cast<const Bytecode::Instruction::Modulo &>(instr).dst;
    case Type::ModuloImmediate:
      return derived_cast<const Bytecode::Instruction::ModuloImmediate &>(instr).dst;
    case Type::Call:
      return derived_cast<const Bytecode::Instruction::Call &>(instr).dst;
    case Type::ArrayCreate:
      return derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr).dst;
    case Type::ArrayLoad:
      return derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr).dst;
    case Type::StructCreate:
      return derived_cast<const Bytecode::Instruction::StructCreate &>(instr).dst;
    case Type::StructLoad:
      return derived_cast<const Bytecode::Instruction::StructLoad &>(instr).dst;
    case Type::Negate:
      return derived_cast<const Bytecode::Instruction::Negate &>(instr).dst;
    case Type::LogicalNot:
      return derived_cast<const Bytecode::Instruction::LogicalNot &>(instr).dst;
    case Type::Jump:
    case Type::JumpConditional:
    case Type::TailCall:
    case Type::Return:
    case Type::ArrayStore:
      return std::nullopt;
  }
  return std::nullopt;
}

// Returns source registers for hoistable instruction types.
// Non-hoistable types (Call, ArrayStore, etc.) are never passed here.
static std::vector<Register> get_src_regs(const Bytecode::Instruction &instr) {
  switch (instr.type()) {
    case Type::Load:
      return {};
    case Type::Move:
      return {derived_cast<const Bytecode::Instruction::Move &>(instr).src};
    case Type::LessThan: {
      const auto &lt = derived_cast<const Bytecode::Instruction::LessThan &>(instr);
      return {lt.lhs, lt.rhs};
    }
    case Type::LessThanImmediate:
      return {derived_cast<const Bytecode::Instruction::LessThanImmediate &>(instr).lhs};
    case Type::GreaterThan: {
      const auto &gt = derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
      return {gt.lhs, gt.rhs};
    }
    case Type::GreaterThanImmediate:
      return {derived_cast<const Bytecode::Instruction::GreaterThanImmediate &>(instr).lhs};
    case Type::LessThanOrEqual: {
      const auto &lte =
          derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr);
      return {lte.lhs, lte.rhs};
    }
    case Type::LessThanOrEqualImmediate:
      return {derived_cast<const Bytecode::Instruction::LessThanOrEqualImmediate &>(instr)
                  .lhs};
    case Type::GreaterThanOrEqual: {
      const auto &gte =
          derived_cast<const Bytecode::Instruction::GreaterThanOrEqual &>(instr);
      return {gte.lhs, gte.rhs};
    }
    case Type::GreaterThanOrEqualImmediate:
      return {derived_cast<const Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr)
                  .lhs};
    case Type::Equal: {
      const auto &e = derived_cast<const Bytecode::Instruction::Equal &>(instr);
      return {e.src1, e.src2};
    }
    case Type::EqualImmediate:
      return {derived_cast<const Bytecode::Instruction::EqualImmediate &>(instr).src};
    case Type::NotEqual: {
      const auto &ne = derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
      return {ne.src1, ne.src2};
    }
    case Type::NotEqualImmediate:
      return {derived_cast<const Bytecode::Instruction::NotEqualImmediate &>(instr).src};
    case Type::Add: {
      const auto &a = derived_cast<const Bytecode::Instruction::Add &>(instr);
      return {a.src1, a.src2};
    }
    case Type::AddImmediate:
      return {derived_cast<const Bytecode::Instruction::AddImmediate &>(instr).src};
    case Type::Subtract: {
      const auto &s = derived_cast<const Bytecode::Instruction::Subtract &>(instr);
      return {s.src1, s.src2};
    }
    case Type::SubtractImmediate:
      return {derived_cast<const Bytecode::Instruction::SubtractImmediate &>(instr).src};
    case Type::Multiply: {
      const auto &m = derived_cast<const Bytecode::Instruction::Multiply &>(instr);
      return {m.src1, m.src2};
    }
    case Type::MultiplyImmediate:
      return {derived_cast<const Bytecode::Instruction::MultiplyImmediate &>(instr).src};
    case Type::Divide: {
      const auto &d = derived_cast<const Bytecode::Instruction::Divide &>(instr);
      return {d.src1, d.src2};
    }
    case Type::DivideImmediate:
      return {derived_cast<const Bytecode::Instruction::DivideImmediate &>(instr).src};
    case Type::Modulo: {
      const auto &mo = derived_cast<const Bytecode::Instruction::Modulo &>(instr);
      return {mo.src1, mo.src2};
    }
    case Type::ModuloImmediate:
      return {derived_cast<const Bytecode::Instruction::ModuloImmediate &>(instr).src};
    case Type::Negate:
      return {derived_cast<const Bytecode::Instruction::Negate &>(instr).src};
    case Type::LogicalNot:
      return {derived_cast<const Bytecode::Instruction::LogicalNot &>(instr).src};
    default:
      return {};
  }
}

static bool is_hoistable(const Bytecode::Instruction &instr,
                          const std::unordered_map<Register, size_t> &def_count) {
  switch (instr.type()) {
    case Type::Load:
    case Type::Move:
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
    case Type::Negate:
    case Type::LogicalNot:
      break;
    default:
      return false;
  }

  auto dst_opt = get_dst_reg(instr);
  if (!dst_opt) return false;

  auto it = def_count.find(*dst_opt);
  if (it == def_count.end() || it->second != 1) return false;

  for (Register src : get_src_regs(instr)) {
    if (def_count.count(src) > 0) return false;
  }

  return true;
}

// Counts how many times each register appears as a source operand across all blocks.
static std::unordered_map<Register, size_t> compute_use_count(
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
        case Type::ArrayLoad: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          use(al.array);
          use(al.index);
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

void BytecodeOptimizer::optimize(std::vector<Bytecode::BasicBlock> &blocks) {
  loop_invariant_code_motion(blocks);
  copy_propagation(blocks);
  dead_code_elimination(blocks);
  tail_call_optimization(blocks);
  peephole(blocks);
  compact_registers(blocks);
}

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
        case Type::ArrayLoad: {
          auto &al = derived_cast<Bytecode::Instruction::ArrayLoad &>(instr);
          al.array = resolve(al.array);
          al.index = resolve(al.index);
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
        case Type::ArrayLoad: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          live.insert(al.array);
          live.insert(al.index);
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
        case Type::ArrayLoad: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          return live.find(al.dst) == live.end();
        }
        case Type::StructCreate: {
          const auto &sc =
              derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
          return live.find(sc.dst) == live.end();
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

void BytecodeOptimizer::loop_invariant_code_motion(
    std::vector<Bytecode::BasicBlock> &blocks) {
  // Step 1: Detect back edges → collect (header H, tail S) loop pairs.
  std::vector<std::pair<size_t, size_t>> loops;
  for (size_t i = 0; i < blocks.size(); ++i) {
    const auto &block = blocks[i];
    if (block.instructions.empty()) continue;
    const auto &last = *block.instructions.back();
    if (last.type() == Type::Jump) {
      const auto &j = derived_cast<const Bytecode::Instruction::Jump &>(last);
      if (static_cast<size_t>(j.label) <= i) {
        loops.push_back({static_cast<size_t>(j.label), i});
      }
    } else if (last.type() == Type::JumpConditional) {
      const auto &jc =
          derived_cast<const Bytecode::Instruction::JumpConditional &>(last);
      if (static_cast<size_t>(jc.label1) <= i) {
        loops.push_back({static_cast<size_t>(jc.label1), i});
      }
      if (static_cast<size_t>(jc.label2) <= i) {
        loops.push_back({static_cast<size_t>(jc.label2), i});
      }
    }
  }

  // Step 2: Hoist loop-invariant instructions to the pre-header for each loop.
  for (const auto &[H, S] : loops) {
    if (H == 0) continue;  // No pre-header exists.

    auto &pre_header = blocks[H - 1];

    // Build def_count: Register → number of definitions in loop blocks H..S.
    std::unordered_map<Register, size_t> def_count;
    for (size_t b = H; b <= S; ++b) {
      for (const auto &instr_ptr : blocks[b].instructions) {
        auto dst_opt = get_dst_reg(*instr_ptr);
        if (dst_opt) {
          def_count[*dst_opt]++;
        }
      }
    }

    // Iteratively hoist: scan H..S, hoist the first hoistable instruction
    // found, update def_count, then restart from H.
    bool changed = true;
    while (changed) {
      changed = false;
      for (size_t b = H; b <= S && !changed; ++b) {
        auto &blk = blocks[b];
        for (size_t j = 0; j < blk.instructions.size() && !changed; ++j) {
          if (!is_hoistable(*blk.instructions[j], def_count)) continue;

          // Remove from loop block.
          auto instr_ptr = std::move(blk.instructions[j]);
          blk.instructions.erase(blk.instructions.begin() +
                                  static_cast<std::ptrdiff_t>(j));

          // Update def_count for the hoisted instruction's dst.
          auto dst_opt = get_dst_reg(*instr_ptr);
          if (dst_opt) {
            auto cnt_it = def_count.find(*dst_opt);
            if (cnt_it != def_count.end()) {
              if (--cnt_it->second == 0) def_count.erase(cnt_it);
            }
          }

          // Insert before the terminator of the pre-header.
          auto insert_it = pre_header.instructions.end();
          if (!pre_header.instructions.empty()) {
            const auto last_type = pre_header.instructions.back()->type();
            if (last_type == Type::Jump || last_type == Type::JumpConditional ||
                last_type == Type::Return) {
              --insert_it;
            }
          }
          pre_header.instructions.insert(insert_it, std::move(instr_ptr));

          changed = true;
        }
      }
    }
  }
}

void BytecodeOptimizer::peephole(std::vector<Bytecode::BasicBlock> &blocks) {
  // Build a global count of how many times each register is read as a source
  // operand.  This is used to prove that a temporary register is used only
  // once (by the immediately-following Move) before we eliminate the Move.
  const auto use_count = compute_use_count(blocks);

  const auto is_foldable_producer = [](Type t) {
    switch (t) {
      case Type::Load:
      case Type::LessThanImmediate:
      case Type::GreaterThanImmediate:
      case Type::LessThanOrEqualImmediate:
      case Type::GreaterThanOrEqualImmediate:
      case Type::EqualImmediate:
      case Type::NotEqualImmediate:
      case Type::AddImmediate:
      case Type::SubtractImmediate:
      case Type::MultiplyImmediate:
      case Type::DivideImmediate:
      case Type::ModuloImmediate:
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
      case Type::LessThanImmediate:
        derived_cast<Bytecode::Instruction::LessThanImmediate &>(instr).dst = dst;
        break;
      case Type::GreaterThanImmediate:
        derived_cast<Bytecode::Instruction::GreaterThanImmediate &>(instr).dst = dst;
        break;
      case Type::LessThanOrEqualImmediate:
        derived_cast<Bytecode::Instruction::LessThanOrEqualImmediate &>(instr).dst = dst;
        break;
      case Type::GreaterThanOrEqualImmediate:
        derived_cast<Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr).dst = dst;
        break;
      case Type::EqualImmediate:
        derived_cast<Bytecode::Instruction::EqualImmediate &>(instr).dst = dst;
        break;
      case Type::NotEqualImmediate:
        derived_cast<Bytecode::Instruction::NotEqualImmediate &>(instr).dst = dst;
        break;
      case Type::AddImmediate:
        derived_cast<Bytecode::Instruction::AddImmediate &>(instr).dst = dst;
        break;
      case Type::SubtractImmediate:
        derived_cast<Bytecode::Instruction::SubtractImmediate &>(instr).dst = dst;
        break;
      case Type::MultiplyImmediate:
        derived_cast<Bytecode::Instruction::MultiplyImmediate &>(instr).dst = dst;
        break;
      case Type::DivideImmediate:
        derived_cast<Bytecode::Instruction::DivideImmediate &>(instr).dst = dst;
        break;
      case Type::ModuloImmediate:
        derived_cast<Bytecode::Instruction::ModuloImmediate &>(instr).dst = dst;
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
        case Type::ArrayLoad: {
          const auto &al =
              derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          track(al.dst);
          track(al.array);
          track(al.index);
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
        case Type::StructLoad: {
          const auto &sl =
              derived_cast<const Bytecode::Instruction::StructLoad &>(instr);
          track(sl.dst);
          track(sl.object);
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
        case Type::ArrayLoad: {
          auto &al = derived_cast<Bytecode::Instruction::ArrayLoad &>(instr);
          al.dst = remap(al.dst);
          al.array = remap(al.array);
          al.index = remap(al.index);
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
        case Type::StructLoad: {
          auto &sl = derived_cast<Bytecode::Instruction::StructLoad &>(instr);
          sl.dst = remap(sl.dst);
          sl.object = remap(sl.object);
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

}  // namespace bytecode
}  // namespace kai
