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
  // 4 instructions: Load r0, Load r1, Add r2, Return r2 after compaction.
  REQUIRE(blocks[0].instructions.size() == 4);
  REQUIRE(blocks[0].instructions[2]->type() == Type::Add);
  const auto &add =
      static_cast<const Bytecode::Instruction::Add &>(*blocks[0].instructions[2]);
  REQUIRE(add.src1 == 0);  // was r2, now r0
  REQUIRE(add.src2 == 1);
  REQUIRE(add.dst  == 2);
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
  REQUIRE(load.dst == 0);
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

// ============================================================
// Pass 0: Loop-Invariant Code Motion (LICM)
// ============================================================

// After optimize(), a Load of the loop bound (value 10) must not remain in
// the condition block (block 1) — it must have been hoisted to block 0.
TEST_CASE("licm_hoists_load_out_of_condition_block") {
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

  // Load(10) must not appear in the condition block or later loop blocks.
  bool load_10_in_condition = false;
  for (const auto &instr_ptr : gen.blocks()[1].instructions) {
    if (instr_ptr->type() == Type::Load) {
      const auto &load =
          static_cast<const Bytecode::Instruction::Load &>(*instr_ptr);
      if (load.value == 10) load_10_in_condition = true;
    }
  }
  REQUIRE_FALSE(load_10_in_condition);

  // Load(10) must have been moved to the pre-header (block 0).
  bool load_10_in_preheader = false;
  for (const auto &instr_ptr : gen.blocks()[0].instructions) {
    if (instr_ptr->type() == Type::Load) {
      const auto &load =
          static_cast<const Bytecode::Instruction::Load &>(*instr_ptr);
      if (load.value == 10) load_10_in_preheader = true;
    }
  }
  REQUIRE(load_10_in_preheader);
}

// The interpreter must still produce the correct result (10) after LICM.
TEST_CASE("licm_correctness_while_loop") {
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

// An invariant Multiply (a * b where a and b are loop-invariant) inside the
// loop body must be hoisted, and the interpreter must give the correct result.
TEST_CASE("licm_hoists_invariant_arithmetic") {
  // Manually build:
  // block 0: r0=2, r1=3, r2=0; Jump @1          (a=2, b=3, i=0)
  // block 1: r4=5; r5=i<5; JumpConditional @2,@3
  // block 2: r3=a*b; r6=i+1; r2=r6; Jump @1
  // block 3: Return r3
  std::vector<Bytecode::BasicBlock> blocks(4);

  blocks[0].append<Bytecode::Instruction::Load>(0, 2);  // r0 = 2 (a)
  blocks[0].append<Bytecode::Instruction::Load>(1, 3);  // r1 = 3 (b)
  blocks[0].append<Bytecode::Instruction::Load>(2, 0);  // r2 = 0 (i)
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Load>(4, 5);         // r4 = 5
  blocks[1].append<Bytecode::Instruction::LessThan>(5, 2, 4);  // r5 = i < 5
  blocks[1].append<Bytecode::Instruction::JumpConditional>(5, 2, 3);

  blocks[2].append<Bytecode::Instruction::Multiply>(3, 0, 1);     // r3 = a*b
  blocks[2].append<Bytecode::Instruction::AddImmediate>(6, 2, 1); // r6 = i+1
  blocks[2].append<Bytecode::Instruction::Move>(2, 6);            // i = r6
  blocks[2].append<Bytecode::Instruction::Jump>(1);

  blocks[3].append<Bytecode::Instruction::Return>(3);  // return a*b

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // Multiply must have been hoisted out of the loop blocks.
  bool multiply_in_loop = false;
  for (size_t i = 1; i <= 2; ++i) {
    for (const auto &instr_ptr : blocks[i].instructions) {
      if (instr_ptr->type() == Type::Multiply) multiply_in_loop = true;
    }
  }
  REQUIRE_FALSE(multiply_in_loop);

  // Interpreter result must be 6 (2 * 3).
  kai::bytecode::BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 6);
}

// LessThan that reads the loop counter (a loop-variant register) must stay
// inside the loop and must not be hoisted.
TEST_CASE("licm_does_not_hoist_variant_instruction") {
  // block 0: r0=0; Jump @1
  // block 1: r2=5 (hoistable); r3=i<5 (NOT hoistable, i is variant); JumpCond @2,@3
  // block 2: r1=i+1; r0=r1; Jump @1
  // block 3: Return r0
  std::vector<Bytecode::BasicBlock> blocks(4);

  blocks[0].append<Bytecode::Instruction::Load>(0, 0);  // r0 = 0 (i)
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Load>(2, 5);         // r2 = 5 -- hoistable
  blocks[1].append<Bytecode::Instruction::LessThan>(3, 0, 2);  // r3 = i<5 -- variant
  blocks[1].append<Bytecode::Instruction::JumpConditional>(3, 2, 3);

  blocks[2].append<Bytecode::Instruction::AddImmediate>(1, 0, 1); // r1 = i+1
  blocks[2].append<Bytecode::Instruction::Move>(0, 1);            // i = r1
  blocks[2].append<Bytecode::Instruction::Jump>(1);

  blocks[3].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // LessThan must remain somewhere in the loop blocks (1 or 2).
  bool less_than_in_loop = false;
  for (size_t i = 1; i <= 2; ++i) {
    for (const auto &instr_ptr : blocks[i].instructions) {
      if (instr_ptr->type() == Type::LessThan) less_than_in_loop = true;
    }
  }
  REQUIRE(less_than_in_loop);
}

// Straight-line code with no loops must survive optimize() without crashing
// and must still produce the correct result.
TEST_CASE("licm_no_crash_without_loop") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("a", lit(3)));
  body->append(decl("b", lit(4)));
  body->append(ret(add(var("a"), var("b"))));

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  REQUIRE_NOTHROW(opt.optimize(gen.blocks()));

  kai::bytecode::BytecodeInterpreter interp;
  REQUIRE(interp.interpret(gen.blocks()) == 7);
}

// The total instruction count across hot blocks (block 1+) must decrease
// after LICM because the loop-bound Load is moved to the pre-header.
TEST_CASE("licm_reduces_instruction_count_in_hot_blocks") {
  auto make_prog = [] {
    auto body = std::make_unique<Ast::Block>();
    body->append(decl("i", lit(0)));
    auto while_body = std::make_unique<Ast::Block>();
    while_body->append(inc("i"));
    body->append(while_loop(lt(var("i"), lit(10)), std::move(while_body)));
    body->append(ret(var("i")));
    return std::move(*body);
  };

  // Unoptimized count inside loop blocks (block 1 onward).
  auto prog1 = make_prog();
  kai::bytecode::BytecodeGenerator gen1;
  gen1.visit_block(prog1);
  gen1.finalize();
  size_t before = 0;
  for (size_t i = 1; i < gen1.blocks().size(); ++i) {
    before += gen1.blocks()[i].instructions.size();
  }

  // Optimized count.
  auto prog2 = make_prog();
  kai::bytecode::BytecodeGenerator gen2;
  gen2.visit_block(prog2);
  gen2.finalize();
  BytecodeOptimizer opt;
  opt.optimize(gen2.blocks());
  size_t after = 0;
  for (size_t i = 1; i < gen2.blocks().size(); ++i) {
    after += gen2.blocks()[i].instructions.size();
  }

  REQUIRE(after < before);
}

// Running optimize() a second time after LICM must not change instruction
// count (idempotency with LICM included).
TEST_CASE("licm_is_idempotent") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), lit(7)), std::move(while_body)));
  body->append(ret(var("i")));

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.optimize(gen.blocks());
  const size_t after_first = total_instr_count(gen.blocks());
  opt.optimize(gen.blocks());
  const size_t after_second = total_instr_count(gen.blocks());

  REQUIRE(after_second == after_first);
}

// ArrayStore is a side-effecting instruction; LICM must never hoist it out
// of the loop regardless of what registers it reads.
TEST_CASE("licm_does_not_hoist_array_store") {
  // block 0: r0=100 (fake array ref), r1=0 (i); Jump @1
  // block 1: r3=5; r4=i<5; JumpConditional @2,@3
  // block 2: ArrayStore(r0, r1, r1); r2=i+1; r1=r2; Jump @1
  // block 3: Return r1
  std::vector<Bytecode::BasicBlock> blocks(4);

  blocks[0].append<Bytecode::Instruction::Load>(0, 100);  // r0 = fake array
  blocks[0].append<Bytecode::Instruction::Load>(1, 0);    // r1 = 0 (i)
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Load>(3, 5);         // r3 = 5
  blocks[1].append<Bytecode::Instruction::LessThan>(4, 1, 3);  // r4 = i < 5
  blocks[1].append<Bytecode::Instruction::JumpConditional>(4, 2, 3);

  blocks[2].append<Bytecode::Instruction::ArrayStore>(0, 1, 1);    // array[i]=i
  blocks[2].append<Bytecode::Instruction::AddImmediate>(2, 1, 1);  // r2 = i+1
  blocks[2].append<Bytecode::Instruction::Move>(1, 2);             // i = r2
  blocks[2].append<Bytecode::Instruction::Jump>(1);

  blocks[3].append<Bytecode::Instruction::Return>(1);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // ArrayStore must remain in block 2 (loop body).
  bool has_array_store = false;
  for (const auto &instr_ptr : blocks[2].instructions) {
    if (instr_ptr->type() == Type::ArrayStore) has_array_store = true;
  }
  REQUIRE(has_array_store);
}

// ============================================================
// Pass 3: Peephole Optimizations
// ============================================================

// AddImmediate r_tmp + Move r_var, r_tmp collapses to AddImmediate r_var
// when r_tmp is used only by the Move.  Mirrors the i++ increment pattern
// after copy propagation has already substituted the variable read.
TEST_CASE("peephole_folds_add_immediate_move") {
  // block 0: Load r0, 0; Jump @1
  // block 1: AddImmediate r2, r0, 1; Move r0, r2; Jump @1  (back edge → loop)
  // block 2: Return r0
  // r2 is used only by the Move → should be folded away.
  std::vector<Bytecode::BasicBlock> blocks(3);
  blocks[0].append<Bytecode::Instruction::Load>(0, 0);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::AddImmediate>(2, 0, 1);  // r2 = r0 + 1
  blocks[1].append<Bytecode::Instruction::Move>(0, 2);             // r0 = r2
  blocks[1].append<Bytecode::Instruction::Jump>(1);

  blocks[2].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // Peephole: AddImmediate r2, r0, 1 + Move r0, r2 → AddImmediate r0, r0, 1.
  // Block 1 must have exactly 2 instructions: AddImmediate + Jump.
  REQUIRE(blocks[1].instructions.size() == 2);
  REQUIRE(blocks[1].instructions[0]->type() == Type::AddImmediate);
  const auto &ai =
      static_cast<const Bytecode::Instruction::AddImmediate &>(*blocks[1].instructions[0]);
  REQUIRE(ai.dst == 0);    // was r2, now r0
  REQUIRE(ai.src == 0);    // source register unchanged
  REQUIRE(ai.value == 1);
}

// Load r_tmp + Move r_var, r_tmp collapses to Load r_var when r_tmp is used
// only by the Move.  Mirrors the variable initializer pattern where the
// initializer loads into a fresh register and then Moves to the var slot.
TEST_CASE("peephole_folds_load_move") {
  // block 0: Load r0, 0; Move r1, r0; Jump @1
  // block 1: Return r1
  // r0 is used only by Move r1, r0 → collapse to Load r1, 0.
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 0);
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);   // r1 = r0
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Return>(1);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // Block 0 must have 2 instructions: Load r0, 0 and Jump @1 after compaction.
  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  const auto &ld =
      static_cast<const Bytecode::Instruction::Load &>(*blocks[0].instructions[0]);
  REQUIRE(ld.dst == 0);   // r1 gets compacted to r0
  REQUIRE(ld.value == 0);
}

// When the temporary register produced by AddImmediate is read by more than
// one instruction, the peephole must NOT fold — it would remove a use that
// is still needed.
TEST_CASE("peephole_not_applied_when_temp_has_multiple_uses") {
  // block 0: Load r0, 0; Jump @1
  // block 1: AddImmediate r2, r0, 1; Move r1, r2; JumpConditional r2, @2, @3
  //   r2 is used by both Move (as src) and JumpConditional (as cond) → count=2
  // blocks 2 & 3: Return r1
  std::vector<Bytecode::BasicBlock> blocks(4);
  blocks[0].append<Bytecode::Instruction::Load>(0, 0);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::AddImmediate>(2, 0, 1);    // r2 = r0 + 1
  blocks[1].append<Bytecode::Instruction::Move>(1, 2);               // r1 = r2
  blocks[1].append<Bytecode::Instruction::JumpConditional>(2, 2, 3); // also reads r2

  blocks[2].append<Bytecode::Instruction::Return>(1);
  blocks[3].append<Bytecode::Instruction::Return>(1);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  // AddImmediate must still write to r2 — its dst must not have been changed.
  bool add_imm_dst_is_r2 = false;
  for (const auto &instr_ptr : blocks[1].instructions) {
    if (instr_ptr->type() == Type::AddImmediate) {
      const auto &ai =
          static_cast<const Bytecode::Instruction::AddImmediate &>(*instr_ptr);
      if (ai.dst == 2) add_imm_dst_is_r2 = true;
    }
  }
  REQUIRE(add_imm_dst_is_r2);
}

// End-to-end structural test: after all passes the loop body of a
// "while (i < N) { i++; }" program must consist of exactly two instructions —
// AddImmediate r_i, r_i, 1 (in-place) and a Jump back — with no residual Move.
TEST_CASE("peephole_increment_loop_body_is_single_add_plus_jump") {
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

  // Find the loop body block: exactly 2 instructions, AddImmediate then Jump,
  // where the AddImmediate writes to its own source (in-place increment).
  bool found_optimal_body = false;
  for (const auto &blk : gen.blocks()) {
    if (blk.instructions.size() == 2 &&
        blk.instructions[0]->type() == Type::AddImmediate &&
        blk.instructions[1]->type() == Type::Jump) {
      const auto &ai =
          static_cast<const Bytecode::Instruction::AddImmediate &>(*blk.instructions[0]);
      if (ai.dst == ai.src) found_optimal_body = true;
    }
  }
  REQUIRE(found_optimal_body);
}

// Correctness: peephole must preserve the loop's computed value.
TEST_CASE("peephole_correctness_increment_loop") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), lit(7)), std::move(while_body)));
  body->append(ret(var("i")));

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.optimize(gen.blocks());

  kai::bytecode::BytecodeInterpreter interp;
  REQUIRE(interp.interpret(gen.blocks()) == 7);
}

// Idempotency: running optimize() a second time after the peephole must not
// change the instruction count.
TEST_CASE("peephole_is_idempotent") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), lit(5)), std::move(while_body)));
  body->append(ret(var("i")));

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.optimize(gen.blocks());
  const size_t after_first = total_instr_count(gen.blocks());
  opt.optimize(gen.blocks());
  const size_t after_second = total_instr_count(gen.blocks());

  REQUIRE(after_second == after_first);
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

// ============================================================
// Pass 4: Register Compaction
// ============================================================

TEST_CASE("register_compaction_removes_gaps_left_by_copy_prop") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 40);
  blocks[0].append<Bytecode::Instruction::Load>(1, 2);
  blocks[0].append<Bytecode::Instruction::Move>(2, 0);   // removed after copy-prop + DCE
  blocks[0].append<Bytecode::Instruction::Add>(3, 2, 1); // src1 rewritten to r0
  blocks[0].append<Bytecode::Instruction::Return>(3);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 4);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[1]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[2]->type() == Type::Add);
  REQUIRE(blocks[0].instructions[3]->type() == Type::Return);

  const auto &add =
      static_cast<const Bytecode::Instruction::Add &>(*blocks[0].instructions[2]);
  const auto &ret =
      static_cast<const Bytecode::Instruction::Return &>(*blocks[0].instructions[3]);

  // After compaction, live registers are dense: r0, r1, r2.
  REQUIRE(add.dst == 2);
  REQUIRE(add.src1 == 0);
  REQUIRE(add.src2 == 1);
  REQUIRE(ret.reg == 2);
}

TEST_CASE("register_compaction_renumbers_call_parameter_slots_consistently") {
  std::vector<Bytecode::BasicBlock> blocks(2);

  // Entry: call block 1 with arg r0 and parameter slot r8, then return call result.
  blocks[0].append<Bytecode::Instruction::Load>(0, 7);
  blocks[0].append<Bytecode::Instruction::Call>(
      10, 1, std::vector<Bytecode::Register>{0}, std::vector<Bytecode::Register>{8});
  blocks[0].append<Bytecode::Instruction::Return>(10);

  // Callee body uses the same parameter register slot.
  blocks[1].append<Bytecode::Instruction::AddImmediate>(9, 8, 1);
  blocks[1].append<Bytecode::Instruction::Return>(9);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions[1]->type() == Type::Call);
  REQUIRE(blocks[1].instructions[0]->type() == Type::AddImmediate);

  const auto &call =
      static_cast<const Bytecode::Instruction::Call &>(*blocks[0].instructions[1]);
  const auto &add_imm = static_cast<const Bytecode::Instruction::AddImmediate &>(
      *blocks[1].instructions[0]);
  const auto &callee_ret =
      static_cast<const Bytecode::Instruction::Return &>(*blocks[1].instructions[1]);
  const auto &entry_ret =
      static_cast<const Bytecode::Instruction::Return &>(*blocks[0].instructions[2]);

  // Original used registers {0,8,9,10} become dense {0,1,2,3}.
  REQUIRE(call.dst == 3);
  REQUIRE(call.arg_registers == std::vector<Bytecode::Register>{0});
  REQUIRE(call.param_registers == std::vector<Bytecode::Register>{1});
  REQUIRE(add_imm.src == 1);
  REQUIRE(add_imm.dst == 2);
  REQUIRE(callee_ret.reg == 2);
  REQUIRE(entry_ret.reg == 3);

  kai::bytecode::BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 8);
}
