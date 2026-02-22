#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ast.h"

namespace kai {
namespace bytecode {

using u64 = uint64_t;

struct Bytecode {
  struct Instruction;

  using Register = u64;
  using Value = u64;
  using Label = u64;

  struct BasicBlock;
  struct RegisterAllocator;
};

struct Bytecode::Instruction {
  enum class Type {
    Move,
    Load,
    LessThan,
    Jump,
    JumpConditional,
    Return,
    Add,
    Subtract,
    AddImmediate,
    Multiply,
    Divide,
  };

  Type type_;

  struct Move;
  struct Load;
  struct LessThan;
  struct Jump;
  struct JumpConditional;
  struct Return;
  struct Add;
  struct Subtract;
  struct AddImmediate;
  struct Multiply;
  struct Divide;

  virtual ~Instruction() = default;

  virtual void dump() const = 0;
  Type type() const;

 protected:
  explicit Instruction(Type type);
};

struct Bytecode::Instruction::Move final : Bytecode::Instruction {
  Move(Register dst, Register src);
  void dump() const override;

  Register dst;
  Register src;
};

struct Bytecode::Instruction::Load final : Bytecode::Instruction {
  Load(Register dst, Value value);
  void dump() const override;

  Register dst;
  Value value;
};

struct Bytecode::Instruction::LessThan final : Bytecode::Instruction {
  LessThan(Register dst, Register lhs, Register rhs);
  void dump() const override;

  Register dst;
  Register lhs;
  Register rhs;
};

struct Bytecode::Instruction::Jump final : Bytecode::Instruction {
  explicit Jump(Label label);
  void dump() const override;

  Label label;
};

struct Bytecode::Instruction::JumpConditional final : Bytecode::Instruction {
  JumpConditional(Register cond, Label label1, Label label2);
  void dump() const override;

  Register cond;
  Label label1;
  Label label2;
};

struct Bytecode::Instruction::Return final : Bytecode::Instruction {
  explicit Return(Register reg);
  void dump() const override;

  Register reg;
};

struct Bytecode::Instruction::Add final : Bytecode::Instruction {
  Add(Register dst, Register src1, Register src2);
  void dump() const override;

  Register dst;
  Register src1;
  Register src2;
};

struct Bytecode::Instruction::Subtract final : Bytecode::Instruction {
  Subtract(Register dst, Register src1, Register src2);
  void dump() const override;

  Register dst;
  Register src1;
  Register src2;
};

struct Bytecode::Instruction::AddImmediate final : Bytecode::Instruction {
  AddImmediate(Register dst, Register src, Value value);
  void dump() const override;

  Register dst;
  Register src;
  Value value;
};

struct Bytecode::Instruction::Multiply final : Bytecode::Instruction {
  Multiply(Register dst, Register src1, Register src2);
  void dump() const override;

  Register dst;
  Register src1;
  Register src2;
};

struct Bytecode::Instruction::Divide final : Bytecode::Instruction {
  Divide(Register dst, Register src1, Register src2);
  void dump() const override;

  Register dst;
  Register src1;
  Register src2;
};

struct Bytecode::BasicBlock {
  std::vector<std::unique_ptr<Instruction>> instructions;

  template <typename T, typename... Args>
  T &append(Args &&...args) {
    instructions.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    return static_cast<T &>(*instructions.back());
  }

  void dump() const;
};

struct Bytecode::RegisterAllocator {
  u64 register_count = 0;

  u64 count() const;
  Register allocate();
  Register current();
};

class BytecodeGenerator {
 public:
  void visit(const ast::Ast &ast);
  void visit_block(const ast::Ast::Block &block);
  void finalize();
  void dump() const;
  const std::vector<Bytecode::BasicBlock> &blocks() const;

 private:
  Bytecode::BasicBlock &current_block();
  Bytecode::Label current_label();

  void visit_variable(const ast::Ast::Variable &var);
  void visit_literal(const ast::Ast::Literal &literal);
  void visit_function_declaration(const ast::Ast::FunctionDeclaration &func_decl);
  void visit_variable_declaration(const ast::Ast::VariableDeclaration &var_decl);
  void visit_less_than(const ast::Ast::LessThan &less_than);
  void visit_increment(const ast::Ast::Increment &increment);
  void visit_if_else(const ast::Ast::IfElse &ifelse);
  void visit_while(const ast::Ast::While &while_);
  void visit_return(const ast::Ast::Return &return_);
  void visit_add(const ast::Ast::Add &add);
  void visit_subtract(const ast::Ast::Subtract &subtract);
  void visit_multiply(const ast::Ast::Multiply &multiply);
  void visit_divide(const ast::Ast::Divide &divide);
  void visit_assignment(const ast::Ast::Assignment &assignment);

  std::unordered_map<std::string, Bytecode::Register> vars_;
  std::vector<Bytecode::BasicBlock> blocks_;
  Bytecode::RegisterAllocator reg_alloc_;
};

class BytecodeInterpreter {
 public:
  Bytecode::Value interpret(const std::vector<Bytecode::BasicBlock> &blocks);

 private:
  void interpret_move(const Bytecode::Instruction::Move &move);
  void interpret_load(const Bytecode::Instruction::Load &load);
  void interpret_less_than(const Bytecode::Instruction::LessThan &less_than);
  void interpret_jump(const Bytecode::Instruction::Jump &jump);
  void interpret_jump_conditional(const Bytecode::Instruction::JumpConditional &jump_cond);
  void interpret_add(const Bytecode::Instruction::Add &add);
  void interpret_subtract(const Bytecode::Instruction::Subtract &subtract);
  void interpret_add_immediate(const Bytecode::Instruction::AddImmediate &add_imm);
  void interpret_multiply(const Bytecode::Instruction::Multiply &multiply);
  void interpret_divide(const Bytecode::Instruction::Divide &divide);

  u64 block_index = 0;
  std::unordered_map<Bytecode::Register, Bytecode::Value> registers_;
};

}  // namespace bytecode
}  // namespace kai
