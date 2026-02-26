#include "test_optimizer_helpers.h"

// ============================================================
// CFG Cleanup
// ============================================================

TEST_CASE("cfg_cleanup_removes_instructions_after_return") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 7);
  blocks[0].append<Bytecode::Instruction::Return>(0);
  blocks[0].append<Bytecode::Instruction::Jump>(1);  // dead after Return

  blocks[1].append<Bytecode::Instruction::Load>(1, 0);
  blocks[1].append<Bytecode::Instruction::Return>(1);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[1]->type() == Type::Return);
}

TEST_CASE("cfg_cleanup_rewires_and_removes_jump_only_chain") {
  std::vector<Bytecode::BasicBlock> blocks(4);
  blocks[0].append<Bytecode::Instruction::Load>(0, 123);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Jump>(2);  // trampoline
  blocks[2].append<Bytecode::Instruction::Jump>(3);  // trampoline
  blocks[3].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks.size() == 2);
  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[1]->type() == Type::Jump);
  const auto &jump =
      static_cast<const Bytecode::Instruction::Jump &>(*blocks[0].instructions[1]);
  REQUIRE(jump.label == 1);
  REQUIRE(blocks[1].instructions.size() == 1);
  REQUIRE(blocks[1].instructions[0]->type() == Type::Return);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 123);
}

TEST_CASE("cfg_cleanup_rewrites_jump_conditional_targets_through_trampolines") {
  std::vector<Bytecode::BasicBlock> blocks(5);
  blocks[0].append<Bytecode::Instruction::Load>(0, 1);
  blocks[0].append<Bytecode::Instruction::AddImmediate>(0, 0, 0);
  blocks[0].append<Bytecode::Instruction::JumpConditional>(0, 1, 2);

  blocks[1].append<Bytecode::Instruction::Jump>(3);  // trampoline
  blocks[2].append<Bytecode::Instruction::Jump>(4);  // trampoline

  blocks[3].append<Bytecode::Instruction::Load>(1, 11);
  blocks[3].append<Bytecode::Instruction::Return>(1);

  blocks[4].append<Bytecode::Instruction::Load>(2, 22);
  blocks[4].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks.size() == 3);
  REQUIRE(blocks[0].instructions.back()->type() == Type::JumpConditional);
  const auto &jump_cond = static_cast<const Bytecode::Instruction::JumpConditional &>(
      *blocks[0].instructions.back());
  REQUIRE(jump_cond.label1 == 1);
  REQUIRE(jump_cond.label2 == 2);
  const bool target1_is_jump_only =
      blocks[jump_cond.label1].instructions.size() == 1 &&
      blocks[jump_cond.label1].instructions[0]->type() == Type::Jump;
  const bool target2_is_jump_only =
      blocks[jump_cond.label2].instructions.size() == 1 &&
      blocks[jump_cond.label2].instructions[0]->type() == Type::Jump;
  REQUIRE_FALSE(target1_is_jump_only);
  REQUIRE_FALSE(target2_is_jump_only);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 11);
}
