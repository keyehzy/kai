#include "../catch.hpp"
#include "../optimizer.h"
#include "test_bytecode_cases.h"

#include <functional>
#include <vector>

using kai::bytecode::Bytecode;
using Type = Bytecode::Instruction::Type;

namespace {

kai::bytecode::BytecodeGenerator compile_return_expr(std::unique_ptr<Ast> expr) {
  Ast::Block program;
  program.append(ret(std::move(expr)));
  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(program);
  gen.finalize();
  return gen;
}

bool has_load_immediate(const std::vector<Bytecode::BasicBlock> &blocks, Bytecode::Value value) {
  for (const auto &block : blocks) {
    for (const auto &instr_ptr : block.instructions) {
      if (instr_ptr->type() != Type::Load) {
        continue;
      }
      const auto &load = static_cast<const Bytecode::Instruction::Load &>(*instr_ptr);
      if (load.value == value) {
        return true;
      }
    }
  }
  return false;
}

size_t count_type(const std::vector<Bytecode::BasicBlock> &blocks, Type type) {
  size_t count = 0;
  for (const auto &block : blocks) {
    for (const auto &instr_ptr : block.instructions) {
      if (instr_ptr->type() == type) {
        ++count;
      }
    }
  }
  return count;
}

}  // namespace

TEST_CASE("bytecode_emits_immediate_binary_and_comparison_variants") {
  struct Case {
    const char *name;
    std::function<std::unique_ptr<Ast>()> expr;
    Type immediate_type;
    kai::ast::Value expected_value;
  };

  const std::vector<Case> cases = {
      {"add immediate", [] { return add(lit(10), lit(3)); }, Type::AddImmediate, 13},
      {"subtract immediate", [] { return sub(lit(10), lit(3)); }, Type::SubtractImmediate, 7},
      {"multiply immediate", [] { return mul(lit(10), lit(3)); }, Type::MultiplyImmediate, 30},
      {"divide immediate", [] { return div(lit(10), lit(3)); }, Type::DivideImmediate, 3},
      {"modulo immediate", [] { return mod(lit(10), lit(3)); }, Type::ModuloImmediate, 1},
      {"less-than immediate", [] { return lt(lit(2), lit(3)); }, Type::LessThanImmediate, 1},
      {"greater-than immediate", [] { return gt(lit(5), lit(3)); }, Type::GreaterThanImmediate,
       1},
      {"less-than-or-equal immediate",
       [] { return std::make_unique<Ast::LessThanOrEqual>(lit(3), lit(3)); },
       Type::LessThanOrEqualImmediate, 1},
      {"greater-than-or-equal immediate",
       [] { return std::make_unique<Ast::GreaterThanOrEqual>(lit(4), lit(3)); },
       Type::GreaterThanOrEqualImmediate, 1},
      {"equal immediate", [] { return std::make_unique<Ast::Equal>(lit(4), lit(4)); },
       Type::EqualImmediate, 1},
      {"not-equal immediate", [] { return std::make_unique<Ast::NotEqual>(lit(4), lit(3)); },
       Type::NotEqualImmediate, 1},
  };

  for (const auto &tc : cases) {
    SECTION(tc.name) {
      auto gen = compile_return_expr(tc.expr());
      const auto &instrs = gen.blocks()[0].instructions;
      REQUIRE(instrs.size() == 3);
      REQUIRE(instrs[1]->type() == tc.immediate_type);

      kai::bytecode::BytecodeInterpreter interp;
      REQUIRE(interp.interpret(gen.blocks()) == tc.expected_value);
    }
  }
}

TEST_CASE("bytecode_uses_subtract_immediate_for_repeated_n_minus_constants") {
  Ast::Block program;
  program.append(decl("n", lit(8)));
  program.append(ret(add(sub(var("n"), lit(1)), sub(var("n"), lit(2)))));

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(program);
  gen.finalize();

  kai::bytecode::BytecodeOptimizer opt;
  opt.optimize(gen.blocks());

  REQUIRE(count_type(gen.blocks(), Type::SubtractImmediate) == 2);
  REQUIRE(!has_load_immediate(gen.blocks(), 1));
  REQUIRE(!has_load_immediate(gen.blocks(), 2));

  kai::bytecode::BytecodeInterpreter interp;
  REQUIRE(interp.interpret(gen.blocks()) == 13);
}
