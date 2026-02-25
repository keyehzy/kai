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
      return ast::derived_cast<const Bytecode::Instruction::Move &>(instr).dst;
    case Type::Load:
      return ast::derived_cast<const Bytecode::Instruction::Load &>(instr).dst;
    case Type::LessThan:
      return ast::derived_cast<const Bytecode::Instruction::LessThan &>(instr).dst;
    case Type::GreaterThan:
      return ast::derived_cast<const Bytecode::Instruction::GreaterThan &>(instr).dst;
    case Type::LessThanOrEqual:
      return ast::derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr).dst;
    case Type::GreaterThanOrEqual:
      return ast::derived_cast<const Bytecode::Instruction::GreaterThanOrEqual &>(instr).dst;
    case Type::Equal:
      return ast::derived_cast<const Bytecode::Instruction::Equal &>(instr).dst;
    case Type::NotEqual:
      return ast::derived_cast<const Bytecode::Instruction::NotEqual &>(instr).dst;
    case Type::Add:
      return ast::derived_cast<const Bytecode::Instruction::Add &>(instr).dst;
    case Type::Subtract:
      return ast::derived_cast<const Bytecode::Instruction::Subtract &>(instr).dst;
    case Type::AddImmediate:
      return ast::derived_cast<const Bytecode::Instruction::AddImmediate &>(instr).dst;
    case Type::Multiply:
      return ast::derived_cast<const Bytecode::Instruction::Multiply &>(instr).dst;
    case Type::Divide:
      return ast::derived_cast<const Bytecode::Instruction::Divide &>(instr).dst;
    case Type::Modulo:
      return ast::derived_cast<const Bytecode::Instruction::Modulo &>(instr).dst;
    case Type::Call:
      return ast::derived_cast<const Bytecode::Instruction::Call &>(instr).dst;
    case Type::ArrayCreate:
      return ast::derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr).dst;
    case Type::ArrayLoad:
      return ast::derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr).dst;
    case Type::StructCreate:
      return ast::derived_cast<const Bytecode::Instruction::StructCreate &>(instr).dst;
    case Type::StructLoad:
      return ast::derived_cast<const Bytecode::Instruction::StructLoad &>(instr).dst;
    case Type::Negate:
      return ast::derived_cast<const Bytecode::Instruction::Negate &>(instr).dst;
    case Type::LogicalNot:
      return ast::derived_cast<const Bytecode::Instruction::LogicalNot &>(instr).dst;
    case Type::Jump:
    case Type::JumpConditional:
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
      return {ast::derived_cast<const Bytecode::Instruction::Move &>(instr).src};
    case Type::LessThan: {
      const auto &lt = ast::derived_cast<const Bytecode::Instruction::LessThan &>(instr);
      return {lt.lhs, lt.rhs};
    }
    case Type::GreaterThan: {
      const auto &gt = ast::derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
      return {gt.lhs, gt.rhs};
    }
    case Type::LessThanOrEqual: {
      const auto &lte =
          ast::derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr);
      return {lte.lhs, lte.rhs};
    }
    case Type::GreaterThanOrEqual: {
      const auto &gte =
          ast::derived_cast<const Bytecode::Instruction::GreaterThanOrEqual &>(instr);
      return {gte.lhs, gte.rhs};
    }
    case Type::Equal: {
      const auto &e = ast::derived_cast<const Bytecode::Instruction::Equal &>(instr);
      return {e.src1, e.src2};
    }
    case Type::NotEqual: {
      const auto &ne = ast::derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
      return {ne.src1, ne.src2};
    }
    case Type::Add: {
      const auto &a = ast::derived_cast<const Bytecode::Instruction::Add &>(instr);
      return {a.src1, a.src2};
    }
    case Type::Subtract: {
      const auto &s = ast::derived_cast<const Bytecode::Instruction::Subtract &>(instr);
      return {s.src1, s.src2};
    }
    case Type::AddImmediate:
      return {ast::derived_cast<const Bytecode::Instruction::AddImmediate &>(instr).src};
    case Type::Multiply: {
      const auto &m = ast::derived_cast<const Bytecode::Instruction::Multiply &>(instr);
      return {m.src1, m.src2};
    }
    case Type::Divide: {
      const auto &d = ast::derived_cast<const Bytecode::Instruction::Divide &>(instr);
      return {d.src1, d.src2};
    }
    case Type::Modulo: {
      const auto &mo = ast::derived_cast<const Bytecode::Instruction::Modulo &>(instr);
      return {mo.src1, mo.src2};
    }
    case Type::Negate:
      return {ast::derived_cast<const Bytecode::Instruction::Negate &>(instr).src};
    case Type::LogicalNot:
      return {ast::derived_cast<const Bytecode::Instruction::LogicalNot &>(instr).src};
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
    case Type::Subtract:
    case Type::AddImmediate:
    case Type::Multiply:
    case Type::Divide:
    case Type::Modulo:
    case Type::LessThan:
    case Type::GreaterThan:
    case Type::LessThanOrEqual:
    case Type::GreaterThanOrEqual:
    case Type::Equal:
    case Type::NotEqual:
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
          use(ast::derived_cast<const Bytecode::Instruction::Move &>(instr).src);
          break;
        case Type::Load:
          break;
        case Type::LessThan: {
          const auto &lt = ast::derived_cast<const Bytecode::Instruction::LessThan &>(instr);
          use(lt.lhs);
          use(lt.rhs);
          break;
        }
        case Type::GreaterThan: {
          const auto &gt =
              ast::derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
          use(gt.lhs);
          use(gt.rhs);
          break;
        }
        case Type::LessThanOrEqual: {
          const auto &lte =
              ast::derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr);
          use(lte.lhs);
          use(lte.rhs);
          break;
        }
        case Type::GreaterThanOrEqual: {
          const auto &gte =
              ast::derived_cast<const Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          use(gte.lhs);
          use(gte.rhs);
          break;
        }
        case Type::Jump:
          break;
        case Type::JumpConditional:
          use(ast::derived_cast<const Bytecode::Instruction::JumpConditional &>(instr).cond);
          break;
        case Type::Call: {
          const auto &c = ast::derived_cast<const Bytecode::Instruction::Call &>(instr);
          for (auto r : c.arg_registers) use(r);
          break;
        }
        case Type::Return:
          use(ast::derived_cast<const Bytecode::Instruction::Return &>(instr).reg);
          break;
        case Type::Equal: {
          const auto &e = ast::derived_cast<const Bytecode::Instruction::Equal &>(instr);
          use(e.src1);
          use(e.src2);
          break;
        }
        case Type::NotEqual: {
          const auto &ne =
              ast::derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
          use(ne.src1);
          use(ne.src2);
          break;
        }
        case Type::Add: {
          const auto &a = ast::derived_cast<const Bytecode::Instruction::Add &>(instr);
          use(a.src1);
          use(a.src2);
          break;
        }
        case Type::Subtract: {
          const auto &s =
              ast::derived_cast<const Bytecode::Instruction::Subtract &>(instr);
          use(s.src1);
          use(s.src2);
          break;
        }
        case Type::AddImmediate:
          use(ast::derived_cast<const Bytecode::Instruction::AddImmediate &>(instr).src);
          break;
        case Type::Multiply: {
          const auto &m =
              ast::derived_cast<const Bytecode::Instruction::Multiply &>(instr);
          use(m.src1);
          use(m.src2);
          break;
        }
        case Type::Divide: {
          const auto &d =
              ast::derived_cast<const Bytecode::Instruction::Divide &>(instr);
          use(d.src1);
          use(d.src2);
          break;
        }
        case Type::Modulo: {
          const auto &mo =
              ast::derived_cast<const Bytecode::Instruction::Modulo &>(instr);
          use(mo.src1);
          use(mo.src2);
          break;
        }
        case Type::ArrayCreate: {
          const auto &ac =
              ast::derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr);
          for (auto r : ac.elements) use(r);
          break;
        }
        case Type::ArrayLoad: {
          const auto &al =
              ast::derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          use(al.array);
          use(al.index);
          break;
        }
        case Type::ArrayStore: {
          const auto &as_ =
              ast::derived_cast<const Bytecode::Instruction::ArrayStore &>(instr);
          use(as_.array);
          use(as_.index);
          use(as_.value);
          break;
        }
        case Type::StructCreate: {
          const auto &sc =
              ast::derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
          for (const auto &[name, r] : sc.fields) use(r);
          break;
        }
        case Type::StructLoad:
          use(ast::derived_cast<const Bytecode::Instruction::StructLoad &>(instr).object);
          break;
        case Type::Negate:
          use(ast::derived_cast<const Bytecode::Instruction::Negate &>(instr).src);
          break;
        case Type::LogicalNot:
          use(ast::derived_cast<const Bytecode::Instruction::LogicalNot &>(instr).src);
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
          auto &m = ast::derived_cast<Bytecode::Instruction::Move &>(instr);
          m.src = resolve(m.src);
          invalidate(m.dst);
          if (m.src != m.dst) {
            copies[m.dst] = m.src;
          }
          break;
        }
        case Type::Load: {
          auto &l = ast::derived_cast<Bytecode::Instruction::Load &>(instr);
          invalidate(l.dst);
          break;
        }
        case Type::LessThan: {
          auto &lt = ast::derived_cast<Bytecode::Instruction::LessThan &>(instr);
          lt.lhs = resolve(lt.lhs);
          lt.rhs = resolve(lt.rhs);
          invalidate(lt.dst);
          break;
        }
        case Type::GreaterThan: {
          auto &gt = ast::derived_cast<Bytecode::Instruction::GreaterThan &>(instr);
          gt.lhs = resolve(gt.lhs);
          gt.rhs = resolve(gt.rhs);
          invalidate(gt.dst);
          break;
        }
        case Type::LessThanOrEqual: {
          auto &lte = ast::derived_cast<Bytecode::Instruction::LessThanOrEqual &>(instr);
          lte.lhs = resolve(lte.lhs);
          lte.rhs = resolve(lte.rhs);
          invalidate(lte.dst);
          break;
        }
        case Type::GreaterThanOrEqual: {
          auto &gte =
              ast::derived_cast<Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          gte.lhs = resolve(gte.lhs);
          gte.rhs = resolve(gte.rhs);
          invalidate(gte.dst);
          break;
        }
        case Type::Jump:
          break;
        case Type::JumpConditional: {
          auto &jc = ast::derived_cast<Bytecode::Instruction::JumpConditional &>(instr);
          jc.cond = resolve(jc.cond);
          break;
        }
        case Type::Call: {
          auto &c = ast::derived_cast<Bytecode::Instruction::Call &>(instr);
          for (auto &arg : c.arg_registers) {
            arg = resolve(arg);
          }
          invalidate(c.dst);
          break;
        }
        case Type::Return: {
          auto &r = ast::derived_cast<Bytecode::Instruction::Return &>(instr);
          r.reg = resolve(r.reg);
          break;
        }
        case Type::Equal: {
          auto &e = ast::derived_cast<Bytecode::Instruction::Equal &>(instr);
          e.src1 = resolve(e.src1);
          e.src2 = resolve(e.src2);
          invalidate(e.dst);
          break;
        }
        case Type::NotEqual: {
          auto &ne = ast::derived_cast<Bytecode::Instruction::NotEqual &>(instr);
          ne.src1 = resolve(ne.src1);
          ne.src2 = resolve(ne.src2);
          invalidate(ne.dst);
          break;
        }
        case Type::Add: {
          auto &a = ast::derived_cast<Bytecode::Instruction::Add &>(instr);
          a.src1 = resolve(a.src1);
          a.src2 = resolve(a.src2);
          invalidate(a.dst);
          break;
        }
        case Type::Subtract: {
          auto &s = ast::derived_cast<Bytecode::Instruction::Subtract &>(instr);
          s.src1 = resolve(s.src1);
          s.src2 = resolve(s.src2);
          invalidate(s.dst);
          break;
        }
        case Type::AddImmediate: {
          auto &ai = ast::derived_cast<Bytecode::Instruction::AddImmediate &>(instr);
          ai.src = resolve(ai.src);
          invalidate(ai.dst);
          break;
        }
        case Type::Multiply: {
          auto &m = ast::derived_cast<Bytecode::Instruction::Multiply &>(instr);
          m.src1 = resolve(m.src1);
          m.src2 = resolve(m.src2);
          invalidate(m.dst);
          break;
        }
        case Type::Divide: {
          auto &d = ast::derived_cast<Bytecode::Instruction::Divide &>(instr);
          d.src1 = resolve(d.src1);
          d.src2 = resolve(d.src2);
          invalidate(d.dst);
          break;
        }
        case Type::Modulo: {
          auto &mo = ast::derived_cast<Bytecode::Instruction::Modulo &>(instr);
          mo.src1 = resolve(mo.src1);
          mo.src2 = resolve(mo.src2);
          invalidate(mo.dst);
          break;
        }
        case Type::ArrayCreate: {
          auto &ac = ast::derived_cast<Bytecode::Instruction::ArrayCreate &>(instr);
          for (auto &elem : ac.elements) {
            elem = resolve(elem);
          }
          invalidate(ac.dst);
          break;
        }
        case Type::ArrayLoad: {
          auto &al = ast::derived_cast<Bytecode::Instruction::ArrayLoad &>(instr);
          al.array = resolve(al.array);
          al.index = resolve(al.index);
          invalidate(al.dst);
          break;
        }
        case Type::ArrayStore: {
          auto &as = ast::derived_cast<Bytecode::Instruction::ArrayStore &>(instr);
          as.array = resolve(as.array);
          as.index = resolve(as.index);
          as.value = resolve(as.value);
          break;
        }
        case Type::StructCreate: {
          auto &sc = ast::derived_cast<Bytecode::Instruction::StructCreate &>(instr);
          for (auto &[name, reg] : sc.fields) {
            reg = resolve(reg);
          }
          invalidate(sc.dst);
          break;
        }
        case Type::StructLoad: {
          auto &sl = ast::derived_cast<Bytecode::Instruction::StructLoad &>(instr);
          sl.object = resolve(sl.object);
          invalidate(sl.dst);
          break;
        }
        case Type::Negate: {
          auto &neg = ast::derived_cast<Bytecode::Instruction::Negate &>(instr);
          neg.src = resolve(neg.src);
          invalidate(neg.dst);
          break;
        }
        case Type::LogicalNot: {
          auto &ln = ast::derived_cast<Bytecode::Instruction::LogicalNot &>(instr);
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
          ast::derived_cast<const Bytecode::Instruction::Move &>(*instr_ptr);
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
              ast::derived_cast<const Bytecode::Instruction::Move &>(instr);
          live.insert(m.src);
          break;
        }
        case Type::Load:
          break;
        case Type::LessThan: {
          const auto &lt =
              ast::derived_cast<const Bytecode::Instruction::LessThan &>(instr);
          live.insert(lt.lhs);
          live.insert(lt.rhs);
          break;
        }
        case Type::GreaterThan: {
          const auto &gt =
              ast::derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
          live.insert(gt.lhs);
          live.insert(gt.rhs);
          break;
        }
        case Type::LessThanOrEqual: {
          const auto &lte =
              ast::derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr);
          live.insert(lte.lhs);
          live.insert(lte.rhs);
          break;
        }
        case Type::GreaterThanOrEqual: {
          const auto &gte = ast::derived_cast<
              const Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          live.insert(gte.lhs);
          live.insert(gte.rhs);
          break;
        }
        case Type::Jump:
          break;
        case Type::JumpConditional: {
          const auto &jc =
              ast::derived_cast<const Bytecode::Instruction::JumpConditional &>(instr);
          live.insert(jc.cond);
          break;
        }
        case Type::Call: {
          const auto &c =
              ast::derived_cast<const Bytecode::Instruction::Call &>(instr);
          for (const auto &arg : c.arg_registers) {
            live.insert(arg);
          }
          break;
        }
        case Type::Return: {
          const auto &r =
              ast::derived_cast<const Bytecode::Instruction::Return &>(instr);
          live.insert(r.reg);
          break;
        }
        case Type::Equal: {
          const auto &e =
              ast::derived_cast<const Bytecode::Instruction::Equal &>(instr);
          live.insert(e.src1);
          live.insert(e.src2);
          break;
        }
        case Type::NotEqual: {
          const auto &ne =
              ast::derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
          live.insert(ne.src1);
          live.insert(ne.src2);
          break;
        }
        case Type::Add: {
          const auto &a =
              ast::derived_cast<const Bytecode::Instruction::Add &>(instr);
          live.insert(a.src1);
          live.insert(a.src2);
          break;
        }
        case Type::Subtract: {
          const auto &s =
              ast::derived_cast<const Bytecode::Instruction::Subtract &>(instr);
          live.insert(s.src1);
          live.insert(s.src2);
          break;
        }
        case Type::AddImmediate: {
          const auto &ai =
              ast::derived_cast<const Bytecode::Instruction::AddImmediate &>(instr);
          live.insert(ai.src);
          break;
        }
        case Type::Multiply: {
          const auto &m =
              ast::derived_cast<const Bytecode::Instruction::Multiply &>(instr);
          live.insert(m.src1);
          live.insert(m.src2);
          break;
        }
        case Type::Divide: {
          const auto &d =
              ast::derived_cast<const Bytecode::Instruction::Divide &>(instr);
          live.insert(d.src1);
          live.insert(d.src2);
          break;
        }
        case Type::Modulo: {
          const auto &mo =
              ast::derived_cast<const Bytecode::Instruction::Modulo &>(instr);
          live.insert(mo.src1);
          live.insert(mo.src2);
          break;
        }
        case Type::ArrayCreate: {
          const auto &ac =
              ast::derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr);
          for (const auto &elem : ac.elements) {
            live.insert(elem);
          }
          break;
        }
        case Type::ArrayLoad: {
          const auto &al =
              ast::derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          live.insert(al.array);
          live.insert(al.index);
          break;
        }
        case Type::ArrayStore: {
          const auto &as =
              ast::derived_cast<const Bytecode::Instruction::ArrayStore &>(instr);
          live.insert(as.array);
          live.insert(as.index);
          live.insert(as.value);
          break;
        }
        case Type::StructCreate: {
          const auto &sc =
              ast::derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
          for (const auto &[name, reg] : sc.fields) {
            live.insert(reg);
          }
          break;
        }
        case Type::StructLoad: {
          const auto &sl =
              ast::derived_cast<const Bytecode::Instruction::StructLoad &>(instr);
          live.insert(sl.object);
          break;
        }
        case Type::Negate: {
          const auto &neg =
              ast::derived_cast<const Bytecode::Instruction::Negate &>(instr);
          live.insert(neg.src);
          break;
        }
        case Type::LogicalNot: {
          const auto &ln =
              ast::derived_cast<const Bytecode::Instruction::LogicalNot &>(instr);
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
        case Type::ArrayStore:
          return false;
        default:
          break;
      }
      // For instructions with a dst register, remove if dst is never read.
      switch (instr.type()) {
        case Type::Move: {
          const auto &m =
              ast::derived_cast<const Bytecode::Instruction::Move &>(instr);
          return live.find(m.dst) == live.end();
        }
        case Type::Load: {
          const auto &l =
              ast::derived_cast<const Bytecode::Instruction::Load &>(instr);
          return live.find(l.dst) == live.end();
        }
        case Type::LessThan: {
          const auto &lt =
              ast::derived_cast<const Bytecode::Instruction::LessThan &>(instr);
          return live.find(lt.dst) == live.end();
        }
        case Type::GreaterThan: {
          const auto &gt =
              ast::derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
          return live.find(gt.dst) == live.end();
        }
        case Type::LessThanOrEqual: {
          const auto &lte =
              ast::derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr);
          return live.find(lte.dst) == live.end();
        }
        case Type::GreaterThanOrEqual: {
          const auto &gte = ast::derived_cast<
              const Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          return live.find(gte.dst) == live.end();
        }
        case Type::Equal: {
          const auto &e =
              ast::derived_cast<const Bytecode::Instruction::Equal &>(instr);
          return live.find(e.dst) == live.end();
        }
        case Type::NotEqual: {
          const auto &ne =
              ast::derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
          return live.find(ne.dst) == live.end();
        }
        case Type::Add: {
          const auto &a =
              ast::derived_cast<const Bytecode::Instruction::Add &>(instr);
          return live.find(a.dst) == live.end();
        }
        case Type::Subtract: {
          const auto &s =
              ast::derived_cast<const Bytecode::Instruction::Subtract &>(instr);
          return live.find(s.dst) == live.end();
        }
        case Type::AddImmediate: {
          const auto &ai =
              ast::derived_cast<const Bytecode::Instruction::AddImmediate &>(instr);
          return live.find(ai.dst) == live.end();
        }
        case Type::Multiply: {
          const auto &m =
              ast::derived_cast<const Bytecode::Instruction::Multiply &>(instr);
          return live.find(m.dst) == live.end();
        }
        case Type::Divide: {
          const auto &d =
              ast::derived_cast<const Bytecode::Instruction::Divide &>(instr);
          return live.find(d.dst) == live.end();
        }
        case Type::Modulo: {
          const auto &mo =
              ast::derived_cast<const Bytecode::Instruction::Modulo &>(instr);
          return live.find(mo.dst) == live.end();
        }
        case Type::ArrayCreate: {
          const auto &ac =
              ast::derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr);
          return live.find(ac.dst) == live.end();
        }
        case Type::ArrayLoad: {
          const auto &al =
              ast::derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          return live.find(al.dst) == live.end();
        }
        case Type::StructCreate: {
          const auto &sc =
              ast::derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
          return live.find(sc.dst) == live.end();
        }
        case Type::StructLoad: {
          const auto &sl =
              ast::derived_cast<const Bytecode::Instruction::StructLoad &>(instr);
          return live.find(sl.dst) == live.end();
        }
        case Type::Negate: {
          const auto &neg =
              ast::derived_cast<const Bytecode::Instruction::Negate &>(instr);
          return live.find(neg.dst) == live.end();
        }
        case Type::LogicalNot: {
          const auto &ln =
              ast::derived_cast<const Bytecode::Instruction::LogicalNot &>(instr);
          return live.find(ln.dst) == live.end();
        }
        default:
          return false;
      }
    });
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
      const auto &j = ast::derived_cast<const Bytecode::Instruction::Jump &>(last);
      if (static_cast<size_t>(j.label) <= i) {
        loops.push_back({static_cast<size_t>(j.label), i});
      }
    } else if (last.type() == Type::JumpConditional) {
      const auto &jc =
          ast::derived_cast<const Bytecode::Instruction::JumpConditional &>(last);
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

  for (auto &block : blocks) {
    auto &instrs = block.instructions;
    size_t i = 0;
    while (i + 1 < instrs.size()) {
      const auto t = instrs[i]->type();
      // Only fold patterns whose producing instruction is Load or AddImmediate.
      if (t != Type::AddImmediate && t != Type::Load) {
        ++i;
        continue;
      }
      // The immediately-following instruction must be a Move.
      if (instrs[i + 1]->type() != Type::Move) {
        ++i;
        continue;
      }
      const auto &mv =
          ast::derived_cast<const Bytecode::Instruction::Move &>(*instrs[i + 1]);
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
      if (t == Type::AddImmediate) {
        ast::derived_cast<Bytecode::Instruction::AddImmediate &>(*instrs[i]).dst = r_var;
      } else {
        ast::derived_cast<Bytecode::Instruction::Load &>(*instrs[i]).dst = r_var;
      }
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
          const auto &m = ast::derived_cast<const Bytecode::Instruction::Move &>(instr);
          track(m.dst);
          track(m.src);
          break;
        }
        case Type::Load:
          track(ast::derived_cast<const Bytecode::Instruction::Load &>(instr).dst);
          break;
        case Type::LessThan: {
          const auto &lt = ast::derived_cast<const Bytecode::Instruction::LessThan &>(instr);
          track(lt.dst);
          track(lt.lhs);
          track(lt.rhs);
          break;
        }
        case Type::GreaterThan: {
          const auto &gt =
              ast::derived_cast<const Bytecode::Instruction::GreaterThan &>(instr);
          track(gt.dst);
          track(gt.lhs);
          track(gt.rhs);
          break;
        }
        case Type::LessThanOrEqual: {
          const auto &lte =
              ast::derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(instr);
          track(lte.dst);
          track(lte.lhs);
          track(lte.rhs);
          break;
        }
        case Type::GreaterThanOrEqual: {
          const auto &gte =
              ast::derived_cast<const Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          track(gte.dst);
          track(gte.lhs);
          track(gte.rhs);
          break;
        }
        case Type::Jump:
          break;
        case Type::JumpConditional:
          track(ast::derived_cast<const Bytecode::Instruction::JumpConditional &>(instr).cond);
          break;
        case Type::Call: {
          const auto &c = ast::derived_cast<const Bytecode::Instruction::Call &>(instr);
          track(c.dst);
          for (auto r : c.arg_registers) track(r);
          for (auto r : c.param_registers) track(r);
          break;
        }
        case Type::Return:
          track(ast::derived_cast<const Bytecode::Instruction::Return &>(instr).reg);
          break;
        case Type::Equal: {
          const auto &e = ast::derived_cast<const Bytecode::Instruction::Equal &>(instr);
          track(e.dst);
          track(e.src1);
          track(e.src2);
          break;
        }
        case Type::NotEqual: {
          const auto &ne =
              ast::derived_cast<const Bytecode::Instruction::NotEqual &>(instr);
          track(ne.dst);
          track(ne.src1);
          track(ne.src2);
          break;
        }
        case Type::Add: {
          const auto &a = ast::derived_cast<const Bytecode::Instruction::Add &>(instr);
          track(a.dst);
          track(a.src1);
          track(a.src2);
          break;
        }
        case Type::Subtract: {
          const auto &s = ast::derived_cast<const Bytecode::Instruction::Subtract &>(instr);
          track(s.dst);
          track(s.src1);
          track(s.src2);
          break;
        }
        case Type::AddImmediate: {
          const auto &ai =
              ast::derived_cast<const Bytecode::Instruction::AddImmediate &>(instr);
          track(ai.dst);
          track(ai.src);
          break;
        }
        case Type::Multiply: {
          const auto &m =
              ast::derived_cast<const Bytecode::Instruction::Multiply &>(instr);
          track(m.dst);
          track(m.src1);
          track(m.src2);
          break;
        }
        case Type::Divide: {
          const auto &d = ast::derived_cast<const Bytecode::Instruction::Divide &>(instr);
          track(d.dst);
          track(d.src1);
          track(d.src2);
          break;
        }
        case Type::Modulo: {
          const auto &mo = ast::derived_cast<const Bytecode::Instruction::Modulo &>(instr);
          track(mo.dst);
          track(mo.src1);
          track(mo.src2);
          break;
        }
        case Type::ArrayCreate: {
          const auto &ac =
              ast::derived_cast<const Bytecode::Instruction::ArrayCreate &>(instr);
          track(ac.dst);
          for (auto elem : ac.elements) track(elem);
          break;
        }
        case Type::ArrayLoad: {
          const auto &al =
              ast::derived_cast<const Bytecode::Instruction::ArrayLoad &>(instr);
          track(al.dst);
          track(al.array);
          track(al.index);
          break;
        }
        case Type::ArrayStore: {
          const auto &as = ast::derived_cast<const Bytecode::Instruction::ArrayStore &>(instr);
          track(as.array);
          track(as.index);
          track(as.value);
          break;
        }
        case Type::StructCreate: {
          const auto &sc =
              ast::derived_cast<const Bytecode::Instruction::StructCreate &>(instr);
          track(sc.dst);
          for (const auto &[name, reg] : sc.fields) track(reg);
          break;
        }
        case Type::StructLoad: {
          const auto &sl =
              ast::derived_cast<const Bytecode::Instruction::StructLoad &>(instr);
          track(sl.dst);
          track(sl.object);
          break;
        }
        case Type::Negate: {
          const auto &n = ast::derived_cast<const Bytecode::Instruction::Negate &>(instr);
          track(n.dst);
          track(n.src);
          break;
        }
        case Type::LogicalNot: {
          const auto &ln =
              ast::derived_cast<const Bytecode::Instruction::LogicalNot &>(instr);
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
          auto &m = ast::derived_cast<Bytecode::Instruction::Move &>(instr);
          m.dst = remap(m.dst);
          m.src = remap(m.src);
          break;
        }
        case Type::Load: {
          auto &l = ast::derived_cast<Bytecode::Instruction::Load &>(instr);
          l.dst = remap(l.dst);
          break;
        }
        case Type::LessThan: {
          auto &lt = ast::derived_cast<Bytecode::Instruction::LessThan &>(instr);
          lt.dst = remap(lt.dst);
          lt.lhs = remap(lt.lhs);
          lt.rhs = remap(lt.rhs);
          break;
        }
        case Type::GreaterThan: {
          auto &gt = ast::derived_cast<Bytecode::Instruction::GreaterThan &>(instr);
          gt.dst = remap(gt.dst);
          gt.lhs = remap(gt.lhs);
          gt.rhs = remap(gt.rhs);
          break;
        }
        case Type::LessThanOrEqual: {
          auto &lte = ast::derived_cast<Bytecode::Instruction::LessThanOrEqual &>(instr);
          lte.dst = remap(lte.dst);
          lte.lhs = remap(lte.lhs);
          lte.rhs = remap(lte.rhs);
          break;
        }
        case Type::GreaterThanOrEqual: {
          auto &gte =
              ast::derived_cast<Bytecode::Instruction::GreaterThanOrEqual &>(instr);
          gte.dst = remap(gte.dst);
          gte.lhs = remap(gte.lhs);
          gte.rhs = remap(gte.rhs);
          break;
        }
        case Type::Jump:
          break;
        case Type::JumpConditional: {
          auto &jc = ast::derived_cast<Bytecode::Instruction::JumpConditional &>(instr);
          jc.cond = remap(jc.cond);
          break;
        }
        case Type::Call: {
          auto &c = ast::derived_cast<Bytecode::Instruction::Call &>(instr);
          c.dst = remap(c.dst);
          for (auto &arg : c.arg_registers) {
            arg = remap(arg);
          }
          for (auto &param : c.param_registers) {
            param = remap(param);
          }
          break;
        }
        case Type::Return: {
          auto &r = ast::derived_cast<Bytecode::Instruction::Return &>(instr);
          r.reg = remap(r.reg);
          break;
        }
        case Type::Equal: {
          auto &e = ast::derived_cast<Bytecode::Instruction::Equal &>(instr);
          e.dst = remap(e.dst);
          e.src1 = remap(e.src1);
          e.src2 = remap(e.src2);
          break;
        }
        case Type::NotEqual: {
          auto &ne = ast::derived_cast<Bytecode::Instruction::NotEqual &>(instr);
          ne.dst = remap(ne.dst);
          ne.src1 = remap(ne.src1);
          ne.src2 = remap(ne.src2);
          break;
        }
        case Type::Add: {
          auto &a = ast::derived_cast<Bytecode::Instruction::Add &>(instr);
          a.dst = remap(a.dst);
          a.src1 = remap(a.src1);
          a.src2 = remap(a.src2);
          break;
        }
        case Type::Subtract: {
          auto &s = ast::derived_cast<Bytecode::Instruction::Subtract &>(instr);
          s.dst = remap(s.dst);
          s.src1 = remap(s.src1);
          s.src2 = remap(s.src2);
          break;
        }
        case Type::AddImmediate: {
          auto &ai = ast::derived_cast<Bytecode::Instruction::AddImmediate &>(instr);
          ai.dst = remap(ai.dst);
          ai.src = remap(ai.src);
          break;
        }
        case Type::Multiply: {
          auto &m = ast::derived_cast<Bytecode::Instruction::Multiply &>(instr);
          m.dst = remap(m.dst);
          m.src1 = remap(m.src1);
          m.src2 = remap(m.src2);
          break;
        }
        case Type::Divide: {
          auto &d = ast::derived_cast<Bytecode::Instruction::Divide &>(instr);
          d.dst = remap(d.dst);
          d.src1 = remap(d.src1);
          d.src2 = remap(d.src2);
          break;
        }
        case Type::Modulo: {
          auto &mo = ast::derived_cast<Bytecode::Instruction::Modulo &>(instr);
          mo.dst = remap(mo.dst);
          mo.src1 = remap(mo.src1);
          mo.src2 = remap(mo.src2);
          break;
        }
        case Type::ArrayCreate: {
          auto &ac = ast::derived_cast<Bytecode::Instruction::ArrayCreate &>(instr);
          ac.dst = remap(ac.dst);
          for (auto &elem : ac.elements) {
            elem = remap(elem);
          }
          break;
        }
        case Type::ArrayLoad: {
          auto &al = ast::derived_cast<Bytecode::Instruction::ArrayLoad &>(instr);
          al.dst = remap(al.dst);
          al.array = remap(al.array);
          al.index = remap(al.index);
          break;
        }
        case Type::ArrayStore: {
          auto &as = ast::derived_cast<Bytecode::Instruction::ArrayStore &>(instr);
          as.array = remap(as.array);
          as.index = remap(as.index);
          as.value = remap(as.value);
          break;
        }
        case Type::StructCreate: {
          auto &sc = ast::derived_cast<Bytecode::Instruction::StructCreate &>(instr);
          sc.dst = remap(sc.dst);
          for (auto &[name, reg] : sc.fields) {
            reg = remap(reg);
          }
          break;
        }
        case Type::StructLoad: {
          auto &sl = ast::derived_cast<Bytecode::Instruction::StructLoad &>(instr);
          sl.dst = remap(sl.dst);
          sl.object = remap(sl.object);
          break;
        }
        case Type::Negate: {
          auto &n = ast::derived_cast<Bytecode::Instruction::Negate &>(instr);
          n.dst = remap(n.dst);
          n.src = remap(n.src);
          break;
        }
        case Type::LogicalNot: {
          auto &ln = ast::derived_cast<Bytecode::Instruction::LogicalNot &>(instr);
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
