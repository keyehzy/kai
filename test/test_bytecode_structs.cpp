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

TEST_CASE("test_bytecode_struct_field_access_minimal") {
    auto program = [] {
      auto body = std::make_unique<Ast::Block>();
      body->append(decl("point", struct_lit({{"x", 40}, {"y", 2}})));
      body->append(ret(add(field_get(var("point"), "x"), field_get(var("point"), "y"))));
      return std::move(*body);
    }();

    kai::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();

    kai::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 42);
}

TEST_CASE("generator_emits_struct_literal_create_for_all_literal_fields") {
    auto program = [] {
      auto body = std::make_unique<Ast::Block>();
      body->append(decl("point", struct_lit({{"x", 40}, {"y", 2}})));
      body->append(ret(field_get(var("point"), "x")));
      return std::move(*body);
    }();

    kai::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();

    REQUIRE(has_instruction(gen.blocks(), Type::StructLiteralCreate));
    REQUIRE_FALSE(has_instruction(gen.blocks(), Type::StructCreate));

    kai::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 40);
}

TEST_CASE("generator_keeps_struct_create_when_field_is_not_literal") {
    auto program = [] {
      auto body = std::make_unique<Ast::Block>();
      body->append(decl("x", lit(9)));
      std::vector<std::pair<std::string, std::unique_ptr<Ast>>> fields;
      fields.emplace_back("x", var("x"));
      fields.emplace_back("y", lit(1));
      body->append(decl("point", std::make_unique<Ast::StructLiteral>(std::move(fields))));
      body->append(ret(field_get(var("point"), "x")));
      return std::move(*body);
    }();

    kai::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();

    REQUIRE(has_instruction(gen.blocks(), Type::StructCreate));
    REQUIRE_FALSE(has_instruction(gen.blocks(), Type::StructLiteralCreate));

    kai::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 9);
}
