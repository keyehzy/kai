#include "test_optimizer_helpers.h"

// ============================================================
// Pass 0: Loop-Invariant Code Motion (LICM)
// ============================================================

// An invariant bound load in the loop header must be hoisted to pre-header.
TEST_CASE("licm_hoists_invariant_loop_bound_load") {
  // block 0: r0=0; Jump @1
  // block 1: r1=10 (hoistable); r2=i<10; JumpConditional @2,@3
  // block 2: r3=i+1; r0=r3; Jump @1
  // block 3: Return r0
  std::vector<Bytecode::BasicBlock> blocks(4);

  blocks[0].append<Bytecode::Instruction::Load>(0, 0);  // r0 = 0 (i)
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Load>(1, 10);        // r1 = 10 -- hoistable
  blocks[1].append<Bytecode::Instruction::LessThan>(2, 0, 1);  // r2 = i < 10
  blocks[1].append<Bytecode::Instruction::JumpConditional>(2, 2, 3);

  blocks[2].append<Bytecode::Instruction::AddImmediate>(3, 0, 1);  // r3 = i+1
  blocks[2].append<Bytecode::Instruction::Move>(0, 3);             // i = r3
  blocks[2].append<Bytecode::Instruction::Jump>(1);

  blocks[3].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.loop_invariant_code_motion(blocks);

  bool load_10_in_condition = false;
  for (const auto &instr_ptr : blocks[1].instructions) {
    if (instr_ptr->type() == Type::Load) {
      const auto &load =
          static_cast<const Bytecode::Instruction::Load &>(*instr_ptr);
      if (load.value == 10) load_10_in_condition = true;
    }
  }
  REQUIRE_FALSE(load_10_in_condition);

  bool load_10_in_preheader = false;
  for (const auto &instr_ptr : blocks[0].instructions) {
    if (instr_ptr->type() == Type::Load) {
      const auto &load =
          static_cast<const Bytecode::Instruction::Load &>(*instr_ptr);
      if (load.value == 10) load_10_in_preheader = true;
    }
  }
  REQUIRE(load_10_in_preheader);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 10);
}

// The interpreter must still produce the correct result (10) after LICM.
TEST_CASE("licm_correctness_while_loop") {
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
  opt.loop_invariant_code_motion(gen.blocks());

  BytecodeInterpreter interp;
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
  opt.loop_invariant_code_motion(blocks);

  // Multiply must have been hoisted out of the loop blocks.
  bool multiply_in_loop = false;
  for (size_t i = 1; i <= 2; ++i) {
    for (const auto &instr_ptr : blocks[i].instructions) {
      if (instr_ptr->type() == Type::Multiply) multiply_in_loop = true;
    }
  }
  REQUIRE_FALSE(multiply_in_loop);

  // Interpreter result must be 6 (2 * 3).
  BytecodeInterpreter interp;
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
  opt.loop_invariant_code_motion(blocks);

  // LessThan must remain somewhere in the loop blocks (1 or 2).
  bool less_than_in_loop = false;
  for (size_t i = 1; i <= 2; ++i) {
    for (const auto &instr_ptr : blocks[i].instructions) {
      if (instr_ptr->type() == Type::LessThan) less_than_in_loop = true;
    }
  }
  REQUIRE(less_than_in_loop);
}

// Straight-line code with no loops must survive LICM without crashing
// and must still produce the correct result.
TEST_CASE("licm_no_crash_without_loop") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("a", lit(3)));
  body->append(decl("b", lit(4)));
  body->append(ret(add(var("a"), var("b"))));

  BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  REQUIRE_NOTHROW(opt.loop_invariant_code_motion(gen.blocks()));

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(gen.blocks()) == 7);
}

// The total instruction count across hot blocks (block 1+) must decrease
// after LICM because the loop-bound Load is moved to the pre-header.
TEST_CASE("licm_reduces_instruction_count_in_hot_blocks") {
  auto make_prog = [] {
    std::vector<Bytecode::BasicBlock> blocks(4);

    blocks[0].append<Bytecode::Instruction::Load>(0, 0);  // r0 = 0 (i)
    blocks[0].append<Bytecode::Instruction::Jump>(1);

    blocks[1].append<Bytecode::Instruction::Load>(1, 10);        // r1 = 10 -- hoistable
    blocks[1].append<Bytecode::Instruction::LessThan>(2, 0, 1);  // r2 = i < 10
    blocks[1].append<Bytecode::Instruction::JumpConditional>(2, 2, 3);

    blocks[2].append<Bytecode::Instruction::AddImmediate>(3, 0, 1);  // r3 = i+1
    blocks[2].append<Bytecode::Instruction::Move>(0, 3);             // i = r3
    blocks[2].append<Bytecode::Instruction::Jump>(1);

    blocks[3].append<Bytecode::Instruction::Return>(0);
    return blocks;
  };

  // Unoptimized count inside loop blocks (block 1 onward).
  size_t before = 0;
  auto blocks_before = make_prog();
  for (size_t i = 1; i < blocks_before.size(); ++i) {
    before += blocks_before[i].instructions.size();
  }

  // Optimized count.
  auto blocks_after = make_prog();
  BytecodeOptimizer opt;
  opt.loop_invariant_code_motion(blocks_after);
  size_t after = 0;
  for (size_t i = 1; i < blocks_after.size(); ++i) {
    after += blocks_after[i].instructions.size();
  }

  REQUIRE(after < before);
}

// Running LICM a second time must not change instruction count (idempotency).
TEST_CASE("licm_is_idempotent") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), lit(7)), std::move(while_body)));
  body->append(ret(var("i")));

  BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.loop_invariant_code_motion(gen.blocks());
  const size_t after_first = total_instr_count(gen.blocks());
  opt.loop_invariant_code_motion(gen.blocks());
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
  opt.loop_invariant_code_motion(blocks);

  // ArrayStore must remain in block 2 (loop body).
  bool has_array_store = false;
  for (const auto &instr_ptr : blocks[2].instructions) {
    if (instr_ptr->type() == Type::ArrayStore) has_array_store = true;
  }
  REQUIRE(has_array_store);
}

