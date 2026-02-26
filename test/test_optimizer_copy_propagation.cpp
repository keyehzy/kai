#include "test_optimizer_helpers.h"

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
  opt.copy_propagation(blocks);

  // Copy prop rewrites through the move chain but does not run DCE.
  REQUIRE(blocks[0].instructions.size() == 4);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[3]->type() == Type::Return);
  const auto &ret =
      static_cast<const Bytecode::Instruction::Return &>(*blocks[0].instructions[3]);
  REQUIRE(ret.reg == 0);
}

// Move r0, r0 is trivially useless and must be removed during copy propagation.
TEST_CASE("copy_prop_trivial_self_move_removed") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 99);
  blocks[0].append<Bytecode::Instruction::Move>(0, 0);  // dst == src
  blocks[0].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.copy_propagation(blocks);

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
  opt.copy_propagation(blocks);

  REQUIRE(blocks[0].instructions.size() == 5);
  REQUIRE(blocks[0].instructions[3]->type() == Type::Add);
  const auto &add =
      static_cast<const Bytecode::Instruction::Add &>(*blocks[0].instructions[3]);
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
  opt.copy_propagation(blocks);

  REQUIRE(blocks[0].instructions.size() == 5);
  REQUIRE(blocks[0].instructions[2]->type() == Type::AddImmediate);
  const auto &ai =
      static_cast<const Bytecode::Instruction::AddImmediate &>(*blocks[0].instructions[2]);
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
  opt.copy_propagation(blocks);

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
  blocks[0].append<Bytecode::Instruction::AddImmediate>(0, 0, 0);
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);               // r1 = r0
  blocks[0].append<Bytecode::Instruction::JumpConditional>(1, 1, 2); // cond = r1

  blocks[1].append<Bytecode::Instruction::Load>(2, 42);
  blocks[1].append<Bytecode::Instruction::Return>(2);

  blocks[2].append<Bytecode::Instruction::Load>(3, 0);
  blocks[2].append<Bytecode::Instruction::Return>(3);

  BytecodeOptimizer opt;
  opt.copy_propagation(blocks);

  REQUIRE(blocks[0].instructions.size() == 4);
  REQUIRE(blocks[0].instructions[3]->type() == Type::JumpConditional);
  const auto &jc = static_cast<const Bytecode::Instruction::JumpConditional &>(
      *blocks[0].instructions[3]);
  REQUIRE(jc.cond == 0);
}

// The register operand of Return is resolved through the copy map.
TEST_CASE("copy_prop_resolves_return_register") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 7);
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);   // r1 = r0
  blocks[0].append<Bytecode::Instruction::Return>(1);    // return r1

  BytecodeOptimizer opt;
  opt.copy_propagation(blocks);

  REQUIRE(blocks[0].instructions.size() == 3);
  REQUIRE(blocks[0].instructions[2]->type() == Type::Return);
  const auto &ret =
      static_cast<const Bytecode::Instruction::Return &>(*blocks[0].instructions[2]);
  REQUIRE(ret.reg == 0);
}
