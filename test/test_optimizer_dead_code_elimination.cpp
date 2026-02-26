#include "test_optimizer_helpers.h"

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
  opt.dead_code_elimination(blocks);

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
  opt.dead_code_elimination(blocks);

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
  opt.dead_code_elimination(blocks);

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
  opt.dead_code_elimination(blocks);

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
  opt.dead_code_elimination(blocks);

  REQUIRE(blocks[0].instructions.size() == 1);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Jump);
}

// JumpConditional has a condition register (src operand) but no dst; it is
// side-effecting (controls flow) and is never removed.
TEST_CASE("dce_jump_conditional_never_removed") {
  std::vector<Bytecode::BasicBlock> blocks(3);
  blocks[0].append<Bytecode::Instruction::Load>(0, 1);
  blocks[0].append<Bytecode::Instruction::AddImmediate>(0, 0, 0);
  blocks[0].append<Bytecode::Instruction::JumpConditional>(0, 1, 2);
  blocks[1].append<Bytecode::Instruction::Load>(1, 10);
  blocks[1].append<Bytecode::Instruction::Return>(1);
  blocks[2].append<Bytecode::Instruction::Load>(2, 20);
  blocks[2].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.dead_code_elimination(blocks);

  REQUIRE(blocks[0].instructions.back()->type() == Type::JumpConditional);
}

// Return is a terminator; it is never removed.
TEST_CASE("dce_return_never_removed") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 42);
  blocks[0].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.dead_code_elimination(blocks);

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
  opt.dead_code_elimination(blocks);

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
  opt.dead_code_elimination(blocks);

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
}
