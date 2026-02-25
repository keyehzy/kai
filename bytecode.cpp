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

size_t register_count(const std::vector<Bytecode::BasicBlock> &blocks) {
  size_t count = 0;
  const auto track = [&count](Bytecode::Register reg) {
    const size_t needed = static_cast<size_t>(reg) + 1;
    if (needed > count) {
      count = needed;
    }
  };

  for (const auto &block : blocks) {
    for (const auto &instr : block.instructions) {
      switch (instr->type()) {
        case Bytecode::Instruction::Type::Move: {
          const auto &move =
              ast::derived_cast<const Bytecode::Instruction::Move &>(*instr);
          track(move.dst);
          track(move.src);
          break;
        }
        case Bytecode::Instruction::Type::Load: {
          const auto &load =
              ast::derived_cast<const Bytecode::Instruction::Load &>(*instr);
          track(load.dst);
          break;
        }
        case Bytecode::Instruction::Type::LessThan: {
          const auto &less_than =
              ast::derived_cast<const Bytecode::Instruction::LessThan &>(*instr);
          track(less_than.dst);
          track(less_than.lhs);
          track(less_than.rhs);
          break;
        }
        case Bytecode::Instruction::Type::GreaterThan: {
          const auto &greater_than =
              ast::derived_cast<const Bytecode::Instruction::GreaterThan &>(*instr);
          track(greater_than.dst);
          track(greater_than.lhs);
          track(greater_than.rhs);
          break;
        }
        case Bytecode::Instruction::Type::LessThanOrEqual: {
          const auto &less_than_or_equal =
              ast::derived_cast<const Bytecode::Instruction::LessThanOrEqual &>(*instr);
          track(less_than_or_equal.dst);
          track(less_than_or_equal.lhs);
          track(less_than_or_equal.rhs);
          break;
        }
        case Bytecode::Instruction::Type::GreaterThanOrEqual: {
          const auto &greater_than_or_equal = ast::derived_cast<
              const Bytecode::Instruction::GreaterThanOrEqual &>(*instr);
          track(greater_than_or_equal.dst);
          track(greater_than_or_equal.lhs);
          track(greater_than_or_equal.rhs);
          break;
        }
        case Bytecode::Instruction::Type::Jump:
          break;
        case Bytecode::Instruction::Type::JumpConditional: {
          const auto &jump_cond =
              ast::derived_cast<const Bytecode::Instruction::JumpConditional &>(*instr);
          track(jump_cond.cond);
          break;
        }
        case Bytecode::Instruction::Type::Call: {
          const auto &call =
              ast::derived_cast<const Bytecode::Instruction::Call &>(*instr);
          track(call.dst);
          for (const auto reg : call.arg_registers) {
            track(reg);
          }
          for (const auto reg : call.param_registers) {
            track(reg);
          }
          break;
        }
        case Bytecode::Instruction::Type::Return: {
          const auto &ret =
              ast::derived_cast<const Bytecode::Instruction::Return &>(*instr);
          track(ret.reg);
          break;
        }
        case Bytecode::Instruction::Type::Equal: {
          const auto &equal =
              ast::derived_cast<const Bytecode::Instruction::Equal &>(*instr);
          track(equal.dst);
          track(equal.src1);
          track(equal.src2);
          break;
        }
        case Bytecode::Instruction::Type::NotEqual: {
          const auto &not_equal =
              ast::derived_cast<const Bytecode::Instruction::NotEqual &>(*instr);
          track(not_equal.dst);
          track(not_equal.src1);
          track(not_equal.src2);
          break;
        }
        case Bytecode::Instruction::Type::Add: {
          const auto &add =
              ast::derived_cast<const Bytecode::Instruction::Add &>(*instr);
          track(add.dst);
          track(add.src1);
          track(add.src2);
          break;
        }
        case Bytecode::Instruction::Type::Subtract: {
          const auto &subtract =
              ast::derived_cast<const Bytecode::Instruction::Subtract &>(*instr);
          track(subtract.dst);
          track(subtract.src1);
          track(subtract.src2);
          break;
        }
        case Bytecode::Instruction::Type::AddImmediate: {
          const auto &add_imm =
              ast::derived_cast<const Bytecode::Instruction::AddImmediate &>(*instr);
          track(add_imm.dst);
          track(add_imm.src);
          break;
        }
        case Bytecode::Instruction::Type::Multiply: {
          const auto &multiply =
              ast::derived_cast<const Bytecode::Instruction::Multiply &>(*instr);
          track(multiply.dst);
          track(multiply.src1);
          track(multiply.src2);
          break;
        }
        case Bytecode::Instruction::Type::Divide: {
          const auto &divide =
              ast::derived_cast<const Bytecode::Instruction::Divide &>(*instr);
          track(divide.dst);
          track(divide.src1);
          track(divide.src2);
          break;
        }
        case Bytecode::Instruction::Type::Modulo: {
          const auto &modulo =
              ast::derived_cast<const Bytecode::Instruction::Modulo &>(*instr);
          track(modulo.dst);
          track(modulo.src1);
          track(modulo.src2);
          break;
        }
        case Bytecode::Instruction::Type::ArrayCreate: {
          const auto &array_create =
              ast::derived_cast<const Bytecode::Instruction::ArrayCreate &>(*instr);
          track(array_create.dst);
          for (const auto reg : array_create.elements) {
            track(reg);
          }
          break;
        }
        case Bytecode::Instruction::Type::ArrayLoad: {
          const auto &array_load =
              ast::derived_cast<const Bytecode::Instruction::ArrayLoad &>(*instr);
          track(array_load.dst);
          track(array_load.array);
          track(array_load.index);
          break;
        }
        case Bytecode::Instruction::Type::ArrayStore: {
          const auto &array_store =
              ast::derived_cast<const Bytecode::Instruction::ArrayStore &>(*instr);
          track(array_store.array);
          track(array_store.index);
          track(array_store.value);
          break;
        }
        case Bytecode::Instruction::Type::StructCreate: {
          const auto &struct_create =
              ast::derived_cast<const Bytecode::Instruction::StructCreate &>(*instr);
          track(struct_create.dst);
          for (const auto &field : struct_create.fields) {
            track(field.second);
          }
          break;
        }
        case Bytecode::Instruction::Type::StructLoad: {
          const auto &struct_load =
              ast::derived_cast<const Bytecode::Instruction::StructLoad &>(*instr);
          track(struct_load.dst);
          track(struct_load.object);
          break;
        }
        case Bytecode::Instruction::Type::Negate: {
          const auto &negate =
              ast::derived_cast<const Bytecode::Instruction::Negate &>(*instr);
          track(negate.dst);
          track(negate.src);
          break;
        }
        case Bytecode::Instruction::Type::LogicalNot: {
          const auto &logical_not =
              ast::derived_cast<const Bytecode::Instruction::LogicalNot &>(*instr);
          track(logical_not.dst);
          track(logical_not.src);
          break;
        }
        default:
          assert(false);
          break;
      }
    }
  }
  return count;
}
}  // namespace

Bytecode::Instruction::Instruction(Type type) : type_(type) {}

Bytecode::Instruction::Type Bytecode::Instruction::type() const { return type_; }

Bytecode::Instruction::Move::Move(Register dst, Register src)
    : Bytecode::Instruction(Type::Move), dst(dst), src(src) {}

void Bytecode::Instruction::Move::dump() const {
  std::printf("Move r%llu, r%llu", dst, src);
}

Bytecode::Instruction::Load::Load(Register dst, Value value)
    : Bytecode::Instruction(Type::Load), dst(dst), value(value) {}

void Bytecode::Instruction::Load::dump() const {
  std::printf("Load r%llu, %llu", dst, value);
}

Bytecode::Instruction::LessThan::LessThan(Register dst, Register lhs, Register rhs)
    : Bytecode::Instruction(Type::LessThan), dst(dst), lhs(lhs), rhs(rhs) {}

void Bytecode::Instruction::LessThan::dump() const {
  std::printf("LessThan r%llu, r%llu, r%llu", dst, lhs, rhs);
}

Bytecode::Instruction::GreaterThan::GreaterThan(Register dst, Register lhs,
                                                Register rhs)
    : Bytecode::Instruction(Type::GreaterThan), dst(dst), lhs(lhs), rhs(rhs) {}

void Bytecode::Instruction::GreaterThan::dump() const {
  std::printf("GreaterThan r%llu, r%llu, r%llu", dst, lhs, rhs);
}

Bytecode::Instruction::LessThanOrEqual::LessThanOrEqual(Register dst, Register lhs,
                                                        Register rhs)
    : Bytecode::Instruction(Type::LessThanOrEqual), dst(dst), lhs(lhs), rhs(rhs) {}

void Bytecode::Instruction::LessThanOrEqual::dump() const {
  std::printf("LessThanOrEqual r%llu, r%llu, r%llu", dst, lhs, rhs);
}

Bytecode::Instruction::GreaterThanOrEqual::GreaterThanOrEqual(Register dst,
                                                              Register lhs,
                                                              Register rhs)
    : Bytecode::Instruction(Type::GreaterThanOrEqual), dst(dst), lhs(lhs), rhs(rhs) {}

void Bytecode::Instruction::GreaterThanOrEqual::dump() const {
  std::printf("GreaterThanOrEqual r%llu, r%llu, r%llu", dst, lhs, rhs);
}

Bytecode::Instruction::Jump::Jump(Label label)
    : Bytecode::Instruction(Type::Jump), label(label) {}

void Bytecode::Instruction::Jump::dump() const { std::printf("Jump @%llu", label); }

Bytecode::Instruction::JumpConditional::JumpConditional(Register cond, Label label1,
                                                        Label label2)
    : Bytecode::Instruction(Type::JumpConditional),
      cond(cond),
      label1(label1),
      label2(label2) {}

void Bytecode::Instruction::JumpConditional::dump() const {
  std::printf("JumpConditional r%llu, @%llu, @%llu", cond, label1, label2);
}

Bytecode::Instruction::Call::Call(Register dst, Label label,
                                  std::vector<Register> arg_registers,
                                  std::vector<Register> param_registers)
    : Bytecode::Instruction(Type::Call),
      dst(dst),
      label(label),
      arg_registers(std::move(arg_registers)),
      param_registers(std::move(param_registers)) {}

void Bytecode::Instruction::Call::dump() const {
  std::printf("Call r%llu, @%llu, [", dst, label);
  for (size_t i = 0; i < arg_registers.size(); ++i) {
    if (i != 0) {
      std::printf(", ");
    }
    std::printf("r%llu", arg_registers[i]);
  }
  std::printf("]");
}

Bytecode::Instruction::Return::Return(Register reg)
    : Bytecode::Instruction(Type::Return), reg(reg) {}

void Bytecode::Instruction::Return::dump() const { std::printf("Return r%llu", reg); }

Bytecode::Instruction::Equal::Equal(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Equal), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Equal::dump() const {
  std::printf("Equal r%llu, r%llu, r%llu", dst, src1, src2);
}

Bytecode::Instruction::NotEqual::NotEqual(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::NotEqual), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::NotEqual::dump() const {
  std::printf("NotEqual r%llu, r%llu, r%llu", dst, src1, src2);
}

Bytecode::Instruction::Add::Add(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Add), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Add::dump() const {
  std::printf("Add r%llu, r%llu, r%llu", dst, src1, src2);
}

Bytecode::Instruction::Subtract::Subtract(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Subtract), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Subtract::dump() const {
  std::printf("Subtract r%llu, r%llu, r%llu", dst, src1, src2);
}

Bytecode::Instruction::AddImmediate::AddImmediate(Register dst, Register src, Value value)
    : Bytecode::Instruction(Type::AddImmediate), dst(dst), src(src), value(value) {}

void Bytecode::Instruction::AddImmediate::dump() const {
  std::printf("Add r%llu, r%llu, %llu", dst, src, value);
}

Bytecode::Instruction::Multiply::Multiply(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Multiply), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Multiply::dump() const {
  std::printf("Multiply r%llu, r%llu, r%llu", dst, src1, src2);
}

Bytecode::Instruction::Divide::Divide(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Divide), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Divide::dump() const {
  std::printf("Divide r%llu, r%llu, r%llu", dst, src1, src2);
}

Bytecode::Instruction::Modulo::Modulo(Register dst, Register src1, Register src2)
    : Bytecode::Instruction(Type::Modulo), dst(dst), src1(src1), src2(src2) {}

void Bytecode::Instruction::Modulo::dump() const {
  std::printf("Modulo r%llu, r%llu, r%llu", dst, src1, src2);
}

Bytecode::Instruction::ArrayCreate::ArrayCreate(Register dst,
                                                std::vector<Register> elements)
    : Bytecode::Instruction(Type::ArrayCreate), dst(dst), elements(std::move(elements)) {}

void Bytecode::Instruction::ArrayCreate::dump() const {
  std::printf("ArrayCreate r%llu, [", dst);
  for (size_t i = 0; i < elements.size(); ++i) {
    if (i != 0) {
      std::printf(", ");
    }
    std::printf("r%llu", elements[i]);
  }
  std::printf("]");
}

Bytecode::Instruction::ArrayLoad::ArrayLoad(Register dst, Register array, Register index)
    : Bytecode::Instruction(Type::ArrayLoad), dst(dst), array(array), index(index) {}

void Bytecode::Instruction::ArrayLoad::dump() const {
  std::printf("ArrayLoad r%llu, r%llu, r%llu", dst, array, index);
}

Bytecode::Instruction::ArrayStore::ArrayStore(Register array, Register index, Register value)
    : Bytecode::Instruction(Type::ArrayStore),
      array(array),
      index(index),
      value(value) {}

void Bytecode::Instruction::ArrayStore::dump() const {
  std::printf("ArrayStore r%llu, r%llu, r%llu", array, index, value);
}

Bytecode::Instruction::StructCreate::StructCreate(
    Register dst, std::vector<std::pair<std::string, Register>> fields)
    : Bytecode::Instruction(Type::StructCreate), dst(dst), fields(std::move(fields)) {}

void Bytecode::Instruction::StructCreate::dump() const {
  std::printf("StructCreate r%llu, {", dst);
  for (size_t i = 0; i < fields.size(); ++i) {
    if (i != 0) {
      std::printf(", ");
    }
    std::printf("%s: r%llu", fields[i].first.c_str(), fields[i].second);
  }
  std::printf("}");
}

Bytecode::Instruction::StructLoad::StructLoad(Register dst, Register object,
                                              std::string field)
    : Bytecode::Instruction(Type::StructLoad),
      dst(dst),
      object(object),
      field(std::move(field)) {}

void Bytecode::Instruction::StructLoad::dump() const {
  std::printf("StructLoad r%llu, r%llu, %s", dst, object, field.c_str());
}

Bytecode::Instruction::Negate::Negate(Register dst, Register src)
    : Bytecode::Instruction(Type::Negate), dst(dst), src(src) {}

void Bytecode::Instruction::Negate::dump() const {
  std::printf("Negate r%llu, r%llu", dst, src);
}

Bytecode::Instruction::LogicalNot::LogicalNot(Register dst, Register src)
    : Bytecode::Instruction(Type::LogicalNot), dst(dst), src(src) {}

void Bytecode::Instruction::LogicalNot::dump() const {
  std::printf("LogicalNot r%llu, r%llu", dst, src);
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
    case Ast::Type::GreaterThan:
      visit_greater_than(ast_cast<Ast::GreaterThan const &>(ast));
      break;
    case Ast::Type::LessThanOrEqual:
      visit_less_than_or_equal(ast_cast<Ast::LessThanOrEqual const &>(ast));
      break;
    case Ast::Type::GreaterThanOrEqual:
      visit_greater_than_or_equal(ast_cast<Ast::GreaterThanOrEqual const &>(ast));
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
    case Ast::Type::NotEqual:
      visit_not_equal(ast_cast<Ast::NotEqual const &>(ast));
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
    case Ast::Type::StructLiteral:
      visit_struct_literal(ast_cast<Ast::StructLiteral const &>(ast));
      break;
    case Ast::Type::FieldAccess:
      visit_field_access(ast_cast<Ast::FieldAccess const &>(ast));
      break;
    case Ast::Type::Assignment:
      visit_assignment(ast_cast<Ast::Assignment const &>(ast));
      break;
    case Ast::Type::Negate:
      visit_negate(ast_cast<Ast::Negate const &>(ast));
      break;
    case Ast::Type::UnaryPlus:
      visit_unary_plus(ast_cast<Ast::UnaryPlus const &>(ast));
      break;
    case Ast::Type::LogicalNot:
      visit_logical_not(ast_cast<Ast::LogicalNot const &>(ast));
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
  auto &parameter_registers = function_parameters_[func_decl.name];
  parameter_registers.clear();
  parameter_registers.reserve(func_decl.parameters.size());
  for (const auto &parameter_name : func_decl.parameters) {
    const auto parameter_register = reg_alloc_.allocate();
    parameter_registers.push_back(parameter_register);
    vars_[parameter_name] = parameter_register;
  }

  if (const auto unresolved_it = unresolved_calls_.find(func_decl.name);
      unresolved_it != unresolved_calls_.end()) {
    for (auto *call : unresolved_it->second) {
      call->label = function_label;
      assert(call->arg_registers.size() == parameter_registers.size());
      call->param_registers = parameter_registers;
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

std::vector<Bytecode::BasicBlock> &BytecodeGenerator::blocks() {
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

void BytecodeGenerator::visit_greater_than(const Ast::GreaterThan &greater_than) {
  visit(*greater_than.left);
  auto left_reg = reg_alloc_.current();
  visit(*greater_than.right);
  auto right_reg = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::GreaterThan>(reg_alloc_.allocate(),
                                                             left_reg, right_reg);
}

void BytecodeGenerator::visit_less_than_or_equal(
    const Ast::LessThanOrEqual &less_than_or_equal) {
  visit(*less_than_or_equal.left);
  const auto left_reg = reg_alloc_.current();
  visit(*less_than_or_equal.right);
  const auto right_reg = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::LessThanOrEqual>(reg_alloc_.allocate(),
                                                                 left_reg, right_reg);
}

void BytecodeGenerator::visit_greater_than_or_equal(
    const Ast::GreaterThanOrEqual &greater_than_or_equal) {
  visit(*greater_than_or_equal.left);
  const auto left_reg = reg_alloc_.current();
  visit(*greater_than_or_equal.right);
  const auto right_reg = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::GreaterThanOrEqual>(
      reg_alloc_.allocate(), left_reg, right_reg);
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
  if (!current_block().instructions.empty()) {
    blocks_.emplace_back();
  }
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
  if (!current_block().instructions.empty()) {
    blocks_.emplace_back();
  }
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
  std::vector<Bytecode::Register> arg_registers;
  arg_registers.reserve(function_call.arguments.size());
  for (const auto &argument : function_call.arguments) {
    visit(*argument);
    arg_registers.push_back(reg_alloc_.current());
  }

  auto &call = current_block().append<Bytecode::Instruction::Call>(
      reg_alloc_.allocate(), 0, std::move(arg_registers));
  if (const auto it = functions_.find(function_call.name); it != functions_.end()) {
    call.label = it->second;
    const auto params_it = function_parameters_.find(function_call.name);
    assert(params_it != function_parameters_.end());
    assert(call.arg_registers.size() == params_it->second.size());
    call.param_registers = params_it->second;
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

void BytecodeGenerator::visit_not_equal(const Ast::NotEqual &not_equal) {
  visit(*not_equal.left);
  auto reg_left = reg_alloc_.current();
  visit(*not_equal.right);
  auto reg_right = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::NotEqual>(reg_alloc_.allocate(), reg_left,
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

void BytecodeGenerator::visit_struct_literal(const Ast::StructLiteral &struct_literal) {
  std::vector<std::pair<std::string, Bytecode::Register>> fields;
  fields.reserve(struct_literal.fields.size());
  for (const auto &field : struct_literal.fields) {
    visit(*field.second);
    fields.emplace_back(field.first, reg_alloc_.current());
  }
  current_block().append<Bytecode::Instruction::StructCreate>(reg_alloc_.allocate(),
                                                              std::move(fields));
}

void BytecodeGenerator::visit_field_access(const Ast::FieldAccess &field_access) {
  visit(*field_access.object);
  const auto object_reg = reg_alloc_.current();
  const auto dst_reg = reg_alloc_.allocate();
  current_block().append<Bytecode::Instruction::StructLoad>(
      dst_reg, object_reg, field_access.field);
}

void BytecodeGenerator::visit_assignment(const Ast::Assignment &assignment) {
  visit(*assignment.value);
  current_block().append<Bytecode::Instruction::Move>(vars_[assignment.name],
                                                      reg_alloc_.current());
}

void BytecodeGenerator::visit_negate(const Ast::Negate &negate) {
  visit(*negate.operand);
  auto src_reg = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::Negate>(reg_alloc_.allocate(), src_reg);
}

void BytecodeGenerator::visit_unary_plus(const Ast::UnaryPlus &unary_plus) {
  visit(*unary_plus.operand);
}

void BytecodeGenerator::visit_logical_not(const Ast::LogicalNot &logical_not) {
  visit(*logical_not.operand);
  auto src_reg = reg_alloc_.current();
  current_block().append<Bytecode::Instruction::LogicalNot>(reg_alloc_.allocate(), src_reg);
}

Bytecode::Value BytecodeInterpreter::interpret(
    const std::vector<Bytecode::BasicBlock> &blocks) {
  assert(!blocks.empty());
  block_index = 0;
  instr_index_ = 0;
  call_stack_.clear();
  register_count_ = register_count(blocks);
  frame_base_ = 0;
  register_stack_.assign(register_count_, 0);
  arrays_.clear();
  structs_.clear();
  next_heap_id_ = 1;

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
      case Bytecode::Instruction::Type::GreaterThan:
        interpret_greater_than(
            ast::derived_cast<Bytecode::Instruction::GreaterThan const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::LessThanOrEqual:
        interpret_less_than_or_equal(
            ast::derived_cast<Bytecode::Instruction::LessThanOrEqual const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::GreaterThanOrEqual:
        interpret_greater_than_or_equal(
            ast::derived_cast<Bytecode::Instruction::GreaterThanOrEqual const &>(*instr));
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
        const auto ret_reg = ast::derived_cast<Bytecode::Instruction::Return const &>(*instr).reg;
        const auto value = register_stack_[frame_base_ + ret_reg];
        if (call_stack_.empty()) {
          return value;
        }
        auto frame = std::move(call_stack_.back());
        call_stack_.pop_back();
        frame_base_ = frame.frame_base;
        register_stack_[frame_base_ + frame.dst_register] = value;
        block_index = frame.return_block_index;
        instr_index_ = frame.return_instr_index;
        break;
      }
      case Bytecode::Instruction::Type::Equal:
        interpret_equal(ast::derived_cast<Bytecode::Instruction::Equal const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::NotEqual:
        interpret_not_equal(
            ast::derived_cast<Bytecode::Instruction::NotEqual const &>(*instr));
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
      case Bytecode::Instruction::Type::StructCreate:
        interpret_struct_create(
            ast::derived_cast<Bytecode::Instruction::StructCreate const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::StructLoad:
        interpret_struct_load(
            ast::derived_cast<Bytecode::Instruction::StructLoad const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::Negate:
        interpret_negate(ast::derived_cast<Bytecode::Instruction::Negate const &>(*instr));
        ++instr_index_;
        break;
      case Bytecode::Instruction::Type::LogicalNot:
        interpret_logical_not(
            ast::derived_cast<Bytecode::Instruction::LogicalNot const &>(*instr));
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
  reg(move.dst) = reg(move.src);
}

void BytecodeInterpreter::interpret_load(const Bytecode::Instruction::Load &load) {
  reg(load.dst) = load.value;
}

void BytecodeInterpreter::interpret_less_than(
    const Bytecode::Instruction::LessThan &less_than) {
  reg(less_than.dst) = reg(less_than.lhs) < reg(less_than.rhs);
}

void BytecodeInterpreter::interpret_greater_than(
    const Bytecode::Instruction::GreaterThan &greater_than) {
  reg(greater_than.dst) = reg(greater_than.lhs) > reg(greater_than.rhs);
}

void BytecodeInterpreter::interpret_less_than_or_equal(
    const Bytecode::Instruction::LessThanOrEqual &less_than_or_equal) {
  reg(less_than_or_equal.dst) =
      reg(less_than_or_equal.lhs) <= reg(less_than_or_equal.rhs);
}

void BytecodeInterpreter::interpret_greater_than_or_equal(
    const Bytecode::Instruction::GreaterThanOrEqual &greater_than_or_equal) {
  reg(greater_than_or_equal.dst) =
      reg(greater_than_or_equal.lhs) >= reg(greater_than_or_equal.rhs);
}

void BytecodeInterpreter::interpret_jump(const Bytecode::Instruction::Jump &jump) {
  block_index = jump.label;
  instr_index_ = 0;
}

void BytecodeInterpreter::interpret_jump_conditional(
    const Bytecode::Instruction::JumpConditional &jump_cond) {
  if (reg(jump_cond.cond)) {
    block_index = jump_cond.label1;
  } else {
    block_index = jump_cond.label2;
  }
  instr_index_ = 0;
}

void BytecodeInterpreter::interpret_call(const Bytecode::Instruction::Call &call,
                                         size_t next_instr_index) {
  assert(call.arg_registers.size() == call.param_registers.size());

  const size_t new_frame_base = frame_base_ + register_count_;
  if (new_frame_base + register_count_ > register_stack_.size()) {
    register_stack_.resize(new_frame_base + register_count_, 0);
  }
  for (size_t i = 0; i < call.param_registers.size(); ++i) {
    register_stack_[new_frame_base + call.param_registers[i]] =
        register_stack_[frame_base_ + call.arg_registers[i]];
  }
  call_stack_.push_back({block_index, next_instr_index, call.dst, frame_base_});
  frame_base_ = new_frame_base;
  block_index = call.label;
  instr_index_ = 0;
}

void BytecodeInterpreter::interpret_equal(const Bytecode::Instruction::Equal &equal) {
  reg(equal.dst) = reg(equal.src1) == reg(equal.src2);
}

void BytecodeInterpreter::interpret_not_equal(
    const Bytecode::Instruction::NotEqual &not_equal) {
  reg(not_equal.dst) = reg(not_equal.src1) != reg(not_equal.src2);
}

void BytecodeInterpreter::interpret_add(const Bytecode::Instruction::Add &add) {
  reg(add.dst) = reg(add.src1) + reg(add.src2);
}

void BytecodeInterpreter::interpret_subtract(
    const Bytecode::Instruction::Subtract &subtract) {
  reg(subtract.dst) = reg(subtract.src1) - reg(subtract.src2);
}

void BytecodeInterpreter::interpret_add_immediate(
    const Bytecode::Instruction::AddImmediate &add_imm) {
  reg(add_imm.dst) = reg(add_imm.src) + add_imm.value;
}

void BytecodeInterpreter::interpret_multiply(
    const Bytecode::Instruction::Multiply &multiply) {
  reg(multiply.dst) = reg(multiply.src1) * reg(multiply.src2);
}

void BytecodeInterpreter::interpret_divide(const Bytecode::Instruction::Divide &divide) {
  reg(divide.dst) = reg(divide.src1) / reg(divide.src2);
}

void BytecodeInterpreter::interpret_modulo(const Bytecode::Instruction::Modulo &modulo) {
  reg(modulo.dst) = reg(modulo.src1) % reg(modulo.src2);
}

void BytecodeInterpreter::interpret_array_create(
    const Bytecode::Instruction::ArrayCreate &array_create) {
  auto array_id = next_heap_id_++;
  auto &array = arrays_[array_id];
  array.reserve(array_create.elements.size());
  for (auto element_reg : array_create.elements) {
    array.push_back(reg(element_reg));
  }
  reg(array_create.dst) = array_id;
}

void BytecodeInterpreter::interpret_array_load(
    const Bytecode::Instruction::ArrayLoad &array_load) {
  auto array_id = reg(array_load.array);
  auto index = reg(array_load.index);
  const auto it = arrays_.find(array_id);
  assert(it != arrays_.end());
  assert(index < it->second.size());
  reg(array_load.dst) = it->second[index];
}

void BytecodeInterpreter::interpret_array_store(
    const Bytecode::Instruction::ArrayStore &array_store) {
  const auto array_id = reg(array_store.array);
  const auto index = reg(array_store.index);
  const auto value = reg(array_store.value);
  const auto it = arrays_.find(array_id);
  assert(it != arrays_.end());
  assert(index < it->second.size());
  it->second[index] = value;
}

void BytecodeInterpreter::interpret_struct_create(
    const Bytecode::Instruction::StructCreate &struct_create) {
  auto struct_id = next_heap_id_++;
  auto &fields = structs_[struct_id];
  for (const auto &field : struct_create.fields) {
    fields[field.first] = reg(field.second);
  }
  reg(struct_create.dst) = struct_id;
}

void BytecodeInterpreter::interpret_struct_load(
    const Bytecode::Instruction::StructLoad &struct_load) {
  const auto struct_id = reg(struct_load.object);
  const auto struct_it = structs_.find(struct_id);
  assert(struct_it != structs_.end());
  const auto field_it = struct_it->second.find(struct_load.field);
  assert(field_it != struct_it->second.end());
  reg(struct_load.dst) = field_it->second;
}

void BytecodeInterpreter::interpret_negate(const Bytecode::Instruction::Negate &negate) {
  reg(negate.dst) = static_cast<Bytecode::Value>(-static_cast<int64_t>(reg(negate.src)));
}

void BytecodeInterpreter::interpret_logical_not(
    const Bytecode::Instruction::LogicalNot &logical_not) {
  reg(logical_not.dst) = reg(logical_not.src) == 0 ? 1 : 0;
}

}  // namespace bytecode
}  // namespace kai
