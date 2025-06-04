// Example of code:
//
// fn foo() {
//   int n = 10;
//   int t1 = 0;
//   int t2 = 1;
//   int t3 = 0;
//   for (int i = 0; i < n; i++) {
//     t3 = t1 + t2;
//     t1 = t2;
//     t2 = t3;
//   }
//   return t1;
// }

#include <cstdint>
#include <vector>
#include <memory>

typedef uint64_t u64;

class Bytecode {

  struct Instruction;

  typedef u64 Register;
  typedef u64 Value;
  typedef u64 Label;

  struct BasicBlock;
  std::vector<BasicBlock> blocks;

  u64 register_count = 0;
  Register allocate_register() { return register_count++; }
  Register current_register() { return register_count - 1; }
};

struct Bytecode::Instruction {
  enum class Type {
    Move,
    Load,
    LessThan,
    Jump,
    JumpConditional,
    Return,
    AddImmediate,
  };

  Type type;

  struct Move;
  struct Load;
  struct LessThan;
  struct Jump;
  struct JumpConditional;
  struct Return;
  struct Add;

  virtual ~Instruction() = default;

protected:
  Instruction(Type type) : type(type) {}
};

struct Bytecode::Instruction::Move final : Bytecode::Instruction {
  Move(Register dst, Register src) : Bytecode::Instruction(Type::Move), dst(dst), src(src) {}

  Register dst;
  Register src;
};

struct Bytecode::Instruction::Load final : Bytecode::Instruction {
  Load(Register dst, Value value) : Bytecode::Instruction(Type::Load), dst(dst), value(value) {}

  Register dst;
  Value value;
};

struct Bytecode::Instruction::LessThan final : Bytecode::Instruction {
  LessThan(Register dst, Register lhs, Register rhs) : Bytecode::Instruction(Type::LessThan), dst(dst), lhs(lhs), rhs(rhs) {}

  Register dst;
  Register lhs;
  Register rhs;
};

struct Bytecode::Instruction::Jump final : Bytecode::Instruction {
  Jump(Label label) : Bytecode::Instruction(Type::Jump), label(label) {}

  Label label;
};

struct Bytecode::Instruction::JumpConditional final : Bytecode::Instruction {
  JumpConditional(Register cond, Label then, Label else_) : Bytecode::Instruction(Type::JumpConditional), cond(cond), then(then), else_(else_) {}

  Register cond;
  Label then;
  Label else_;
};

struct Bytecode::Instruction::Return final : Bytecode::Instruction {
  Return(Register reg) : Bytecode::Instruction(Type::Return), reg(reg) {}

  Register reg;
};

struct Bytecode::Instruction::Add final : Bytecode::Instruction {
  Add(Register dst, Register src, Value value) : Bytecode::Instruction(Type::Add), dst(dst), src(src), value(value) {}

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
};



int main() {
  return 0;
}
