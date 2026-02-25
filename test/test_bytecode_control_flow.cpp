#include "catch.hpp"
#include "test_bytecode_cases.h"

TEST_CASE("test_bytecode_if_else") {
    auto max = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("a", lit(4)));
      decl_body->append(decl("b", lit(7)));
      decl_body->append(decl("max", lit(0)));

      auto if_body = std::make_unique<Ast::Block>();
      if_body->append(assign("max", var("a")));

      auto else_body = std::make_unique<Ast::Block>();
      else_body->append(assign("max", var("b")));

      decl_body->append(
          if_else(lt(var("b"), var("a")), std::move(if_body), std::move(else_body)));
      decl_body->append(ret(var("max")));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(max);
    gen.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 7);
}

TEST_CASE("test_bytecode_top_level_block_example") {
    Ast::Block program;
    program.append(ret(lit(42)));

    kai::bytecode::BytecodeGenerator gen;
    gen.visit(program);
    gen.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 42);
}

TEST_CASE("test_bytecode_nested_if_inside_while") {
    auto program = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("n", lit(3)));
      decl_body->append(decl("i", lit(0)));
      decl_body->append(decl("x", lit(0)));

      auto while_body = std::make_unique<Ast::Block>();
      auto if_body = std::make_unique<Ast::Block>();
      if_body->append(assign("x", add(var("x"), lit(10))));
      auto else_body = std::make_unique<Ast::Block>();
      else_body->append(assign("x", add(var("x"), lit(1))));
      while_body->append(
          if_else(lt(var("i"), lit(1)), std::move(if_body), std::move(else_body)));
      while_body->append(inc("i"));

      decl_body->append(while_loop(lt(var("i"), var("n")), std::move(while_body)));
      decl_body->append(ret(var("x")));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 12);
}

TEST_CASE("test_bytecode_interpreter_reuse_across_programs") {
    auto with_loop = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      decl_body->append(decl("i", lit(0)));

      auto while_body = std::make_unique<Ast::Block>();
      while_body->append(inc("i"));
      decl_body->append(while_loop(lt(var("i"), lit(1)), std::move(while_body)));
      decl_body->append(ret(var("i")));
      return std::move(*decl_body);
    }();

    Ast::Block simple;
    simple.append(ret(lit(7)));

    kai::bytecode::BytecodeGenerator gen_with_loop;
    gen_with_loop.visit_block(with_loop);
    gen_with_loop.finalize();

    kai::bytecode::BytecodeGenerator gen_simple;
    gen_simple.visit(simple);
    gen_simple.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen_with_loop.blocks()) == 1);
    REQUIRE(interp.interpret(gen_simple.blocks()) == 7);
}

TEST_CASE("test_bytecode_count_to_ten_using_while_loop") {
    auto program = [] {
      auto body = std::make_unique<Ast::Block>();
      body->append(decl("i", lit(0)));

      auto while_body = std::make_unique<Ast::Block>();
      while_body->append(inc("i"));

      body->append(while_loop(lt(var("i"), lit(10)), std::move(while_body)));
      body->append(ret(var("i")));
      return std::move(*body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 10);
}

TEST_CASE("test_bytecode_count_down_from_ten_to_one_using_while_loop") {
    auto program = [] {
      auto body = std::make_unique<Ast::Block>();
      body->append(decl("i", lit(10)));

      auto while_body = std::make_unique<Ast::Block>();
      while_body->append(assign("i", sub(var("i"), lit(1))));

      body->append(while_loop(gt(var("i"), lit(1)), std::move(while_body)));
      body->append(ret(var("i")));
      return std::move(*body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 1);
}
