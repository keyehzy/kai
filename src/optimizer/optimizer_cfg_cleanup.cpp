#include "../optimizer.h"

#include <limits>
#include <unordered_set>

namespace kai {

using Label = Bytecode::Label;
using Type = Bytecode::Instruction::Type;

namespace {

bool is_terminator_type(Type type) {
  switch (type) {
    case Type::Jump:
    case Type::JumpConditional:
    case Type::Return:
    case Type::TailCall:
      return true;
    default:
      return false;
  }
}

bool is_jump_only_block(const Bytecode::BasicBlock &block) {
  return block.instructions.size() == 1 && block.instructions[0]->type() == Type::Jump;
}

}  // namespace

void BytecodeOptimizer::cfg_cleanup(std::vector<Bytecode::BasicBlock> &blocks) {
  // 1) Trim everything after the first terminator in each block.
  for (auto &block : blocks) {
    for (size_t i = 0; i < block.instructions.size(); ++i) {
      if (!is_terminator_type(block.instructions[i]->type())) {
        continue;
      }
      block.instructions.erase(block.instructions.begin() + static_cast<std::ptrdiff_t>(i + 1),
                               block.instructions.end());
      break;
    }
  }

  // 2) Collapse jump-only trampoline chains by retargeting incoming branches.
  const auto resolve_jump_target = [&blocks](Label label) -> Label {
    if (label >= blocks.size()) {
      return label;
    }

    const auto original = label;
    std::unordered_set<Label> visited;
    auto current = label;
    while (current < blocks.size() && is_jump_only_block(blocks[current])) {
      const auto &jump =
          derived_cast<const Bytecode::Instruction::Jump &>(*blocks[current].instructions[0]);
      const auto next = jump.label;
      if (next == current) {
        return current;
      }
      if (!visited.insert(current).second || visited.count(next) > 0) {
        return original;
      }
      current = next;
    }
    return current;
  };

  for (auto &block : blocks) {
    for (auto &instr_ptr : block.instructions) {
      auto &instr = *instr_ptr;
      if (instr.type() == Type::Jump) {
        auto &jump = derived_cast<Bytecode::Instruction::Jump &>(instr);
        jump.label = resolve_jump_target(jump.label);
      } else if (instr.type() == Type::JumpConditional) {
        auto &jump_cond = derived_cast<Bytecode::Instruction::JumpConditional &>(instr);
        jump_cond.label1 = resolve_jump_target(jump_cond.label1);
        jump_cond.label2 = resolve_jump_target(jump_cond.label2);
      }
    }
  }

  // 3) Remove jump-only trampolines that became unreferenced.
  std::vector<size_t> incoming(blocks.size(), 0);
  for (const auto &block : blocks) {
    for (const auto &instr_ptr : block.instructions) {
      const auto &instr = *instr_ptr;
      switch (instr.type()) {
        case Type::Jump: {
          const auto target = derived_cast<const Bytecode::Instruction::Jump &>(instr).label;
          if (target < incoming.size()) {
            ++incoming[target];
          }
          break;
        }
        case Type::JumpConditional: {
          const auto &jump_cond =
              derived_cast<const Bytecode::Instruction::JumpConditional &>(instr);
          if (jump_cond.label1 < incoming.size()) {
            ++incoming[jump_cond.label1];
          }
          if (jump_cond.label2 < incoming.size()) {
            ++incoming[jump_cond.label2];
          }
          break;
        }
        case Type::Call: {
          const auto target = derived_cast<const Bytecode::Instruction::Call &>(instr).label;
          if (target < incoming.size()) {
            ++incoming[target];
          }
          break;
        }
        case Type::TailCall: {
          const auto target =
              derived_cast<const Bytecode::Instruction::TailCall &>(instr).label;
          if (target < incoming.size()) {
            ++incoming[target];
          }
          break;
        }
        default:
          break;
      }
    }
  }

  std::vector<bool> keep(blocks.size(), true);
  bool removed_any = false;
  for (size_t i = 1; i < blocks.size(); ++i) {
    if (is_jump_only_block(blocks[i]) && incoming[i] == 0) {
      keep[i] = false;
      removed_any = true;
    }
  }
  if (!removed_any) {
    return;
  }

  constexpr auto kInvalidLabel = std::numeric_limits<Label>::max();
  std::vector<Label> old_to_new(blocks.size(), kInvalidLabel);
  std::vector<Bytecode::BasicBlock> remapped_blocks;
  remapped_blocks.reserve(blocks.size());
  for (size_t i = 0; i < blocks.size(); ++i) {
    if (!keep[i]) {
      continue;
    }
    old_to_new[i] = static_cast<Label>(remapped_blocks.size());
    remapped_blocks.push_back(std::move(blocks[i]));
  }
  blocks = std::move(remapped_blocks);

  const auto remap_label = [&old_to_new](Label label) -> Label {
    if (label >= old_to_new.size()) {
      return label;
    }
    const auto mapped = old_to_new[label];
    return mapped == kInvalidLabel ? label : mapped;
  };

  for (auto &block : blocks) {
    for (auto &instr_ptr : block.instructions) {
      auto &instr = *instr_ptr;
      switch (instr.type()) {
        case Type::Jump: {
          auto &jump = derived_cast<Bytecode::Instruction::Jump &>(instr);
          jump.label = remap_label(jump.label);
          break;
        }
        case Type::JumpConditional: {
          auto &jump_cond = derived_cast<Bytecode::Instruction::JumpConditional &>(instr);
          jump_cond.label1 = remap_label(jump_cond.label1);
          jump_cond.label2 = remap_label(jump_cond.label2);
          break;
        }
        case Type::Call: {
          auto &call = derived_cast<Bytecode::Instruction::Call &>(instr);
          call.label = remap_label(call.label);
          break;
        }
        case Type::TailCall: {
          auto &tail_call = derived_cast<Bytecode::Instruction::TailCall &>(instr);
          tail_call.label = remap_label(tail_call.label);
          break;
        }
        default:
          break;
      }
    }
  }
}

}  // namespace kai
