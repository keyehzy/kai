#include "../catch.hpp"
#include "../bytecode.h"
#include "../optimizer.h"
#include "test_bytecode_cases.h"

using namespace kai::bytecode;
using Type = Bytecode::Instruction::Type;

// Count total instructions across all blocks.
static size_t total_instr_count(const std::vector<Bytecode::BasicBlock> &blocks) {
  size_t n = 0;
  for (const auto &b : blocks) n += b.instructions.size();
  return n;
}

// ============================================================
// Pass 1: Copy Propagation
// ============================================================

// A two-step Move chain (r0→r1→r2) should be fully resolved so that
// both intermediate moves are eventually dead and removed by DCE.
TEST_CASE("copy_prop_move_chain_collapsed") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 42);
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);   // r1 = r0
  blocks[0].append<Bytecode::Instruction::Move>(2, 1);   // r2 = r1  →  r0 after resolve
  blocks[0].append<Bytecode::Instruction::Return>(2);    // return r2  →  r0 after resolve

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // Copy prop: Move r2 becomes Move r2, r0; Return becomes Return r0.
  // DCE: r1 and r2 are never read after substitution → both Moves removed.
  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[1]->type() == Type::Return);
  const auto &ret =
      static_cast<const Bytecode::Instruction::Return &>(*blocks[0].instructions[1]);
  REQUIRE(ret.reg == 0);
}

// Move r0, r0 is trivially useless and must be removed during copy propagation.
TEST_CASE("copy_prop_trivial_self_move_removed") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 99);
  blocks[0].append<Bytecode::Instruction::Move>(0, 0);  // dst == src
  blocks[0].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[1]->type() == Type::Return);
}

// When a binary op reads a register that is a known copy alias, the source
// operand is substituted with the canonical register.
TEST_CASE("copy_prop_substitutes_source_in_binary_op") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 40);
  blocks[0].append<Bytecode::Instruction::Load>(1, 2);
  blocks[0].append<Bytecode::Instruction::Move>(2, 0);        // r2 = r0 (alias)
  blocks[0].append<Bytecode::Instruction::Add>(3, 2, 1);      // r3 = r2 + r1
  blocks[0].append<Bytecode::Instruction::Return>(3);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // After copy prop: Add r3, r0, r1 (r2 substituted with r0).
  // After DCE: Move r2 has no live readers → removed.
  // 4 instructions: Load r0, Load r1, Add r3, Return r3.
  REQUIRE(blocks[0].instructions.size() == 4);
  REQUIRE(blocks[0].instructions[2]->type() == Type::Add);
  const auto &add =
      static_cast<const Bytecode::Instruction::Add &>(*blocks[0].instructions[2]);
  REQUIRE(add.src1 == 0);  // was r2, now r0
  REQUIRE(add.src2 == 1);
  REQUIRE(add.dst  == 3);
}

// AddImmediate (used by i++) has its source register substituted through a
// preceding Move, mirroring the core loop-increment optimization.
TEST_CASE("copy_prop_substitutes_source_in_add_immediate") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 0);              // r0 = 0 (variable i)
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);              // r1 = r0
  blocks[0].append<Bytecode::Instruction::AddImmediate>(2, 1, 1);   // r2 = r1 + 1
  blocks[0].append<Bytecode::Instruction::Move>(0, 2);              // r0 = r2 (write-back)
  blocks[0].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // After copy prop:
  //   Move r1, r0: copies={r1:r0}
  //   AddImmediate r2, r1, 1 → src=resolve(r1)=r0 → AddImmediate r2, r0, 1
  //   Move r0, r2: copies={r0:r2}; Return r0 → Return r2
  // After DCE: Move r1 is dead (r1 not in live) → removed.
  // Remaining: Load r0 / AddImmediate r2,r0,1 / Move r0,r2 / Return r2 = 4 instructions.
  REQUIRE(blocks[0].instructions.size() == 4);
  REQUIRE(blocks[0].instructions[1]->type() == Type::AddImmediate);
  const auto &ai =
      static_cast<const Bytecode::Instruction::AddImmediate &>(*blocks[0].instructions[1]);
  REQUIRE(ai.src   == 0);  // was r1, now r0
  REQUIRE(ai.value == 1);
}

// Re-assigning the canonical source register must invalidate any copies that
// depended on it, preventing stale substitution in later instructions.
TEST_CASE("copy_prop_copy_invalidated_when_source_overwritten") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 10);  // r0 = 10
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);   // r1 = r0; copies={r1:r0}
  blocks[0].append<Bytecode::Instruction::Load>(0, 20);  // r0 = 20; invalidates r1→r0
  blocks[0].append<Bytecode::Instruction::Add>(2, 1, 0); // r2 = r1 + r0
  blocks[0].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // After Load r0, 20 the copy r1→r0 is gone. Add must NOT substitute r1 with r0.
  // All 5 instructions live.
  REQUIRE(blocks[0].instructions.size() == 5);
  const auto &add =
      static_cast<const Bytecode::Instruction::Add &>(*blocks[0].instructions[3]);
  REQUIRE(add.src1 == 1);  // r1, NOT replaced with r0
  REQUIRE(add.src2 == 0);
}

// The condition register of a JumpConditional is resolved through the copy map.
TEST_CASE("copy_prop_resolves_jump_conditional_cond") {
  std::vector<Bytecode::BasicBlock> blocks(3);
  blocks[0].append<Bytecode::Instruction::Load>(0, 1);
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);               // r1 = r0
  blocks[0].append<Bytecode::Instruction::JumpConditional>(1, 1, 2); // cond = r1

  blocks[1].append<Bytecode::Instruction::Load>(2, 42);
  blocks[1].append<Bytecode::Instruction::Return>(2);

  blocks[2].append<Bytecode::Instruction::Load>(3, 0);
  blocks[2].append<Bytecode::Instruction::Return>(3);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // JumpConditional cond should be r0 after substitution; Move r1 then dead.
  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[1]->type() == Type::JumpConditional);
  const auto &jc = static_cast<const Bytecode::Instruction::JumpConditional &>(
      *blocks[0].instructions[1]);
  REQUIRE(jc.cond == 0);
}

// The register operand of Return is resolved through the copy map.
TEST_CASE("copy_prop_resolves_return_register") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 7);
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);   // r1 = r0
  blocks[0].append<Bytecode::Instruction::Return>(1);    // return r1

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[1]->type() == Type::Return);
  const auto &ret =
      static_cast<const Bytecode::Instruction::Return &>(*blocks[0].instructions[1]);
  REQUIRE(ret.reg == 0);
}

// ============================================================
// Pass 2: Dead Code Elimination
// ============================================================

// A Load whose destination is never used as a source operand anywhere is removed.
TEST_CASE("dce_dead_load_removed") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 999); // r0 — never read
  blocks[0].append<Bytecode::Instruction::Load>(1, 42);
  blocks[0].append<Bytecode::Instruction::Return>(1);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  const auto &load =
      static_cast<const Bytecode::Instruction::Load &>(*blocks[0].instructions[0]);
  REQUIRE(load.dst == 1);
  REQUIRE(load.value == 42);
}

// An Add whose result is never read is dead and should be eliminated.
TEST_CASE("dce_dead_binary_op_removed") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 3);
  blocks[0].append<Bytecode::Instruction::Load>(1, 4);
  blocks[0].append<Bytecode::Instruction::Add>(2, 0, 1);  // r2 — never read
  blocks[0].append<Bytecode::Instruction::Load>(3, 10);
  blocks[0].append<Bytecode::Instruction::Return>(3);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // r2 not in live → Add removed. r0 and r1 remain live (DCE collects sources
  // before removal, so their Loads are kept in this single pass).
  REQUIRE(blocks[0].instructions.size() == 4);
  for (const auto &instr : blocks[0].instructions) {
    REQUIRE(instr->type() != Type::Add);
  }
}

// A Move whose destination is never used as a source is dead and removed.
TEST_CASE("dce_dead_move_removed") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 5);
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);   // r1 — never read
  blocks[0].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 2);
  for (const auto &instr : blocks[0].instructions) {
    REQUIRE(instr->type() != Type::Move);
  }
}

// A Load whose destination is read must not be removed.
TEST_CASE("dce_live_load_kept") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 42);
  blocks[0].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
}

// Jump has no destination register; it is never subject to removal.
TEST_CASE("dce_jump_never_removed") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Jump>(1);
  blocks[1].append<Bytecode::Instruction::Load>(0, 1);
  blocks[1].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 1);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Jump);
}

// JumpConditional has a condition register (src operand) but no dst; it is
// side-effecting (controls flow) and is never removed.
TEST_CASE("dce_jump_conditional_never_removed") {
  std::vector<Bytecode::BasicBlock> blocks(3);
  blocks[0].append<Bytecode::Instruction::Load>(0, 1);
  blocks[0].append<Bytecode::Instruction::JumpConditional>(0, 1, 2);
  blocks[1].append<Bytecode::Instruction::Load>(1, 10);
  blocks[1].append<Bytecode::Instruction::Return>(1);
  blocks[2].append<Bytecode::Instruction::Load>(2, 20);
  blocks[2].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions[1]->type() == Type::JumpConditional);
}

// Return is a terminator; it is never removed.
TEST_CASE("dce_return_never_removed") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 42);
  blocks[0].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.back()->type() == Type::Return);
}

// ArrayStore is a side-effecting store; it is never removed regardless of
// whether its registers appear live elsewhere.
TEST_CASE("dce_array_store_never_removed") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 1);    // array ref
  blocks[0].append<Bytecode::Instruction::Load>(1, 0);    // index
  blocks[0].append<Bytecode::Instruction::Load>(2, 99);   // value
  blocks[0].append<Bytecode::Instruction::ArrayStore>(0, 1, 2);
  blocks[0].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  bool has_array_store = false;
  for (const auto &instr : blocks[0].instructions) {
    if (instr->type() == Type::ArrayStore) has_array_store = true;
  }
  REQUIRE(has_array_store);
}

// An instruction in block 0 whose dst is read in block 1 must not be removed —
// DCE liveness is computed globally across all blocks.
TEST_CASE("dce_cross_block_liveness_respected") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 42);
  blocks[0].append<Bytecode::Instruction::Jump>(1);
  blocks[1].append<Bytecode::Instruction::Return>(0);  // reads r0 from block 0

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
}

// ============================================================
// Integration: Optimizer Preserves Correctness
// ============================================================

TEST_CASE("optimizer_preserves_simple_arithmetic") {
  Ast::Block program;
  program.append(decl("a", lit(17)));
  program.append(decl("b", lit(25)));
  program.append(ret(add(var("a"), var("b"))));

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(program);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.optimize(gen.blocks());

  kai::bytecode::BytecodeInterpreter interp;
  REQUIRE(interp.interpret(gen.blocks()) == 42);
}

TEST_CASE("optimizer_preserves_while_loop_correctness") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), lit(10)), std::move(while_body)));
  body->append(ret(var("i")));

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.optimize(gen.blocks());

  kai::bytecode::BytecodeInterpreter interp;
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

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.optimize(gen.blocks());

  kai::bytecode::BytecodeInterpreter interp;
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
  kai::bytecode::BytecodeGenerator gen1;
  gen1.visit_block(prog1);
  gen1.finalize();
  const size_t before = total_instr_count(gen1.blocks());

  // Optimized count.
  auto prog2 = make_program();
  kai::bytecode::BytecodeGenerator gen2;
  gen2.visit_block(prog2);
  gen2.finalize();
  BytecodeOptimizer opt;
  opt.optimize(gen2.blocks());
  const size_t after = total_instr_count(gen2.blocks());

  REQUIRE(after < before);
}

// Running the optimizer a second time on already-optimized code must not change
// the instruction count (idempotency).
TEST_CASE("optimizer_is_idempotent") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), lit(3)), std::move(while_body)));
  body->append(ret(var("i")));

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.optimize(gen.blocks());
  const size_t after_first  = total_instr_count(gen.blocks());
  opt.optimize(gen.blocks());
  const size_t after_second = total_instr_count(gen.blocks());

  REQUIRE(after_second == after_first);
}
