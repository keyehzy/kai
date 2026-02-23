#include "../catch.hpp"
#include "test_bytecode_cases.h"

TEST_CASE("test_bytecode_struct_field_access_minimal") {
    auto program = [] {
      auto body = std::make_unique<Ast::Block>();
      body->append(decl("point", struct_lit({{"x", 40}, {"y", 2}})));
      body->append(ret(add(field_get(var("point"), "x"), field_get(var("point"), "y"))));
      return std::move(*body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 42);
}
