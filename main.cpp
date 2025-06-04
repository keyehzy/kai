#include <cstdint>
#include <vector>
#include <memory>
#include <cassert>

#include "ast.h"

template <typename Derived_Reference, typename Base>
Derived_Reference derived_cast(Base &base) {
  using Derived = std::remove_reference_t<Derived_Reference>;
  static_assert(std::is_base_of_v<Base, Derived>);
  static_assert(!std::is_base_of_v<Derived, Base>);
  return static_cast<Derived_Reference>(base);
}

namespace kai {
namespace bytecode {

using namespace ast;

typedef uint64_t u64;

struct Bytecode {
  struct Instruction;

  typedef u64 Register;
  typedef u64 Value;
  typedef u64 Label;

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
    AddImmediate,
  };

  Type type_;

  struct Move;
  struct Load;
  struct LessThan;
  struct Jump;
  struct JumpConditional;
  struct Return;
  struct Add;
  struct AddImmediate;

  virtual ~Instruction() = default;

  virtual void dump() const = 0;

  Type type() const { return type_; }

protected:
  Instruction(Type type) : type_(type) {}
};

struct Bytecode::Instruction::Move final : Bytecode::Instruction {
  Move(Register dst, Register src) : Bytecode::Instruction(Type::Move), dst(dst), src(src) {}

  void dump() const override {
    std::printf("Move r%zu, r%zu", dst, src);
  }

  Register dst;
  Register src;
};

struct Bytecode::Instruction::Load final : Bytecode::Instruction {
  Load(Register dst, Value value) : Bytecode::Instruction(Type::Load), dst(dst), value(value) {}

  void dump() const override {
    std::printf("Load r%zu, %zu", dst, value);
  }

  Register dst;
  Value value;
};

struct Bytecode::Instruction::LessThan final : Bytecode::Instruction {
  LessThan(Register dst, Register lhs, Register rhs) : Bytecode::Instruction(Type::LessThan), dst(dst), lhs(lhs), rhs(rhs) {}

  void dump() const override {
    std::printf("LessThan r%zu, r%zu, r%zu", dst, lhs, rhs);
  }

  Register dst;
  Register lhs;
  Register rhs;
};

struct Bytecode::Instruction::Jump final : Bytecode::Instruction {
  Jump(Label label) : Bytecode::Instruction(Type::Jump), label(label) {}

  void dump() const override {
    std::printf("Jump @%zu", label);
  }

  Label label;
};

struct Bytecode::Instruction::JumpConditional final : Bytecode::Instruction {
  JumpConditional(Register cond, Label label1, Label label2) : Bytecode::Instruction(Type::JumpConditional), cond(cond), label1(label1), label2(label2) {}

  void dump() const override {
    std::printf("JumpConditional r%zu, @%zu, @%zu", cond, label1, label2);
  }

  Register cond;
  Label label1;
  Label label2;
};

struct Bytecode::Instruction::Return final : Bytecode::Instruction {
  Return(Register reg) : Bytecode::Instruction(Type::Return), reg(reg) {}

  void dump() const override {
    std::printf("Return r%zu", reg);
  }

  Register reg;
};

struct Bytecode::Instruction::Add final : Bytecode::Instruction {
  Add(Register dst, Register src1, Register src2) : Bytecode::Instruction(Type::Add), dst(dst), src1(src1), src2(src2) {}

  void dump() const override {
    std::printf("Add r%zu, r%zu, r%zu", dst, src1, src2);
  }

  Register dst;
  Register src1;
  Register src2;
};

struct Bytecode::Instruction::AddImmediate final : Bytecode::Instruction {
  AddImmediate(Register dst, Register src, Value value) : Bytecode::Instruction(Type::AddImmediate), dst(dst), src(src), value(value) {}

  void dump() const override {
    std::printf("Add r%zu, r%zu, %zu", dst, src, value);
  }

  Register dst;
  Register src;
  Value value;
};

struct Bytecode::BasicBlock {
  std::vector<std::unique_ptr<Instruction>> instructions;

  template <typename T, typename... Args>
  T& append(Args&& ...args) {
    instructions.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    return static_cast<T&>(*instructions.back());
  }

  void dump() const {
    for (const auto& instr : instructions) {
      std::printf("  ");
      instr->dump();
      std::printf("\n");
    }
  }
};

struct Bytecode::RegisterAllocator {
  u64 register_count = 0;

  u64 count() const { return register_count; }
  Register allocate() { return register_count++; }
  Register current() { return register_count - 1; }
};

class BytecodeGenerator {
public:
  void visit(const Ast& ast) {
    switch(ast.type) {
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
    case Ast::Type::Assignment:
      visit_assignment(ast_cast<Ast::Assignment const &>(ast));
      break;
    default:
      assert(false);
    }
  }

  void visit_block(Ast::Block const& block) {
    blocks_.emplace_back();
    for (const auto& child : block.children) {
      visit(*child);
    }
  }

  void finalize() {
    for (size_t i = 0; i < blocks_.size(); ++i) {
      auto& block = blocks_[i];
      if (!block.instructions.empty()) {
        auto last_type = block.instructions.back()->type();
        if (last_type != Bytecode::Instruction::Type::Jump &&
            last_type != Bytecode::Instruction::Type::JumpConditional &&
            last_type != Bytecode::Instruction::Type::Return) {
          block.append<Bytecode::Instruction::Jump>(i + 1);
        }
      }
    }
  }

  void dump() const {
    for (size_t i = 0; i < blocks_.size(); ++i) {
      std::printf("%zu:\n", i);
      blocks_[i].dump();
    }
  }

  const std::vector<Bytecode::BasicBlock>& blocks() const { return blocks_; }

private:
  Bytecode::BasicBlock& current_block() {
    if (blocks_.empty()) {
      blocks_.emplace_back();
    }
    return blocks_.back();
  }

  Bytecode::Label current_label() {
    assert(blocks_.size() > 0);
    return blocks_.size() - 1;
  }

  void visit_variable(Ast::Variable const& var) {
    current_block().append<Bytecode::Instruction::Move>(reg_alloc_.allocate(), vars_[var.name]);
  }

  void visit_literal(Ast::Literal const& literal) {
    current_block().append<Bytecode::Instruction::Load>(reg_alloc_.allocate(), literal.value);
  }

  void visit_variable_declaration(Ast::VariableDeclaration const& var_decl) {
    visit(*var_decl.initializer);
    auto reg_id = reg_alloc_.current();
    auto dst_reg = reg_alloc_.allocate();
    current_block().append<Bytecode::Instruction::Move>(dst_reg, reg_id);
    vars_[var_decl.name] = dst_reg;
  }

  void visit_less_than(Ast::LessThan const& less_than) {
    visit(*less_than.left);
    auto left_reg = reg_alloc_.current();
    visit(*less_than.right);
    auto right_reg = reg_alloc_.current();
    current_block().append<Bytecode::Instruction::LessThan>(reg_alloc_.allocate(), left_reg, right_reg);
  }

  void visit_increment(Ast::Increment const& increment) {
    visit(*increment.variable);
    auto reg_id = reg_alloc_.current();
    auto dst_reg = reg_alloc_.allocate();
    current_block().append<Bytecode::Instruction::AddImmediate>(dst_reg, reg_id, 1);
    current_block().append<Bytecode::Instruction::Move>(vars_[increment.variable->name], dst_reg);
  }

  void visit_if_else(Ast::IfElse const& ifelse) {
    blocks_.emplace_back();
    visit(*ifelse.condition);
    auto reg_id = reg_alloc_.current();
    auto& jump_conditional = current_block().append<Bytecode::Instruction::JumpConditional>(reg_id, -1, -1);

    visit_block(*ifelse.body);
    auto label_after_body = current_label();
    jump_conditional.label1 = label_after_body;
    jump_conditional.label2 = label_after_body + 1;
    auto& jump_to_end = current_block().append<Bytecode::Instruction::Jump>(-1);

    visit_block(*ifelse.else_body);
    auto label_after_else = current_label();
    jump_to_end.label = label_after_else + 1;
    blocks_.emplace_back();
  }

  void visit_while(Ast::While const& while_) {
    blocks_.emplace_back();
    visit(*while_.condition);
    auto reg_id = reg_alloc_.current();
    auto block_label = current_label();
    auto& jump_conditional = current_block().append<Bytecode::Instruction::JumpConditional>(reg_id, -1, -1);

    visit_block(*while_.body);
    auto label_after_body = current_label();
    jump_conditional.label1 = label_after_body;
    jump_conditional.label2 = label_after_body + 1;
    current_block().append<Bytecode::Instruction::Jump>(block_label);
    blocks_.emplace_back();
  }

  void visit_return(Ast::Return const& return_) {
    visit(*return_.value);
    current_block().append<Bytecode::Instruction::Return>(reg_alloc_.current());
  }

  void visit_add(Ast::Add const& add) {
    visit(*add.left);
    auto reg_left = reg_alloc_.current();
    visit(*add.right);
    auto reg_right = reg_alloc_.current();
    current_block().append<Bytecode::Instruction::Add>(reg_alloc_.allocate(), reg_left, reg_right);
  }

  void visit_assignment(Ast::Assignment const& assignment) {
    visit(*assignment.value);
    current_block().append<Bytecode::Instruction::Move>(vars_[assignment.name], reg_alloc_.current());
  }


  std::unordered_map<std::string, Bytecode::Register> vars_;
  std::vector<Bytecode::BasicBlock> blocks_;
  Bytecode::RegisterAllocator reg_alloc_;
};

class BytecodeInterpreter {
public:
  Bytecode::Value interpret(const std::vector<Bytecode::BasicBlock>& blocks) {
    for(;;) {
      const auto& block = blocks[block_index];
      for (const auto& instr : block.instructions) {
        switch(instr->type()) {
        case Bytecode::Instruction::Type::Move:
          interpret_move(derived_cast<Bytecode::Instruction::Move const&>(*instr));
          break;
        case Bytecode::Instruction::Type::Load:
          interpret_load(derived_cast<Bytecode::Instruction::Load const&>(*instr));
          break;
        case Bytecode::Instruction::Type::LessThan:
          interpret_less_than(derived_cast<Bytecode::Instruction::LessThan const&>(*instr));
          break;
        case Bytecode::Instruction::Type::Jump:
          interpret_jump(derived_cast<Bytecode::Instruction::Jump const&>(*instr));
          break;
        case Bytecode::Instruction::Type::JumpConditional:
          interpret_jump_conditional(derived_cast<Bytecode::Instruction::JumpConditional const&>(*instr));
          break;
        case Bytecode::Instruction::Type::Return:
          return registers_[derived_cast<Bytecode::Instruction::Return const&>(*instr).reg];
        case Bytecode::Instruction::Type::Add:
          interpret_add(derived_cast<Bytecode::Instruction::Add const&>(*instr));
          break;
        case Bytecode::Instruction::Type::AddImmediate:
          interpret_add_immediate(derived_cast<Bytecode::Instruction::AddImmediate const&>(*instr));
          break;
        default:
          assert(0);
          break;
        }
      }
    }
    assert(0);
    return Bytecode::Value(-1);
  }

private:
  void interpret_move(Bytecode::Instruction::Move const& move) {
    registers_[move.dst] = registers_[move.src];
  }

  void interpret_load(Bytecode::Instruction::Load const& load) {
    registers_[load.dst] = load.value;
  }

  void interpret_less_than(Bytecode::Instruction::LessThan const& less_than) {
    registers_[less_than.dst] = registers_[less_than.lhs] < registers_[less_than.rhs];
  }

  void interpret_jump(Bytecode::Instruction::Jump const& jump) {
    block_index = jump.label;
  }

  void interpret_jump_conditional(Bytecode::Instruction::JumpConditional const& jump_cond) {
    if (registers_[jump_cond.cond]) {
      block_index = jump_cond.label1;
    } else {
      block_index = jump_cond.label2;
    }
  }

  void interpret_add(Bytecode::Instruction::Add const& add) {
    registers_[add.dst] = registers_[add.src1] + registers_[add.src2];
  }

  void interpret_add_immediate(Bytecode::Instruction::AddImmediate const& add_imm) {
    registers_[add_imm.dst] = registers_[add_imm.src] + add_imm.value;
  }

  u64 block_index = 0;
  std::unordered_map<Bytecode::Register, Bytecode::Value> registers_;
};

} // namespace bytecode
} // namepsace kai

using namespace kai::ast;

Ast::Block fibonacci_example() {
  // begin function declaration
  auto decl_body = std::make_unique<Ast::Block>();

  // declare n, i, t1, t2, t3
  decl_body->append<Ast::VariableDeclaration>("n", std::make_unique<Ast::Literal>(10));
  decl_body->append<Ast::VariableDeclaration>("t1", std::make_unique<Ast::Literal>(0));
  decl_body->append<Ast::VariableDeclaration>("t2", std::make_unique<Ast::Literal>(1));
  decl_body->append<Ast::VariableDeclaration>("t3", std::make_unique<Ast::Literal>(0));

  // for (int i = 0; i < n; i++) {
  //   t3 = t1 + t2;
  //   t1 = t2;
  //   t2 = t3;
  // }
  decl_body->append<Ast::VariableDeclaration>("i", std::make_unique<Ast::Literal>(0));
  auto less_than =
      std::make_unique<Ast::LessThan>(std::make_unique<Ast::Variable>("i"), std::make_unique<Ast::Variable>("n"));
  auto block = std::make_unique<Ast::Block>();
  block->append<Ast::Assignment>(
      "t3", std::make_unique<Ast::Add>(std::make_unique<Ast::Variable>("t1"), std::make_unique<Ast::Variable>("t2")));
  block->append<Ast::Assignment>("t1", std::make_unique<Ast::Variable>("t2"));
  block->append<Ast::Assignment>("t2", std::make_unique<Ast::Variable>("t3"));
  block->append<Ast::Increment>(std::make_unique<Ast::Variable>("i"));
  auto while_loop = std::make_unique<Ast::While>(std::move(less_than), std::move(block));
  decl_body->children.push_back(std::move(while_loop));
  // end for loop

  decl_body->append<Ast::Return>(std::make_unique<Ast::Variable>("t1"));
  // end function declaration
  return std::move(*decl_body);
}

int main() {
  auto fib = fibonacci_example();
  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(fib);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  std::cout << interp.interpret(gen.blocks()) << "\n";

  return 0;
}
