#include "../optimizer.h"
#include "optimizer_internal.h"

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kai {

using Label = Bytecode::Label;
using Register = Bytecode::Register;
using Type = Bytecode::Instruction::Type;
using Value = Bytecode::Value;

namespace {

struct Fact {
  enum class Kind {
    Register,
    Constant,
  };

  Kind kind;
  Register reg;
  Value value;

  static Fact register_fact(Register r) {
    return {Kind::Register, r, 0};
  }

  static Fact constant_fact(Value v) {
    return {Kind::Constant, 0, v};
  }

  bool operator==(const Fact &other) const = default;
};

using FactMap = std::unordered_map<Register, Fact>;

struct ResolvedValue {
  bool is_constant;
  Register reg;
  Value value;
};

ResolvedValue resolve_value(Register reg, const FactMap &facts) {
  std::unordered_set<Register> visited;
  auto current = reg;

  for (;;) {
    const auto it = facts.find(current);
    if (it == facts.end()) {
      return {false, current, 0};
    }

    const auto &fact = it->second;
    if (fact.kind == Fact::Kind::Constant) {
      return {true, 0, fact.value};
    }

    if (!visited.insert(current).second || visited.count(fact.reg) > 0) {
      return {false, current, 0};
    }
    current = fact.reg;
  }
}

Register resolve_register_alias(Register reg, const FactMap &facts) {
  std::unordered_set<Register> visited;
  auto current = reg;

  for (;;) {
    const auto it = facts.find(current);
    if (it == facts.end()) {
      return current;
    }

    const auto &fact = it->second;
    if (fact.kind == Fact::Kind::Constant) {
      return current;
    }

    if (!visited.insert(current).second || visited.count(fact.reg) > 0) {
      return current;
    }
    current = fact.reg;
  }
}

void invalidate(FactMap &facts, Register dst) {
  std::unordered_set<Register> invalidated = {dst};
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto it = facts.begin(); it != facts.end();) {
      bool remove = invalidated.count(it->first) > 0;
      if (!remove) {
        const auto &fact = it->second;
        remove = fact.kind == Fact::Kind::Register &&
                 invalidated.count(fact.reg) > 0;
      }
      if (!remove) {
        ++it;
        continue;
      }
      changed |= invalidated.insert(it->first).second;
      it = facts.erase(it);
    }
  }
}

void set_register_fact(FactMap &facts, Register dst, Register src) {
  if (dst == src) {
    return;
  }
  invalidate(facts, dst);
  facts[dst] = Fact::register_fact(src);
}

void set_constant_fact(FactMap &facts, Register dst, Value value) {
  invalidate(facts, dst);
  facts[dst] = Fact::constant_fact(value);
}

void transfer_instruction(const Bytecode::Instruction &instr, FactMap &facts) {
  switch (instr.type()) {
    case Type::Move: {
      const auto &move = derived_cast<const Bytecode::Instruction::Move &>(instr);
      set_register_fact(facts, move.dst, move.src);
      return;
    }
    case Type::Load: {
      const auto &load = derived_cast<const Bytecode::Instruction::Load &>(instr);
      set_constant_fact(facts, load.dst, load.value);
      return;
    }
    default:
      break;
  }

  if (const auto dst_opt = get_dst_reg(instr); dst_opt) {
    invalidate(facts, *dst_opt);
  }
}

void add_successor(std::vector<size_t> &successors, Label label, size_t block_count) {
  if (label >= block_count) {
    return;
  }
  const auto succ = static_cast<size_t>(label);
  if (std::find(successors.begin(), successors.end(), succ) == successors.end()) {
    successors.push_back(succ);
  }
}

std::vector<std::vector<size_t>> build_successors(
    const std::vector<Bytecode::BasicBlock> &blocks) {
  std::vector<std::vector<size_t>> successors(blocks.size());

  for (size_t i = 0; i < blocks.size(); ++i) {
    bool found_terminator = false;
    for (const auto &instr_ptr : blocks[i].instructions) {
      const auto &instr = *instr_ptr;
      switch (instr.type()) {
        case Type::Jump: {
          const auto &jump = derived_cast<const Bytecode::Instruction::Jump &>(instr);
          add_successor(successors[i], jump.label, blocks.size());
          found_terminator = true;
          break;
        }
        case Type::JumpConditional: {
          const auto &jump_cond =
              derived_cast<const Bytecode::Instruction::JumpConditional &>(instr);
          add_successor(successors[i], jump_cond.label1, blocks.size());
          add_successor(successors[i], jump_cond.label2, blocks.size());
          found_terminator = true;
          break;
        }
        case Type::JumpEqualImmediate: {
          const auto &jump_equal_imm =
              derived_cast<const Bytecode::Instruction::JumpEqualImmediate &>(instr);
          add_successor(successors[i], jump_equal_imm.label1, blocks.size());
          add_successor(successors[i], jump_equal_imm.label2, blocks.size());
          found_terminator = true;
          break;
        }
        case Type::JumpGreaterThanImmediate: {
          const auto &jump_greater_than_imm =
              derived_cast<const Bytecode::Instruction::JumpGreaterThanImmediate &>(instr);
          add_successor(successors[i], jump_greater_than_imm.label1, blocks.size());
          add_successor(successors[i], jump_greater_than_imm.label2, blocks.size());
          found_terminator = true;
          break;
        }
        case Type::JumpLessThanOrEqual: {
          const auto &jump_lte =
              derived_cast<const Bytecode::Instruction::JumpLessThanOrEqual &>(instr);
          add_successor(successors[i], jump_lte.label1, blocks.size());
          add_successor(successors[i], jump_lte.label2, blocks.size());
          found_terminator = true;
          break;
        }
        case Type::Return:
        case Type::TailCall:
          found_terminator = true;
          break;
        default:
          break;
      }

      if (found_terminator) {
        break;
      }
    }

    if (!found_terminator && i + 1 < blocks.size()) {
      successors[i].push_back(i + 1);
    }
  }

  return successors;
}

std::vector<std::vector<size_t>> build_predecessors(
    const std::vector<std::vector<size_t>> &successors) {
  std::vector<std::vector<size_t>> predecessors(successors.size());

  for (size_t from = 0; from < successors.size(); ++from) {
    for (const auto to : successors[from]) {
      predecessors[to].push_back(from);
    }
  }

  return predecessors;
}

FactMap meet_predecessors(size_t block_index,
                          const std::vector<std::vector<size_t>> &predecessors,
                          const std::vector<FactMap> &out_states,
                          const std::vector<bool> &out_initialized) {
  if (predecessors[block_index].empty()) {
    return {};
  }

  bool found_any_initialized_pred = false;
  FactMap in_state;
  for (const auto pred : predecessors[block_index]) {
    if (!out_initialized[pred]) {
      continue;
    }
    if (!found_any_initialized_pred) {
      in_state = out_states[pred];
      found_any_initialized_pred = true;
      continue;
    }
    const auto &pred_state = out_states[pred];
    std::erase_if(in_state, [&pred_state](const auto &entry) {
      const auto it = pred_state.find(entry.first);
      return it == pred_state.end() || !(it->second == entry.second);
    });
  }

  if (!found_any_initialized_pred) {
    return {};
  }

  return in_state;
}

void rewrite_instruction(std::unique_ptr<Bytecode::Instruction> &instr_ptr,
                         FactMap &facts,
                         const FactMap &entry_facts) {
  auto &instr = *instr_ptr;

  const auto resolve_register = [&facts](Register reg) {
    return resolve_register_alias(reg, facts);
  };

  switch (instr.type()) {
    case Type::Move: {
      auto &move = derived_cast<Bytecode::Instruction::Move &>(instr);
      const auto original_src = move.src;
      move.src = resolve_register(move.src);
      set_register_fact(facts, move.dst, original_src);
      break;
    }
    case Type::Load: {
      const auto &load = derived_cast<const Bytecode::Instruction::Load &>(instr);
      set_constant_fact(facts, load.dst, load.value);
      break;
    }
    case Type::LessThan: {
      auto &less_than = derived_cast<Bytecode::Instruction::LessThan &>(instr);
      less_than.lhs = resolve_register(less_than.lhs);
      less_than.rhs = resolve_register(less_than.rhs);
      invalidate(facts, less_than.dst);
      break;
    }
    case Type::LessThanImmediate: {
      auto &less_than_imm =
          derived_cast<Bytecode::Instruction::LessThanImmediate &>(instr);
      less_than_imm.lhs = resolve_register(less_than_imm.lhs);
      invalidate(facts, less_than_imm.dst);
      break;
    }
    case Type::GreaterThan: {
      auto &greater_than = derived_cast<Bytecode::Instruction::GreaterThan &>(instr);
      greater_than.lhs = resolve_register(greater_than.lhs);
      greater_than.rhs = resolve_register(greater_than.rhs);
      invalidate(facts, greater_than.dst);
      break;
    }
    case Type::GreaterThanImmediate: {
      auto &greater_than_imm =
          derived_cast<Bytecode::Instruction::GreaterThanImmediate &>(instr);
      greater_than_imm.lhs = resolve_register(greater_than_imm.lhs);
      invalidate(facts, greater_than_imm.dst);
      break;
    }
    case Type::LessThanOrEqual: {
      auto &less_than_or_equal =
          derived_cast<Bytecode::Instruction::LessThanOrEqual &>(instr);
      less_than_or_equal.lhs = resolve_register(less_than_or_equal.lhs);
      less_than_or_equal.rhs = resolve_register(less_than_or_equal.rhs);
      invalidate(facts, less_than_or_equal.dst);
      break;
    }
    case Type::LessThanOrEqualImmediate: {
      auto &less_than_or_equal_imm =
          derived_cast<Bytecode::Instruction::LessThanOrEqualImmediate &>(instr);
      less_than_or_equal_imm.lhs = resolve_register(less_than_or_equal_imm.lhs);
      invalidate(facts, less_than_or_equal_imm.dst);
      break;
    }
    case Type::GreaterThanOrEqual: {
      auto &greater_than_or_equal =
          derived_cast<Bytecode::Instruction::GreaterThanOrEqual &>(instr);
      greater_than_or_equal.lhs = resolve_register(greater_than_or_equal.lhs);
      greater_than_or_equal.rhs = resolve_register(greater_than_or_equal.rhs);
      invalidate(facts, greater_than_or_equal.dst);
      break;
    }
    case Type::GreaterThanOrEqualImmediate: {
      auto &greater_than_or_equal_imm =
          derived_cast<Bytecode::Instruction::GreaterThanOrEqualImmediate &>(instr);
      greater_than_or_equal_imm.lhs = resolve_register(greater_than_or_equal_imm.lhs);
      invalidate(facts, greater_than_or_equal_imm.dst);
      break;
    }
    case Type::Jump:
      break;
    case Type::JumpConditional: {
      auto &jump_cond = derived_cast<Bytecode::Instruction::JumpConditional &>(instr);
      const auto resolved = resolve_value(jump_cond.cond, facts);
      if (resolved.is_constant) {
        const auto target = resolved.value != 0 ? jump_cond.label1 : jump_cond.label2;
        instr_ptr = std::make_unique<Bytecode::Instruction::Jump>(target);
      } else {
        jump_cond.cond = resolve_register(jump_cond.cond);
      }
      break;
    }
    case Type::JumpEqualImmediate: {
      auto &jump_equal_imm =
          derived_cast<Bytecode::Instruction::JumpEqualImmediate &>(instr);
      const auto resolved = resolve_value(jump_equal_imm.src, facts);
      if (resolved.is_constant) {
        const auto target = resolved.value == jump_equal_imm.value ? jump_equal_imm.label1
                                                                    : jump_equal_imm.label2;
        instr_ptr = std::make_unique<Bytecode::Instruction::Jump>(target);
      } else {
        jump_equal_imm.src = resolve_register(jump_equal_imm.src);
      }
      break;
    }
    case Type::JumpGreaterThanImmediate: {
      auto &jump_greater_than_imm =
          derived_cast<Bytecode::Instruction::JumpGreaterThanImmediate &>(instr);
      const auto resolved = resolve_value(jump_greater_than_imm.lhs, facts);
      if (resolved.is_constant) {
        const auto target = resolved.value > jump_greater_than_imm.value
                                ? jump_greater_than_imm.label1
                                : jump_greater_than_imm.label2;
        instr_ptr = std::make_unique<Bytecode::Instruction::Jump>(target);
      } else {
        jump_greater_than_imm.lhs = resolve_register(jump_greater_than_imm.lhs);
      }
      break;
    }
    case Type::JumpLessThanOrEqual: {
      auto &jump_lte = derived_cast<Bytecode::Instruction::JumpLessThanOrEqual &>(instr);
      const auto lhs_resolved = resolve_value(jump_lte.lhs, facts);
      const auto rhs_resolved = resolve_value(jump_lte.rhs, facts);
      if (lhs_resolved.is_constant && rhs_resolved.is_constant) {
        const auto target = lhs_resolved.value <= rhs_resolved.value ? jump_lte.label1
                                                                      : jump_lte.label2;
        instr_ptr = std::make_unique<Bytecode::Instruction::Jump>(target);
      } else {
        if (!lhs_resolved.is_constant) {
          jump_lte.lhs = resolve_register(jump_lte.lhs);
        }
        if (!rhs_resolved.is_constant) {
          jump_lte.rhs = resolve_register(jump_lte.rhs);
        }
      }
      break;
    }
    case Type::Call: {
      auto &call = derived_cast<Bytecode::Instruction::Call &>(instr);
      for (auto &arg : call.arg_registers) {
        arg = resolve_register(arg);
      }
      invalidate(facts, call.dst);
      break;
    }
    case Type::TailCall: {
      auto &tail_call = derived_cast<Bytecode::Instruction::TailCall &>(instr);
      for (auto &arg : tail_call.arg_registers) {
        arg = resolve_register(arg);
      }
      break;
    }
    case Type::Return: {
      auto &ret = derived_cast<Bytecode::Instruction::Return &>(instr);
      const auto entry_resolved = resolve_register_alias(ret.reg, entry_facts);
      const auto current_resolved = resolve_register(ret.reg);
      if (current_resolved != entry_resolved) {
        ret.reg = current_resolved;
      }
      break;
    }
    case Type::Equal: {
      auto &equal = derived_cast<Bytecode::Instruction::Equal &>(instr);
      equal.src1 = resolve_register(equal.src1);
      equal.src2 = resolve_register(equal.src2);
      invalidate(facts, equal.dst);
      break;
    }
    case Type::EqualImmediate: {
      auto &equal_imm = derived_cast<Bytecode::Instruction::EqualImmediate &>(instr);
      equal_imm.src = resolve_register(equal_imm.src);
      invalidate(facts, equal_imm.dst);
      break;
    }
    case Type::NotEqual: {
      auto &not_equal = derived_cast<Bytecode::Instruction::NotEqual &>(instr);
      not_equal.src1 = resolve_register(not_equal.src1);
      not_equal.src2 = resolve_register(not_equal.src2);
      invalidate(facts, not_equal.dst);
      break;
    }
    case Type::NotEqualImmediate: {
      auto &not_equal_imm =
          derived_cast<Bytecode::Instruction::NotEqualImmediate &>(instr);
      not_equal_imm.src = resolve_register(not_equal_imm.src);
      invalidate(facts, not_equal_imm.dst);
      break;
    }
    case Type::Add: {
      auto &add = derived_cast<Bytecode::Instruction::Add &>(instr);
      add.src1 = resolve_register(add.src1);
      add.src2 = resolve_register(add.src2);
      invalidate(facts, add.dst);
      break;
    }
    case Type::AddImmediate: {
      auto &add_imm = derived_cast<Bytecode::Instruction::AddImmediate &>(instr);
      add_imm.src = resolve_register(add_imm.src);
      invalidate(facts, add_imm.dst);
      break;
    }
    case Type::Subtract: {
      auto &subtract = derived_cast<Bytecode::Instruction::Subtract &>(instr);
      subtract.src1 = resolve_register(subtract.src1);
      subtract.src2 = resolve_register(subtract.src2);
      invalidate(facts, subtract.dst);
      break;
    }
    case Type::SubtractImmediate: {
      auto &subtract_imm =
          derived_cast<Bytecode::Instruction::SubtractImmediate &>(instr);
      subtract_imm.src = resolve_register(subtract_imm.src);
      invalidate(facts, subtract_imm.dst);
      break;
    }
    case Type::Multiply: {
      auto &multiply = derived_cast<Bytecode::Instruction::Multiply &>(instr);
      multiply.src1 = resolve_register(multiply.src1);
      multiply.src2 = resolve_register(multiply.src2);
      invalidate(facts, multiply.dst);
      break;
    }
    case Type::MultiplyImmediate: {
      auto &multiply_imm =
          derived_cast<Bytecode::Instruction::MultiplyImmediate &>(instr);
      multiply_imm.src = resolve_register(multiply_imm.src);
      invalidate(facts, multiply_imm.dst);
      break;
    }
    case Type::Divide: {
      auto &divide = derived_cast<Bytecode::Instruction::Divide &>(instr);
      divide.src1 = resolve_register(divide.src1);
      divide.src2 = resolve_register(divide.src2);
      invalidate(facts, divide.dst);
      break;
    }
    case Type::DivideImmediate: {
      auto &divide_imm = derived_cast<Bytecode::Instruction::DivideImmediate &>(instr);
      divide_imm.src = resolve_register(divide_imm.src);
      invalidate(facts, divide_imm.dst);
      break;
    }
    case Type::Modulo: {
      auto &modulo = derived_cast<Bytecode::Instruction::Modulo &>(instr);
      modulo.src1 = resolve_register(modulo.src1);
      modulo.src2 = resolve_register(modulo.src2);
      invalidate(facts, modulo.dst);
      break;
    }
    case Type::ModuloImmediate: {
      auto &modulo_imm = derived_cast<Bytecode::Instruction::ModuloImmediate &>(instr);
      modulo_imm.src = resolve_register(modulo_imm.src);
      invalidate(facts, modulo_imm.dst);
      break;
    }
    case Type::ArrayCreate: {
      auto &array_create = derived_cast<Bytecode::Instruction::ArrayCreate &>(instr);
      for (auto &element : array_create.elements) {
        element = resolve_register(element);
      }
      invalidate(facts, array_create.dst);
      break;
    }
    case Type::ArrayLiteralCreate: {
      auto &array_literal_create =
          derived_cast<Bytecode::Instruction::ArrayLiteralCreate &>(instr);
      invalidate(facts, array_literal_create.dst);
      break;
    }
    case Type::ArrayLoad: {
      auto &array_load = derived_cast<Bytecode::Instruction::ArrayLoad &>(instr);
      array_load.array = resolve_register(array_load.array);
      array_load.index = resolve_register(array_load.index);
      invalidate(facts, array_load.dst);
      break;
    }
    case Type::ArrayLoadImmediate: {
      auto &array_load_imm =
          derived_cast<Bytecode::Instruction::ArrayLoadImmediate &>(instr);
      array_load_imm.array = resolve_register(array_load_imm.array);
      invalidate(facts, array_load_imm.dst);
      break;
    }
    case Type::ArrayStore: {
      auto &array_store = derived_cast<Bytecode::Instruction::ArrayStore &>(instr);
      array_store.array = resolve_register(array_store.array);
      array_store.index = resolve_register(array_store.index);
      array_store.value = resolve_register(array_store.value);
      break;
    }
    case Type::StructCreate: {
      auto &struct_create = derived_cast<Bytecode::Instruction::StructCreate &>(instr);
      for (auto &[name, reg] : struct_create.fields) {
        reg = resolve_register(reg);
      }
      invalidate(facts, struct_create.dst);
      break;
    }
    case Type::StructLiteralCreate: {
      auto &struct_literal_create =
          derived_cast<Bytecode::Instruction::StructLiteralCreate &>(instr);
      invalidate(facts, struct_literal_create.dst);
      break;
    }
    case Type::StructLoad: {
      auto &struct_load = derived_cast<Bytecode::Instruction::StructLoad &>(instr);
      struct_load.object = resolve_register(struct_load.object);
      invalidate(facts, struct_load.dst);
      break;
    }
    case Type::AddressOf: {
      auto &address_of = derived_cast<Bytecode::Instruction::AddressOf &>(instr);
      // `AddressOf` must preserve the exact source register identity. Rewriting
      // through copy aliases changes pointee identity and breaks semantics.
      invalidate(facts, address_of.dst);
      break;
    }
    case Type::LoadIndirect: {
      auto &load_indirect = derived_cast<Bytecode::Instruction::LoadIndirect &>(instr);
      load_indirect.pointer = resolve_register(load_indirect.pointer);
      invalidate(facts, load_indirect.dst);
      break;
    }
    case Type::Negate: {
      auto &negate = derived_cast<Bytecode::Instruction::Negate &>(instr);
      negate.src = resolve_register(negate.src);
      invalidate(facts, negate.dst);
      break;
    }
    case Type::LogicalNot: {
      auto &logical_not = derived_cast<Bytecode::Instruction::LogicalNot &>(instr);
      logical_not.src = resolve_register(logical_not.src);
      invalidate(facts, logical_not.dst);
      break;
    }
  }
}

}  // namespace

void BytecodeOptimizer::copy_propagation(std::vector<Bytecode::BasicBlock> &blocks) {
  const auto successors = build_successors(blocks);
  const auto predecessors = build_predecessors(successors);

  std::vector<FactMap> in_states(blocks.size());
  std::vector<FactMap> out_states(blocks.size());
  std::vector<bool> out_initialized(blocks.size(), false);

  std::deque<size_t> worklist;
  std::vector<bool> queued(blocks.size(), false);
  for (size_t i = 0; i < blocks.size(); ++i) {
    worklist.push_back(i);
    queued[i] = true;
  }

  while (!worklist.empty()) {
    const auto block_index = worklist.front();
    worklist.pop_front();
    queued[block_index] = false;

    auto in_state =
        meet_predecessors(block_index, predecessors, out_states, out_initialized);
    auto out_state = in_state;

    for (const auto &instr_ptr : blocks[block_index].instructions) {
      transfer_instruction(*instr_ptr, out_state);
    }

    if (in_state == in_states[block_index] && out_state == out_states[block_index]) {
      continue;
    }

    in_states[block_index] = std::move(in_state);
    out_states[block_index] = std::move(out_state);
    out_initialized[block_index] = true;

    for (const auto succ : successors[block_index]) {
      if (!queued[succ]) {
        worklist.push_back(succ);
        queued[succ] = true;
      }
    }
  }

  for (size_t i = 0; i < blocks.size(); ++i) {
    auto facts = in_states[i];
    const auto entry_facts = facts;
    for (auto &instr_ptr : blocks[i].instructions) {
      rewrite_instruction(instr_ptr, facts, entry_facts);
    }

    std::erase_if(blocks[i].instructions, [](const auto &instr_ptr) {
      if (instr_ptr->type() != Type::Move) {
        return false;
      }
      const auto &move = derived_cast<const Bytecode::Instruction::Move &>(*instr_ptr);
      return move.dst == move.src;
    });
  }
}

}  // namespace kai
