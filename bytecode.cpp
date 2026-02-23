#include "bytecode.h"

#include <cassert>
#include <cstdio>

namespace kai {
namespace bytecode {

using namespace ast;

namespace {
bool has_terminator(const Bytecode::BasicBlock &block) {
  if (block.instructions.empty()) {
    return false;
  }
  const auto last_type = block.instructions.back()->type();
  return last_type == Bytecode::Instruction::Type::Jump ||
         last_type == Bytecode::Instruction::Type::JumpConditional ||
         last_type == Bytecode::Instruction::Type::Return;
}
}  // namespace

Bytecode::Instruction::Instruction(Type type) : type_(type) {}

Bytecode::Instruction::Type Bytecode::Instruction::type() const { return type_; }

Bytecode::Instruction::Move::Move(Register dst, Register src)
    : Bytecode::Instruction(Type::Move), dst(dst), src(src) {}

void Bytecode::Instruction::Move::dump() const {
  std::printf("Move r%zu, r%zu", dst, src);
}

Bytecode::Instruction::Load::Load(Register dst, Value value)
    : Bytecode::Instruction(Type::Load), dst(dst), value(value) {}

void Bytecode::Instruction::Load::dump() const {
  std::printf("Load r%zu, %zu", dst, value);
}

Bytecode::Instruction::LessThan::LessThan(Register dst, Register lhs, Register rhs)
    : Bytecode::Instruction(Type::LessThan), dst(dst), lhs(lhs), rhs(rhs) {}

void Bytecode::Instruction::LessThan::dump() const {
  std::printf("LessThan r%zu, r%zu, r%zu", dst, lhs, rhs);
}

Bytecode::Instruction::Jump::Jump(Label label)
    : Bytecode::Instruction(Type::Jump), label(label) {}

void Bytecode::Instruction::Jump::dump() const { std::printf("Jump @%zu", label); }

Bytecode::Instruction::JumpConditional::JumpConditional(Register cond, Label label1,
                                                        Label label2)
    : Bytecode::Instruction(Type::JumpConditional),
      cond(cond),
      label1(label1),
      label2(label2) {}

void Bytecode::Instruction::JumpConditional::dump() const {
  std::printf("JumpConditional r%zu, @%zu, @%zu", cond, label1, label2);
}

Bytecode::Instruction::Call::Call(Register dst, Label label)
    : Bytecode::Instruction(Type::Call), dst(dst), label(label) {}

void Bytecode::Instruction::Call::dump() const { std::printf("Call r%zu, @%zu", dst, label); }

Bytecode::Instruction::Return::Return(Register reg)
    : Bytecode::Instruction(Type::Return), reg(reg) {}

void Bytecode::Instruction::Return::dump() const { std::printf("Return r%zu", reg); }

Bytecode::Instruction::Equal::Equal(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Equal), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Equal::dump() const {
  std::printf("Equal r%zu, r%zu, r%zu", dst, src1, src2);
}

Bytecode::Instruction::Add::Add(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Add), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Add::dump() const {
  std::printf("Add r%zu, r%zu, r%zu", dst, src1, src2);
}

Bytecode::Instruction::Subtract::Subtract(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Subtract), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Subtract::dump() const {
  std::printf("Subtract r%zu, r%zu, r%zu", dst, src1, src2);
}

Bytecode::Instruction::AddImmediate::AddImmediate(Register dst, Register src, Value value)
    : Bytecode::Instruction(Type::AddImmediate), dst(dst), src(src), value(value) {}

void Bytecode::Instruction::AddImmediate::dump() const {
  std::printf("Add r%zu, r%zu, %zu", dst, src, value);
}

Bytecode::Instruction::Multiply::Multiply(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Multiply), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Multiply::dump() const {
  std::printf("Multiply r%zu, r%zu, r%zu", dst, src1, src2);
}

Bytecode::Instruction::Divide::Divide(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Divide), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Divide::dump() const {
  std::printf("Divide r%zu, r%zu, r%zu", dst, src1, src2);
}

Bytecode::Instruction::Modulo::Modulo(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Modulo), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Modulo::dump() const {
  std::printf("Modulo r%zu, r%zu, r%zu", dst, src1, src2);
}

Bytecode::Instruction::ArrayCreate::ArrayCreate(Register dst,
                                                std::vector<Register> elements)
    : Bytecode::Instruction(Type::ArrayCreate), dst(dst), elements(std::move(elements)) {}

void Bytecode::Instruction::ArrayCreate::dump() const {
  std::printf("ArrayCreate r%zu, [", dst);
  for (size_t i = 0; i < elements.size(); ++i) {
    if (i != 0) {
      std::printf(", ");
    }
    std::printf("r%zu", elements[i]);
  }
  std::printf("]");
}

Bytecode::Instruction::ArrayLoad::ArrayLoad(Register dst, Register array, Register index)
    : Bytecode::Instruction(Type::ArrayLoad), dst(dst), array(array), index(index) {}

void Bytecode::Instruction::ArrayLoad::dump() const {
  std::printf("ArrayLoad r%zu, r%zu, r%zu", dst, array, index);
}

Bytecode::Instruction::ArrayStore::ArrayStore(Register array, Register index, Register value)
    : Bytecode::Instruction(Type::ArrayStore),
      array(array),
      index(index),
      value(value) {}

void Bytecode::Instruction::ArrayStore::dump() const {
  std::printf("ArrayStore r%zu, r%zu, r%zu", array, index, value);
}

void Bytecode::BasicBlock::dump() const {
  for (const auto &instr : instructions) {
    std::printf("  ");
    instr->dump();
    std::printf("\n");
  }
}

u64 Bytecode::RegisterAllocator::count() const { return register_count; }

Bytecode::Register Bytecode::RegisterAllocator::allocate() { return register_count++; }

Bytecode::Register Bytecode::RegisterAllocator::current() { return register_count - 1; }

void BytecodeGenerator::visit(const Ast &ast) {
  switch (ast.type) {
    case Ast::Type::FunctionDeclaration:
      visit_function_declaration(ast_cast<Ast::FunctionDeclaration const &>(ast));
      break;
    case Ast::Type::Variable:
      visit_variable(ast_cast<Ast::Variable const &>(ast));
      break;
    case Ast::Type::Literal:
      visit_literal(ast_cast<Ast::Literal const &>(ast));
      break;
    case Ast::Type::VariableDeclaration:
      visit_variable_declaration(ast_cast<Ast::VariableDeclaration const &>(ast));
      break;
    case Ast::Type::LessThan:
      visit_less_than(ast_cast<Ast::LessThan const &>(ast));
      break;
    case Ast::Type::Increment:
      visit_increment(ast_cast<Ast::Increment const &>(ast));
      break;
    case Ast::Type::Block:
      visit_block(ast_cast<Ast::Block const &>(ast));
      break;
    case Ast::Type::IfElse:
      visit_if_else(ast_cast<Ast::IfElse const &>(ast));
      break;
    case Ast::Type::While:
      visit_while(ast_cast<Ast::While const &>(ast));
      break;
    case Ast::Type::FunctionCall:
      visit_function_call(ast_cast<Ast::FunctionCall const &>(ast));
      break;
    case Ast::Type::Return:
      visit_return(ast_cast<Ast::Return const &>(ast));
      break;
    case Ast::Type::Equal:
      visit_equal(ast_cast<Ast::Equal const &>(ast));
      break;
    case Ast::Type::Add:
      visit_add(ast_cast<Ast::Add const &>(ast));
      break;
    case Ast::Type::Subtract:
      visit_subtract(ast_cast<Ast::Subtract const &>(ast));
      break;
    case Ast::Type::Multiply:
      visit_multiply(ast_cast<Ast::Multiply const &>(ast));
      break;
    case Ast::Type::Divide:
      visit_divide(ast_cast<Ast::Divide const &>(ast));
      break;
    case Ast::Type::Modulo:
      visit_modulo(ast_cast<Ast::Modulo const &>(ast));
      break;
    case Ast::Type::ArrayLiteral:
      visit_array_literal(ast_cast<Ast::ArrayLiteral const &>(ast));
      break;
    case Ast::Type::Index:
      visit_index(ast_cast<Ast::Index const &>(ast));
      break;
    case Ast::Type::IndexAssignment:
      visit_index_assignment(ast_cast<Ast::IndexAssignment const &>(ast));
      break;
    case Ast::Type::Assignment:
      visit_assignment(ast_cast<Ast::Assignment const &>(ast));
      break;
    default:
      assert(false);
  }
}

void BytecodeGenerator::visit_function_declaration(
    const Ast::FunctionDeclaration &func_decl) {
  auto outer_vars = vars_;
  auto &jump_to_after_decl = current_block().append<Bytecode::Instruction::Jump>(-1);
  auto function_label = static_cast<Bytecode::Label>(blocks_.size());
  functions_[func_decl.name] = function_label;
  if (const auto unresolved_it = unresolved_calls_.find(func_decl.name);
      unresolved_it != unresolved_calls_.end()) {
    for (auto *call : unresolved_it->second) {
      call->label = function_label;
    }
    unresolved_calls_.erase(unresolved_it);
  }
  visit_block(*func_decl.body);
  if (!has_terminator(current_block())) {
    const auto reg = reg_alloc_.allocate();
    current_block().append<Bytecode::Instruction::Load>(reg, 0);
    current_block().append<Bytecode::Instruction::Return>(reg);
  }
  auto after_decl_label = static_cast<Bytecode::Label>(blocks_.size());
  blocks_.emplace_back();
  jump_to_after_decl.label = after_decl_label;
  vars_ = std::move(outer_vars);
}

void BytecodeGenerator::visit_block(const Ast::Block &block) {
  blocks_.emplace_back();
  for (const auto &child : block.children) {
    visit(*child);
  }
}

void BytecodeGenerator::finalize() {
  assert(unresolved_calls_.empty());
  for (size_t i = 0; i < blocks_.size(); ++i) {
    auto &block = blocks_[i];
    const bool block_has_terminator = has_terminator(block);

    if (!block_has_terminator && i + 1 < blocks_.size()) {
      block.append<Bytecode::Instruction::Jump>(i + 1);
    } else if (!block_has_terminator) {
      auto reg = reg_alloc_.allocate();
      block.append<Bytecode::Instruction::Load>(reg, 0);
      block.append<Bytecode::Instruction::Return>(reg);
    }
  }
}

void BytecodeGenerator::dump() const {
  for (size_t i = 0; i < blocks_.size(); ++i) {
    std::printf("%zu:\n", i);
    blocks_[i].dump();
  }
}

const std::vector<Bytecode::BasicBlock> &BytecodeGenerator::blocks() const {
  return blocks_;
}

Bytecode::BasicBlock &BytecodeGenerator::current_block() {
  if (blocks_.empty()) {
    blocks_.emplace_back();
  }
  return blocks_.back();
}

Bytecode::Label BytecodeGenerator::current_label() {
  assert(!blocks_.empty());
  return blocks_.size() - 1;
}

void BytecodeGenerator::visit_variable(const Ast::Variable &var) {
  current_block().append<Bytecode::Instruction::Move>(reg_alloc_.allocate(), vars_[var.name]);
}

void BytecodeGenerator::visit_literal(const Ast::Literal &literal) {
  current_block().append<Bytecode::Instruction::Load>(reg_alloc_.allocate(), literal.value);
}

void BytecodeGenerator::visit_variable_declaration(
    const Ast::VariableDeclaration &var_decl) {
  visit(*var_decl.initializer);
  auto reg_id = reg_alloc_.current();
  auto dst_reg = reg_alloc_.allocate();
  current_block().append<Bytecode::Instruction::Move>(dst_reg, reg_id);
  vars_[var_decl.name] = dst_reg;
}

void BytecodeGenerator::visit_less_than(const Ast::LessThan &less_than) {
  visit(*less_than.left);
  auto left_reg = reg_alloc_.current();
  visit(*less_than.right);
  auto right_reg = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::LessThan>(reg_alloc_.allocate(), left_reg,
                                                          right_reg);
}

void BytecodeGenerator::visit_increment(const Ast::Increment &increment) {
  visit(*increment.variable);
  auto reg_id = reg_alloc_.current();
  auto dst_reg = reg_alloc_.allocate();
  current_block().append<Bytecode::Instruction::AddImmediate>(dst_reg, reg_id, 1);
  current_block().append<Bytecode::Instruction::Move>(vars_[increment.variable->name],
                                                      dst_reg);
}

void BytecodeGenerator::visit_if_else(const Ast::IfElse &ifelse) {
  blocks_.emplace_back();
  visit(*ifelse.condition);
  auto reg_id = reg_alloc_.current();
  auto &jump_conditional =
      current_block().append<Bytecode::Instruction::JumpConditional>(reg_id, -1, -1);

  auto if_body_label = static_cast<Bytecode::Label>(blocks_.size());
  visit_block(*ifelse.body);
  auto &jump_to_end = current_block().append<Bytecode::Instruction::Jump>(-1);

  auto else_body_label = static_cast<Bytecode::Label>(blocks_.size());
  visit_block(*ifelse.else_body);
  auto end_label = static_cast<Bytecode::Label>(blocks_.size());
  blocks_.emplace_back();

  jump_conditional.label1 = if_body_label;
  jump_conditional.label2 = else_body_label;
  jump_to_end.label = end_label;
}

void BytecodeGenerator::visit_while(const Ast::While &while_) {
  blocks_.emplace_back();
  auto condition_label = current_label();
  visit(*while_.condition);
  auto reg_id = reg_alloc_.current();
  auto &jump_conditional =
      current_block().append<Bytecode::Instruction::JumpConditional>(reg_id, -1, -1);

  auto body_label = static_cast<Bytecode::Label>(blocks_.size());
  visit_block(*while_.body);
  current_block().append<Bytecode::Instruction::Jump>(condition_label);
  auto end_label = static_cast<Bytecode::Label>(blocks_.size());
  blocks_.emplace_back();

  jump_conditional.label1 = body_label;
  jump_conditional.label2 = end_label;
}

void BytecodeGenerator::visit_function_call(const Ast::FunctionCall &function_call) {
  auto &call = current_block().append<Bytecode::Instruction::Call>(reg_alloc_.allocate(), 0);
  if (const auto it = functions_.find(function_call.name); it != functions_.end()) {
    call.label = it->second;
  } else {
    unresolved_calls_[function_call.name].push_back(&call);
  }
}

void BytecodeGenerator::visit_return(const Ast::Return &return_) {
  visit(*return_.value);
  current_block().append<Bytecode::Instruction::Return>(reg_alloc_.current());
}

void BytecodeGenerator::visit_equal(const Ast::Equal &equal) {
  visit(*equal.left);
  auto reg_left = reg_alloc_.current();
  visit(*equal.right);
  auto reg_right = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::Equal>(reg_alloc_.allocate(), reg_left,
                                                       reg_right);
}

void BytecodeGenerator::visit_add(const Ast::Add &add) {
  visit(*add.left);
  auto reg_left = reg_alloc_.current();
  visit(*add.right);
  auto reg_right = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::Add>(reg_alloc_.allocate(), reg_left,
                                                     reg_right);
}

void BytecodeGenerator::visit_subtract(const Ast::Subtract &subtract) {
  visit(*subtract.left);
  auto reg_left = reg_alloc_.current();
  visit(*subtract.right);
  auto reg_right = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::Subtract>(reg_alloc_.allocate(), reg_left,
                                                          reg_right);
}

void BytecodeGenerator::visit_multiply(const Ast::Multiply &multiply) {
  visit(*multiply.left);
  auto reg_left = reg_alloc_.current();
  visit(*multiply.right);
  auto reg_right = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::Multiply>(reg_alloc_.allocate(), reg_left,
                                                          reg_right);
}

void BytecodeGenerator::visit_divide(const Ast::Divide &divide) {
  visit(*divide.left);
  auto reg_left = reg_alloc_.current();
  visit(*divide.right);
  auto reg_right = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::Divide>(reg_alloc_.allocate(), reg_left,
                                                        reg_right);
}

void BytecodeGenerator::visit_modulo(const Ast::Modulo &modulo) {
  visit(*modulo.left);
  auto reg_left = reg_alloc_.current();
  visit(*modulo.right);
  auto reg_right = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::Modulo>(reg_alloc_.allocate(), reg_left,
                                                        reg_right);
}

void BytecodeGenerator::visit_array_literal(const Ast::ArrayLiteral &array_literal) {
  std::vector<Bytecode::Register> element_regs;
  element_regs.reserve(array_literal.elements.size());
  for (const auto &element : array_literal.elements) {
    visit(*element);
    element_regs.push_back(reg_alloc_.current());
  }
  current_block().append<Bytecode::Instruction::ArrayCreate>(reg_alloc_.allocate(),
                                                             std::move(element_regs));
}

void BytecodeGenerator::visit_index(const Ast::Index &index) {
  visit(*index.array);
  auto array_reg = reg_alloc_.current();
  visit(*index.index);
  auto index_reg = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::ArrayLoad>(reg_alloc_.allocate(), array_reg,
                                                           index_reg);
}

void BytecodeGenerator::visit_index_assignment(
    const Ast::IndexAssignment &index_assignment) {
  visit(*index_assignment.array);
  const auto array_reg = reg_alloc_.current();
  visit(*index_assignment.index);
  const auto index_reg = reg_alloc_.current();
  visit(*index_assignment.value);
  const auto value_reg = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::ArrayStore>(array_reg, index_reg, value_reg);
}

void BytecodeGenerator::visit_assignment(const Ast::Assignment &assignment) {
  visit(*assignment.value);
  current_block().append<Bytecode::Instruction::Move>(vars_[assignment.name],
                                                      reg_alloc_.current());
}

Bytecode::Value BytecodeInterpreter::interpret(
    const std::vector<Bytecode::BasicBlock> &blocks) {
  assert(!blocks.empty());
  block_index = 0;
  instr_index_ = 0;
  call_stack_.clear();
  registers_.clear();
  arrays_.clear();
  next_array_id_ = 1;

  for (;;) {
    assert(block_index < blocks.size());
    const auto &block = blocks[block_index];
    assert(instr_index_ < block.instructions.size());
    const auto &instr = block.instructions[instr_index_];
    switch (instr->type()) {
      case Bytecode::Instruction::Type::Move:
        interpret_move(ast::derived_cast<Bytecode::Instruction::Move const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::Load:
        interpret_load(ast::derived_cast<Bytecode::Instruction::Load const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::LessThan:
        interpret_less_than(ast::derived_cast<Bytecode::Instruction::LessThan const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::Jump:
        interpret_jump(ast::derived_cast<Bytecode::Instruction::Jump const &>(*instr));
        break;
      case Bytecode::Instruction::Type::JumpConditional:
        interpret_jump_conditional(
            ast::derived_cast<Bytecode::Instruction::JumpConditional const &>(*instr));
        break;
      case Bytecode::Instruction::Type::Call:
        interpret_call(ast::derived_cast<Bytecode::Instruction::Call const &>(*instr),
                       instr_index_ + 1);
        break;
      case Bytecode::Instruction::Type::Return: {
        const auto reg = ast::derived_cast<Bytecode::Instruction::Return const &>(*instr).reg;
        const auto value = registers_[reg];
        if (call_stack_.empty()) {
          return value;
        }
        auto frame = std::move(call_stack_.back());
        call_stack_.pop_back();
        registers_ = std::move(frame.registers);
        registers_[frame.dst_register] = value;
        block_index = frame.return_block_index;
        instr_index_ = frame.return_instr_index;
        break;
      }
      case Bytecode::Instruction::Type::Equal:
        interpret_equal(ast::derived_cast<Bytecode::Instruction::Equal const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::Add:
        interpret_add(ast::derived_cast<Bytecode::Instruction::Add const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::Subtract:
        interpret_subtract(ast::derived_cast<Bytecode::Instruction::Subtract const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::AddImmediate:
        interpret_add_immediate(
            ast::derived_cast<Bytecode::Instruction::AddImmediate const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::Multiply:
        interpret_multiply(ast::derived_cast<Bytecode::Instruction::Multiply const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::Divide:
        interpret_divide(ast::derived_cast<Bytecode::Instruction::Divide const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::Modulo:
        interpret_modulo(ast::derived_cast<Bytecode::Instruction::Modulo const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::ArrayCreate:
        interpret_array_create(
            ast::derived_cast<Bytecode::Instruction::ArrayCreate const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::ArrayLoad:
        interpret_array_load(ast::derived_cast<Bytecode::Instruction::ArrayLoad const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::ArrayStore:
        interpret_array_store(
            ast::derived_cast<Bytecode::Instruction::ArrayStore const &>(*instr));
        ++instr_index_;
        break;
      default:
        assert(false);
        break;
    }
  }
  assert(false);
  return Bytecode::Value(-1);
}

void BytecodeInterpreter::interpret_move(const Bytecode::Instruction::Move &move) {
  registers_[move.dst] = registers_[move.src];
}

void BytecodeInterpreter::interpret_load(const Bytecode::Instruction::Load &load) {
  registers_[load.dst] = load.value;
}

void BytecodeInterpreter::interpret_less_than(
    const Bytecode::Instruction::LessThan &less_than) {
  registers_[less_than.dst] = registers_[less_than.lhs] < registers_[less_than.rhs];
}

void BytecodeInterpreter::interpret_jump(const Bytecode::Instruction::Jump &jump) {
  block_index = jump.label;
  instr_index_ = 0;
}

void BytecodeInterpreter::interpret_jump_conditional(
    const Bytecode::Instruction::JumpConditional &jump_cond) {
  if (registers_[jump_cond.cond]) {
    block_index = jump_cond.label1;
  } else {
    block_index = jump_cond.label2;
  }
  instr_index_ = 0;
}

void BytecodeInterpreter::interpret_call(const Bytecode::Instruction::Call &call,
                                         size_t next_instr_index) {
  call_stack_.push_back({block_index, next_instr_index, call.dst, registers_});
  block_index = call.label;
  instr_index_ = 0;
}

void BytecodeInterpreter::interpret_equal(const Bytecode::Instruction::Equal &equal) {
  registers_[equal.dst] = registers_[equal.src1] == registers_[equal.src2];
}

void BytecodeInterpreter::interpret_add(const Bytecode::Instruction::Add &add) {
  registers_[add.dst] = registers_[add.src1] + registers_[add.src2];
}

void BytecodeInterpreter::interpret_subtract(
    const Bytecode::Instruction::Subtract &subtract) {
  registers_[subtract.dst] = registers_[subtract.src1] - registers_[subtract.src2];
}

void BytecodeInterpreter::interpret_add_immediate(
    const Bytecode::Instruction::AddImmediate &add_imm) {
  registers_[add_imm.dst] = registers_[add_imm.src] + add_imm.value;
}

void BytecodeInterpreter::interpret_multiply(
    const Bytecode::Instruction::Multiply &multiply) {
  registers_[multiply.dst] = registers_[multiply.src1] * registers_[multiply.src2];
}

void BytecodeInterpreter::interpret_divide(const Bytecode::Instruction::Divide &divide) {
  registers_[divide.dst] = registers_[divide.src1] / registers_[divide.src2];
}

void BytecodeInterpreter::interpret_modulo(const Bytecode::Instruction::Modulo &modulo) {
  registers_[modulo.dst] = registers_[modulo.src1] % registers_[modulo.src2];
}

void BytecodeInterpreter::interpret_array_create(
    const Bytecode::Instruction::ArrayCreate &array_create) {
  auto array_id = next_array_id_++;
  auto &array = arrays_[array_id];
  array.reserve(array_create.elements.size());
  for (auto element_reg : array_create.elements) {
    array.push_back(registers_[element_reg]);
  }
  registers_[array_create.dst] = array_id;
}

void BytecodeInterpreter::interpret_array_load(
    const Bytecode::Instruction::ArrayLoad &array_load) {
  auto array_id = registers_[array_load.array];
  auto index = registers_[array_load.index];
  const auto it = arrays_.find(array_id);
  assert(it != arrays_.end());
  assert(index < it->second.size());
  registers_[array_load.dst] = it->second[index];
}

void BytecodeInterpreter::interpret_array_store(
    const Bytecode::Instruction::ArrayStore &array_store) {
  const auto array_id = registers_[array_store.array];
  const auto index = registers_[array_store.index];
  const auto value = registers_[array_store.value];
  const auto it = arrays_.find(array_id);
  assert(it != arrays_.end());
  assert(index < it->second.size());
  it->second[index] = value;
}

}  // namespace bytecode
}  // namespace kai
