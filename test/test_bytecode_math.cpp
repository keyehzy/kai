#include "../catch.hpp"
#include "test_bytecode_cases.h"

TEST_CASE("test_bytecode_fibonacci") {
    auto fib = [] {
      auto decl_body = std::make_unique<Ast::Block>();

      decl_body->append(decl("n", lit(10)));
      decl_body->append(decl("t1", lit(0)));
      decl_body->append(decl("t2", lit(1)));
      decl_body->append(decl("t3", lit(0)));

      decl_body->append(decl("i", lit(0)));
      auto block = std::make_unique<Ast::Block>();
      block->append(assign("t3", add(var("t1"), var("t2"))));
      block->append(assign("t1", var("t2")));
      block->append(assign("t2", var("t3")));
      block->append(inc("i"));
      decl_body->append(while_loop(lt(var("i"), var("n")), std::move(block)));

      decl_body->append(ret(var("t1")));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(fib);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 55);
}

TEST_CASE("test_bytecode_factorial") {
    auto factorial = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("n", lit(5)));
      decl_body->append(decl("result", lit(1)));
      decl_body->append(decl("i", lit(1)));

      auto block = std::make_unique<Ast::Block>();
      block->append(assign("result", mul(var("result"), var("i"))));
      block->append(inc("i"));
      decl_body->append(
          while_loop(lt(var("i"), add(var("n"), lit(1))), std::move(block)));
      decl_body->append(ret(var("result")));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(factorial);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 120);
}

TEST_CASE("test_bytecode_subtract") {
    auto difference = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("a", lit(20)));
      decl_body->append(decl("b", lit(8)));
      decl_body->append(ret(sub(var("a"), var("b"))));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(difference);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 12);
}

TEST_CASE("test_bytecode_divide") {
    auto quotient = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("a", lit(20)));
      decl_body->append(decl("b", lit(5)));
      decl_body->append(ret(div(var("a"), var("b"))));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(quotient);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 4);
}

TEST_CASE("test_bytecode_modulo") {
    auto remainder = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("a", lit(20)));
      decl_body->append(decl("b", lit(6)));
      decl_body->append(ret(mod(var("a"), var("b"))));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(remainder);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 2);
}

TEST_CASE("test_bytecode_greater_than") {
    auto comparison = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("a", lit(3)));
      decl_body->append(decl("b", lit(2)));
      decl_body->append(ret(gt(var("a"), var("b"))));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(comparison);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 1);
}
