#include "../catch.hpp"
#include "test_bytecode_cases.h"

TEST_CASE("test_bytecode_function_declaration") {
    auto program = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      auto sum_body = std::make_unique<Ast::Block>();
      sum_body->append(decl("a", lit(4)));
      sum_body->append(decl("b", lit(2)));
      sum_body->append(ret(add(var("a"), var("b"))));
      decl_body->append(
          std::make_unique<Ast::FunctionDeclaration>("sum", std::move(sum_body)));
      decl_body->append(ret(call("sum")));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 6);
}

TEST_CASE("test_bytecode_function_call_recursion") {
    auto program = [] {
      auto program = std::make_unique<Ast::Block>();
      program->append(decl("n", lit(5)));

      auto fact_body = std::make_unique<Ast::Block>();
      auto base_case = std::make_unique<Ast::Block>();
      base_case->append(ret(lit(1)));

      auto recursive_case = std::make_unique<Ast::Block>();
      recursive_case->append(decl("saved_n", var("n")));
      recursive_case->append(assign("n", sub(var("n"), lit(1))));
      recursive_case->append(ret(mul(var("saved_n"), call("fact"))));

      fact_body->append(if_else(lt(var("n"), lit(2)), std::move(base_case),
                                std::move(recursive_case)));
      program->append(
          std::make_unique<Ast::FunctionDeclaration>("fact", std::move(fact_body)));
      program->append(ret(call("fact")));
      return std::move(*program);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 120);
}

TEST_CASE("test_bytecode_function_forward_call") {
    auto program = [] {
      auto program = std::make_unique<Ast::Block>();
      program->append(ret(call("later")));

      auto later_body = std::make_unique<Ast::Block>();
      later_body->append(ret(lit(42)));
      program->append(
          std::make_unique<Ast::FunctionDeclaration>("later", std::move(later_body)));

      return std::move(*program);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(program);
    gen.finalize();
    gen.dump();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 42);
}

TEST_CASE("test_bytecode_bug_missing_function_block") {
    auto program = std::make_unique<Ast::Block>();
    program->append(decl("dummy", lit(0)));

    auto f_body = std::make_unique<Ast::Block>();
    f_body->append(ret(lit(42)));
    program->append(std::make_unique<Ast::FunctionDeclaration>("f", std::move(f_body)));

    program->append(ret(call("f")));

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(*program);
    gen.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 42);
}

TEST_CASE("test_bytecode_bug_implicit_fallthrough") {
    auto program = std::make_unique<Ast::Block>();

    auto f_body = std::make_unique<Ast::Block>();
    f_body->append(decl("a", lit(1)));
    program->append(std::make_unique<Ast::FunctionDeclaration>("f", std::move(f_body)));

    auto g_body = std::make_unique<Ast::Block>();
    g_body->append(ret(lit(50)));
    program->append(std::make_unique<Ast::FunctionDeclaration>("g", std::move(g_body)));

    program->append(ret(call("f")));

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(*program);
    gen.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 0);
}

TEST_CASE("test_bytecode_bug_generator_scope_poisoning") {
    auto program = std::make_unique<Ast::Block>();
    program->append(decl("val", lit(10)));

    auto shadow_body = std::make_unique<Ast::Block>();
    shadow_body->append(decl("val", lit(99)));
    shadow_body->append(ret(var("val")));
    program->append(
        std::make_unique<Ast::FunctionDeclaration>("shadow", std::move(shadow_body)));

    program->append(decl("dummy", call("shadow")));
    program->append(ret(var("val")));

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(*program);
    gen.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 10);
}
