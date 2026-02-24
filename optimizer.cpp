#include "optimizer.h"

#include <unordered_map>
#include <unordered_set>

namespace kai {
namespace bytecode {

using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;

void BytecodeOptimizer::optimize(std::vector<Bytecode::BasicBlock> &blocks) {
  copy_propagation(blocks);
  dead_code_elimination(blocks);
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
        default:
          return false;
      }
    });
  }
}

}  // namespace bytecode
}  // namespace kai
