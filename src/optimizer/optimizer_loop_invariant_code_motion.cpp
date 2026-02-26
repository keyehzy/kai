#include "../optimizer.h"
#include "optimizer_internal.h"

#include <unordered_map>
#include <vector>

namespace kai {

using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;

namespace {

// Returns source registers for hoistable instruction types.
// Non-hoistable types (Call, ArrayStore, etc.) are never passed here.
std::vector<Register> get_src_regs(const Bytecode::Instruction &instr) {
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

bool is_hoistable(const Bytecode::Instruction &instr,
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
  if (!dst_opt) {
    return false;
  }

  auto it = def_count.find(*dst_opt);
  if (it == def_count.end() || it->second != 1) {
    return false;
  }

  for (Register src : get_src_regs(instr)) {
    if (def_count.count(src) > 0) {
      return false;
    }
  }

  return true;
}

}  // namespace

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
    } else if (last.type() == Type::JumpEqualImmediate) {
      const auto &jump_equal_imm =
          derived_cast<const Bytecode::Instruction::JumpEqualImmediate &>(last);
      if (static_cast<size_t>(jump_equal_imm.label1) <= i) {
        loops.push_back({static_cast<size_t>(jump_equal_imm.label1), i});
      }
      if (static_cast<size_t>(jump_equal_imm.label2) <= i) {
        loops.push_back({static_cast<size_t>(jump_equal_imm.label2), i});
      }
    } else if (last.type() == Type::JumpGreaterThanImmediate) {
      const auto &jump_greater_than_imm =
          derived_cast<const Bytecode::Instruction::JumpGreaterThanImmediate &>(last);
      if (static_cast<size_t>(jump_greater_than_imm.label1) <= i) {
        loops.push_back({static_cast<size_t>(jump_greater_than_imm.label1), i});
      }
      if (static_cast<size_t>(jump_greater_than_imm.label2) <= i) {
        loops.push_back({static_cast<size_t>(jump_greater_than_imm.label2), i});
      }
    } else if (last.type() == Type::JumpLessThanOrEqual) {
      const auto &jump_lte =
          derived_cast<const Bytecode::Instruction::JumpLessThanOrEqual &>(last);
      if (static_cast<size_t>(jump_lte.label1) <= i) {
        loops.push_back({static_cast<size_t>(jump_lte.label1), i});
      }
      if (static_cast<size_t>(jump_lte.label2) <= i) {
        loops.push_back({static_cast<size_t>(jump_lte.label2), i});
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
                last_type == Type::JumpEqualImmediate ||
                last_type == Type::JumpGreaterThanImmediate ||
                last_type == Type::JumpLessThanOrEqual || last_type == Type::Return ||
                last_type == Type::TailCall) {
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

}  // namespace kai
