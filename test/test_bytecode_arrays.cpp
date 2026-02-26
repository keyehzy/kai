#include "catch.hpp"
#include "test_bytecode_cases.h"

using Type = kai::Bytecode::Instruction::Type;

static bool has_instruction(const std::vector<kai::Bytecode::BasicBlock> &blocks, Type type) {
  for (const auto &block : blocks) {
    for (const auto &instr : block.instructions) {
      if (instr->type() == type) {
        return true;
      }
    }
  }
  return false;
}

TEST_CASE("test_bytecode_array_indexing") {
    auto program = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("values", arr({11, 22, 33})));
      decl_body->append(ret(idx(var("values"), lit(1))));
      return std::move(*decl_body);
    }();

    kai::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();

    kai::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 22);
}

TEST_CASE("generator_emits_array_literal_create_for_all_literal_elements") {
    auto program = [] {
      auto body = std::make_unique<Ast::Block>();
      body->append(decl("values", arr({1, 2, 3})));
      body->append(ret(idx(var("values"), lit(2))));
      return std::move(*body);
    }();

    kai::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();

    REQUIRE(has_instruction(gen.blocks(), Type::ArrayLiteralCreate));
    REQUIRE_FALSE(has_instruction(gen.blocks(), Type::ArrayCreate));

    kai::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 3);
}

TEST_CASE("generator_keeps_array_create_when_element_is_not_literal") {
    auto program = [] {
      auto body = std::make_unique<Ast::Block>();
      body->append(decl("x", lit(7)));
      std::vector<std::unique_ptr<Ast>> elements;
      elements.emplace_back(var("x"));
      elements.emplace_back(lit(5));
      body->append(decl("values", std::make_unique<Ast::ArrayLiteral>(std::move(elements))));
      body->append(ret(idx(var("values"), lit(0))));
      return std::move(*body);
    }();

    kai::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();

    REQUIRE(has_instruction(gen.blocks(), Type::ArrayCreate));
    REQUIRE_FALSE(has_instruction(gen.blocks(), Type::ArrayLiteralCreate));

    kai::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 7);
}

TEST_CASE("test_bytecode_array_index_assignment_minimal_example") {
    auto program = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("values", arr({7, 8, 9})));
      decl_body->append(idx_assign(var("values"), lit(1), lit(42)));
      decl_body->append(ret(idx(var("values"), lit(1))));
      return std::move(*decl_body);
    }();

    kai::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();

    kai::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 42);
}
