#include "test_optimizer_helpers.h"

// ============================================================
// Integration: Optimizer Preserves Correctness
// ============================================================

TEST_CASE("optimizer_preserves_simple_arithmetic") {
  Ast::Block program;
  program.append(decl("a", lit(17)));
  program.append(decl("b", lit(25)));
  program.append(ret(add(var("a"), var("b"))));

  BytecodeGenerator gen;
  gen.visit_block(program);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.optimize(gen.blocks());

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(gen.blocks()) == 42);
}

TEST_CASE("optimizer_preserves_while_loop_correctness") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), lit(10)), std::move(while_body)));
  body->append(ret(var("i")));

  BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.optimize(gen.blocks());

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(gen.blocks()) == 10);
}

TEST_CASE("optimizer_preserves_fibonacci") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("n",  lit(10)));
  body->append(decl("t1", lit(0)));
  body->append(decl("t2", lit(1)));
  body->append(decl("t3", lit(0)));
  body->append(decl("i",  lit(0)));

  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(assign("t3", add(var("t1"), var("t2"))));
  while_body->append(assign("t1", var("t2")));
  while_body->append(assign("t2", var("t3")));
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), var("n")), std::move(while_body)));
  body->append(ret(var("t1")));

  BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.optimize(gen.blocks());

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(gen.blocks()) == 55);
}

// The optimizer must reduce total instruction count for a while-loop program
// because the generator emits redundant Move instructions around variable
// reads and arithmetic results.
TEST_CASE("optimizer_reduces_instruction_count_for_while_loop") {
  auto make_program = [] {
    auto body = std::make_unique<Ast::Block>();
    body->append(decl("i", lit(0)));
    auto while_body = std::make_unique<Ast::Block>();
    while_body->append(inc("i"));
    body->append(while_loop(lt(var("i"), lit(5)), std::move(while_body)));
    body->append(ret(var("i")));
    return std::move(*body);
  };

  // Unoptimized count.
  auto prog1 = make_program();
  BytecodeGenerator gen1;
  gen1.visit_block(prog1);
  gen1.finalize();
  const size_t before = total_instr_count(gen1.blocks());

  // Optimized count.
  auto prog2 = make_program();
  BytecodeGenerator gen2;
  gen2.visit_block(prog2);
  gen2.finalize();
  BytecodeOptimizer opt;
  opt.optimize(gen2.blocks());
  const size_t after = total_instr_count(gen2.blocks());

  REQUIRE(after < before);
}


