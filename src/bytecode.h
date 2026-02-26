#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ast.h"

namespace kai {

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
    LessThanImmediate,
    GreaterThan,
    GreaterThanImmediate,
    LessThanOrEqual,
    LessThanOrEqualImmediate,
    GreaterThanOrEqual,
    GreaterThanOrEqualImmediate,
    Jump,
    JumpConditional,
    Call,
    TailCall,
    Return,
    Equal,
    EqualImmediate,
    NotEqual,
    NotEqualImmediate,
    Add,
    AddImmediate,
    Subtract,
    SubtractImmediate,
    Multiply,
    MultiplyImmediate,
    Divide,
    DivideImmediate,
    Modulo,
    ModuloImmediate,
    ArrayCreate,
    ArrayLiteralCreate,
    ArrayLoad,
    ArrayLoadImmediate,
    ArrayStore,
    StructCreate,
    StructLiteralCreate,
    StructLoad,
    Negate,
    LogicalNot,
  };

  Type type_;

  struct Move;
  struct Load;
  struct LessThan;
  struct LessThanImmediate;
  struct GreaterThan;
  struct GreaterThanImmediate;
  struct LessThanOrEqual;
  struct LessThanOrEqualImmediate;
  struct GreaterThanOrEqual;
  struct GreaterThanOrEqualImmediate;
  struct Jump;
  struct JumpConditional;
  struct Call;
  struct TailCall;
  struct Return;
  struct Equal;
  struct EqualImmediate;
  struct NotEqual;
  struct NotEqualImmediate;
  struct Add;
  struct AddImmediate;
  struct Subtract;
  struct SubtractImmediate;
  struct Multiply;
  struct MultiplyImmediate;
  struct Divide;
  struct DivideImmediate;
  struct Modulo;
  struct ModuloImmediate;
  struct ArrayCreate;
  struct ArrayLiteralCreate;
  struct ArrayLoad;
  struct ArrayLoadImmediate;
  struct ArrayStore;
  struct StructCreate;
  struct StructLiteralCreate;
  struct StructLoad;
  struct Negate;
  struct LogicalNot;

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

struct Bytecode::Instruction::LessThanImmediate final : Bytecode::Instruction {
  LessThanImmediate(Register dst, Register lhs, Value value);
  void dump() const override;

  Register dst;
  Register lhs;
  Value value;
};

struct Bytecode::Instruction::GreaterThan final : Bytecode::Instruction {
  GreaterThan(Register dst, Register lhs, Register rhs);
  void dump() const override;

  Register dst;
  Register lhs;
  Register rhs;
};

struct Bytecode::Instruction::GreaterThanImmediate final : Bytecode::Instruction {
  GreaterThanImmediate(Register dst, Register lhs, Value value);
  void dump() const override;

  Register dst;
  Register lhs;
  Value value;
};

struct Bytecode::Instruction::LessThanOrEqual final : Bytecode::Instruction {
  LessThanOrEqual(Register dst, Register lhs, Register rhs);
  void dump() const override;

  Register dst;
  Register lhs;
  Register rhs;
};

struct Bytecode::Instruction::LessThanOrEqualImmediate final : Bytecode::Instruction {
  LessThanOrEqualImmediate(Register dst, Register lhs, Value value);
  void dump() const override;

  Register dst;
  Register lhs;
  Value value;
};

struct Bytecode::Instruction::GreaterThanOrEqual final : Bytecode::Instruction {
  GreaterThanOrEqual(Register dst, Register lhs, Register rhs);
  void dump() const override;

  Register dst;
  Register lhs;
  Register rhs;
};

struct Bytecode::Instruction::GreaterThanOrEqualImmediate final : Bytecode::Instruction {
  GreaterThanOrEqualImmediate(Register dst, Register lhs, Value value);
  void dump() const override;

  Register dst;
  Register lhs;
  Value value;
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

struct Bytecode::Instruction::Call final : Bytecode::Instruction {
  Call(Register dst, Label label, std::vector<Register> arg_registers = {},
       std::vector<Register> param_registers = {});
  void dump() const override;

  Register dst;
  Label label;
  std::vector<Register> arg_registers;
  std::vector<Register> param_registers;
};

struct Bytecode::Instruction::TailCall final : Bytecode::Instruction {
  TailCall(Label label, std::vector<Register> arg_registers = {},
           std::vector<Register> param_registers = {});
  void dump() const override;

  Label label;
  std::vector<Register> arg_registers;
  std::vector<Register> param_registers;
};

struct Bytecode::Instruction::Return final : Bytecode::Instruction {
  explicit Return(Register reg);
  void dump() const override;

  Register reg;
};

struct Bytecode::Instruction::Equal final : Bytecode::Instruction {
  Equal(Register dst, Register src1, Register src2);
  void dump() const override;

  Register dst;
  Register src1;
  Register src2;
};

struct Bytecode::Instruction::EqualImmediate final : Bytecode::Instruction {
  EqualImmediate(Register dst, Register src, Value value);
  void dump() const override;

  Register dst;
  Register src;
  Value value;
};

struct Bytecode::Instruction::NotEqual final : Bytecode::Instruction {
  NotEqual(Register dst, Register src1, Register src2);
  void dump() const override;

  Register dst;
  Register src1;
  Register src2;
};

struct Bytecode::Instruction::NotEqualImmediate final : Bytecode::Instruction {
  NotEqualImmediate(Register dst, Register src, Value value);
  void dump() const override;

  Register dst;
  Register src;
  Value value;
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

struct Bytecode::Instruction::SubtractImmediate final : Bytecode::Instruction {
  SubtractImmediate(Register dst, Register src, Value value);
  void dump() const override;

  Register dst;
  Register src;
  Value value;
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

struct Bytecode::Instruction::MultiplyImmediate final : Bytecode::Instruction {
  MultiplyImmediate(Register dst, Register src, Value value);
  void dump() const override;

  Register dst;
  Register src;
  Value value;
};

struct Bytecode::Instruction::Divide final : Bytecode::Instruction {
  Divide(Register dst, Register src1, Register src2);
  void dump() const override;

  Register dst;
  Register src1;
  Register src2;
};

struct Bytecode::Instruction::DivideImmediate final : Bytecode::Instruction {
  DivideImmediate(Register dst, Register src, Value value);
  void dump() const override;

  Register dst;
  Register src;
  Value value;
};

struct Bytecode::Instruction::Modulo final : Bytecode::Instruction {
  Modulo(Register dst, Register src1, Register src2);
  void dump() const override;

  Register dst;
  Register src1;
  Register src2;
};

struct Bytecode::Instruction::ModuloImmediate final : Bytecode::Instruction {
  ModuloImmediate(Register dst, Register src, Value value);
  void dump() const override;

  Register dst;
  Register src;
  Value value;
};

struct Bytecode::Instruction::ArrayCreate final : Bytecode::Instruction {
  ArrayCreate(Register dst, std::vector<Register> elements);
  void dump() const override;

  Register dst;
  std::vector<Register> elements;
};

struct Bytecode::Instruction::ArrayLiteralCreate final : Bytecode::Instruction {
  ArrayLiteralCreate(Register dst, std::vector<Value> elements);
  void dump() const override;

  Register dst;
  std::vector<Value> elements;
};

struct Bytecode::Instruction::ArrayLoad final : Bytecode::Instruction {
  ArrayLoad(Register dst, Register array, Register index);
  void dump() const override;

  Register dst;
  Register array;
  Register index;
};

struct Bytecode::Instruction::ArrayLoadImmediate final : Bytecode::Instruction {
  ArrayLoadImmediate(Register dst, Register array, Value index);
  void dump() const override;

  Register dst;
  Register array;
  Value index;
};

struct Bytecode::Instruction::ArrayStore final : Bytecode::Instruction {
  ArrayStore(Register array, Register index, Register value);
  void dump() const override;

  Register array;
  Register index;
  Register value;
};

struct Bytecode::Instruction::StructCreate final : Bytecode::Instruction {
  StructCreate(Register dst, std::vector<std::pair<std::string, Register>> fields);
  void dump() const override;

  Register dst;
  std::vector<std::pair<std::string, Register>> fields;
};

struct Bytecode::Instruction::StructLiteralCreate final : Bytecode::Instruction {
  StructLiteralCreate(Register dst, std::vector<std::pair<std::string, Value>> fields);
  void dump() const override;

  Register dst;
  std::vector<std::pair<std::string, Value>> fields;
};

struct Bytecode::Instruction::StructLoad final : Bytecode::Instruction {
  StructLoad(Register dst, Register object, std::string field);
  void dump() const override;

  Register dst;
  Register object;
  std::string field;
};

struct Bytecode::Instruction::Negate final : Bytecode::Instruction {
  Negate(Register dst, Register src);
  void dump() const override;

  Register dst;
  Register src;
};

struct Bytecode::Instruction::LogicalNot final : Bytecode::Instruction {
  LogicalNot(Register dst, Register src);
  void dump() const override;

  Register dst;
  Register src;
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
  void visit(const Ast &ast);
  void visit_block(const Ast::Block &block);
  void finalize();
  void dump() const;
  const std::vector<Bytecode::BasicBlock> &blocks() const;
  std::vector<Bytecode::BasicBlock> &blocks();

 private:
  Bytecode::BasicBlock &current_block();
  Bytecode::Label current_label();

  void visit_variable(const Ast::Variable &var);
  void visit_literal(const Ast::Literal &literal);
  void visit_function_declaration(const Ast::FunctionDeclaration &func_decl);
  void visit_variable_declaration(const Ast::VariableDeclaration &var_decl);
  void visit_less_than(const Ast::LessThan &less_than);
  void visit_greater_than(const Ast::GreaterThan &greater_than);
  void visit_less_than_or_equal(const Ast::LessThanOrEqual &less_than_or_equal);
  void visit_greater_than_or_equal(
      const Ast::GreaterThanOrEqual &greater_than_or_equal);
  void visit_increment(const Ast::Increment &increment);
  void visit_if_else(const Ast::IfElse &ifelse);
  void visit_while(const Ast::While &while_);
  void visit_function_call(const Ast::FunctionCall &function_call);
  void visit_return(const Ast::Return &return_);
  void visit_equal(const Ast::Equal &equal);
  void visit_not_equal(const Ast::NotEqual &not_equal);
  void visit_add(const Ast::Add &add);
  void visit_subtract(const Ast::Subtract &subtract);
  void visit_multiply(const Ast::Multiply &multiply);
  void visit_divide(const Ast::Divide &divide);
  void visit_modulo(const Ast::Modulo &modulo);
  void visit_array_literal(const Ast::ArrayLiteral &array_literal);
  void visit_index(const Ast::Index &index);
  void visit_index_assignment(const Ast::IndexAssignment &index_assignment);
  void visit_struct_literal(const Ast::StructLiteral &struct_literal);
  void visit_field_access(const Ast::FieldAccess &field_access);
  void visit_assignment(const Ast::Assignment &assignment);
  void visit_negate(const Ast::Negate &negate);
  void visit_unary_plus(const Ast::UnaryPlus &unary_plus);
  void visit_logical_not(const Ast::LogicalNot &logical_not);

  std::unordered_map<std::string, Bytecode::Register> vars_;
  std::unordered_map<std::string, Bytecode::Label> functions_;
  std::unordered_map<std::string, std::vector<Bytecode::Register>> function_parameters_;
  std::unordered_map<std::string, std::vector<Bytecode::Instruction::Call *>>
      unresolved_calls_;
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
  void interpret_less_than_immediate(
      const Bytecode::Instruction::LessThanImmediate &less_than_imm);
  void interpret_greater_than(const Bytecode::Instruction::GreaterThan &greater_than);
  void interpret_greater_than_immediate(
      const Bytecode::Instruction::GreaterThanImmediate &greater_than_imm);
  void interpret_less_than_or_equal(
      const Bytecode::Instruction::LessThanOrEqual &less_than_or_equal);
  void interpret_less_than_or_equal_immediate(
      const Bytecode::Instruction::LessThanOrEqualImmediate &less_than_or_equal_imm);
  void interpret_greater_than_or_equal(
      const Bytecode::Instruction::GreaterThanOrEqual &greater_than_or_equal);
  void interpret_greater_than_or_equal_immediate(
      const Bytecode::Instruction::GreaterThanOrEqualImmediate &greater_than_or_equal_imm);
  void interpret_jump(const Bytecode::Instruction::Jump &jump);
  void interpret_jump_conditional(const Bytecode::Instruction::JumpConditional &jump_cond);
  void interpret_call(const Bytecode::Instruction::Call &call, size_t next_instr_index);
  void interpret_tail_call(const Bytecode::Instruction::TailCall &tail_call);
  void interpret_equal(const Bytecode::Instruction::Equal &equal);
  void interpret_equal_immediate(const Bytecode::Instruction::EqualImmediate &equal_imm);
  void interpret_not_equal(const Bytecode::Instruction::NotEqual &not_equal);
  void interpret_not_equal_immediate(
      const Bytecode::Instruction::NotEqualImmediate &not_equal_imm);
  void interpret_add(const Bytecode::Instruction::Add &add);
  void interpret_subtract(const Bytecode::Instruction::Subtract &subtract);
  void interpret_add_immediate(const Bytecode::Instruction::AddImmediate &add_imm);
  void interpret_subtract_immediate(
      const Bytecode::Instruction::SubtractImmediate &subtract_imm);
  void interpret_multiply(const Bytecode::Instruction::Multiply &multiply);
  void interpret_multiply_immediate(
      const Bytecode::Instruction::MultiplyImmediate &multiply_imm);
  void interpret_divide(const Bytecode::Instruction::Divide &divide);
  void interpret_divide_immediate(const Bytecode::Instruction::DivideImmediate &divide_imm);
  void interpret_modulo(const Bytecode::Instruction::Modulo &modulo);
  void interpret_modulo_immediate(const Bytecode::Instruction::ModuloImmediate &modulo_imm);
  void interpret_array_create(const Bytecode::Instruction::ArrayCreate &array_create);
  void interpret_array_literal_create(
      const Bytecode::Instruction::ArrayLiteralCreate &array_literal_create);
  void interpret_array_load(const Bytecode::Instruction::ArrayLoad &array_load);
  void interpret_array_load_immediate(
      const Bytecode::Instruction::ArrayLoadImmediate &array_load_immediate);
  void interpret_array_store(const Bytecode::Instruction::ArrayStore &array_store);
  void interpret_struct_create(const Bytecode::Instruction::StructCreate &struct_create);
  void interpret_struct_literal_create(
      const Bytecode::Instruction::StructLiteralCreate &struct_literal_create);
  void interpret_struct_load(const Bytecode::Instruction::StructLoad &struct_load);
  void interpret_negate(const Bytecode::Instruction::Negate &negate);
  void interpret_logical_not(const Bytecode::Instruction::LogicalNot &logical_not);

  Bytecode::Value& reg(Bytecode::Register r) { return register_stack_[frame_base_ + r]; }

  u64 block_index = 0;
  size_t instr_index_ = 0;
  struct CallFrame {
    u64 return_block_index;
    size_t return_instr_index;
    Bytecode::Register dst_register;
    size_t frame_base;
  };
  std::vector<CallFrame> call_stack_;
  std::vector<Bytecode::Value> register_stack_;
  size_t frame_base_ = 0;
  size_t register_count_ = 0;
  std::unordered_map<Bytecode::Value, std::vector<Bytecode::Value>> arrays_;
  std::unordered_map<Bytecode::Value, std::unordered_map<std::string, Bytecode::Value>>
      structs_;
  Bytecode::Value next_heap_id_ = 1;
};

}  // namespace kai
