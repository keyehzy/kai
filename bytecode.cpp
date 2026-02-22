#include "bytecode.h"

#include <cassert>
#include <cstdio>

namespace kai {
namespace bytecode {

using namespace ast;

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

Bytecode::Instruction::Return::Return(Register reg)
    : Bytecode::Instruction(Type::Return), reg(reg) {}

void Bytecode::Instruction::Return::dump() const { std::printf("Return r%zu", reg); }

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
    case Ast::Type::Return:
      visit_return(ast_cast<Ast::Return const &>(ast));
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
    case Ast::Type::Assignment:
      visit_assignment(ast_cast<Ast::Assignment const &>(ast));
      break;
    default:
      assert(false);
  }
}

void BytecodeGenerator::visit_function_declaration(
    const Ast::FunctionDeclaration &func_decl) {
  visit_block(*func_decl.body);
}

void BytecodeGenerator::visit_block(const Ast::Block &block) {
  blocks_.emplace_back();
  for (const auto &child : block.children) {
    visit(*child);
  }
}

void BytecodeGenerator::finalize() {
  for (size_t i = 0; i < blocks_.size(); ++i) {
    auto &block = blocks_[i];
    bool has_terminator = false;
    if (!block.instructions.empty()) {
      auto last_type = block.instructions.back()->type();
      has_terminator = last_type == Bytecode::Instruction::Type::Jump ||
                       last_type == Bytecode::Instruction::Type::JumpConditional ||
                       last_type == Bytecode::Instruction::Type::Return;
    }

    if (!has_terminator && i + 1 < blocks_.size()) {
      block.append<Bytecode::Instruction::Jump>(i + 1);
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

void BytecodeGenerator::visit_return(const Ast::Return &return_) {
  visit(*return_.value);
  current_block().append<Bytecode::Instruction::Return>(reg_alloc_.current());
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

void BytecodeGenerator::visit_assignment(const Ast::Assignment &assignment) {
  visit(*assignment.value);
  current_block().append<Bytecode::Instruction::Move>(vars_[assignment.name],
                                                      reg_alloc_.current());
}

Bytecode::Value BytecodeInterpreter::interpret(
    const std::vector<Bytecode::BasicBlock> &blocks) {
  for (;;) {
    const auto &block = blocks[block_index];
    for (const auto &instr : block.instructions) {
      switch (instr->type()) {
        case Bytecode::Instruction::Type::Move:
          interpret_move(ast::derived_cast<Bytecode::Instruction::Move const &>(*instr));
          break;
        case Bytecode::Instruction::Type::Load:
          interpret_load(ast::derived_cast<Bytecode::Instruction::Load const &>(*instr));
          break;
        case Bytecode::Instruction::Type::LessThan:
          interpret_less_than(
              ast::derived_cast<Bytecode::Instruction::LessThan const &>(*instr));
          break;
        case Bytecode::Instruction::Type::Jump:
          interpret_jump(ast::derived_cast<Bytecode::Instruction::Jump const &>(*instr));
          break;
        case Bytecode::Instruction::Type::JumpConditional:
          interpret_jump_conditional(
              ast::derived_cast<Bytecode::Instruction::JumpConditional const &>(*instr));
          break;
        case Bytecode::Instruction::Type::Return:
          return registers_[ast::derived_cast<Bytecode::Instruction::Return const &>(*instr)
                                .reg];
        case Bytecode::Instruction::Type::Add:
          interpret_add(ast::derived_cast<Bytecode::Instruction::Add const &>(*instr));
          break;
        case Bytecode::Instruction::Type::Subtract:
          interpret_subtract(
              ast::derived_cast<Bytecode::Instruction::Subtract const &>(*instr));
          break;
        case Bytecode::Instruction::Type::AddImmediate:
          interpret_add_immediate(
              ast::derived_cast<Bytecode::Instruction::AddImmediate const &>(*instr));
          break;
        case Bytecode::Instruction::Type::Multiply:
          interpret_multiply(
              ast::derived_cast<Bytecode::Instruction::Multiply const &>(*instr));
          break;
        case Bytecode::Instruction::Type::Divide:
          interpret_divide(ast::derived_cast<Bytecode::Instruction::Divide const &>(*instr));
          break;
        default:
          assert(false);
          break;
      }
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
}

void BytecodeInterpreter::interpret_jump_conditional(
    const Bytecode::Instruction::JumpConditional &jump_cond) {
  if (registers_[jump_cond.cond]) {
    block_index = jump_cond.label1;
  } else {
    block_index = jump_cond.label2;
  }
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

}  // namespace bytecode
}  // namespace kai
