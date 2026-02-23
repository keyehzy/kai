#include "../catch.hpp"
#include "test_bytecode_cases.h"

TEST_CASE("test_bytecode_array_indexing") {
    auto program = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("values", arr({11, 22, 33})));
      decl_body->append(ret(idx(var("values"), lit(1))));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 22);
}

TEST_CASE("test_bytecode_array_index_assignment_minimal_example") {
    auto program = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("values", arr({7, 8, 9})));
      decl_body->append(idx_assign(var("values"), lit(1), lit(42)));
      decl_body->append(ret(idx(var("values"), lit(1))));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 42);
}
