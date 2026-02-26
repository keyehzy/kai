#include "test_optimizer_helpers.h"

// ============================================================
// Synergy: Targeted Cross-Pass Interactions
// ============================================================

TEST_CASE("synergy_copy_prop_compare_fusion_and_dce_remove_alias_compare_chain") {
  std::vector<Bytecode::BasicBlock> blocks(3);

  blocks[0].append<Bytecode::Instruction::Load>(0, 10);
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);                 // alias
  blocks[0].append<Bytecode::Instruction::EqualImmediate>(2, 1, 10);   // reads alias
  blocks[0].append<Bytecode::Instruction::JumpConditional>(2, 1, 2);

  blocks[1].append<Bytecode::Instruction::Load>(3, 7);
  blocks[1].append<Bytecode::Instruction::Return>(3);

  blocks[2].append<Bytecode::Instruction::Load>(4, 9);
  blocks[2].append<Bytecode::Instruction::Return>(4);

  BytecodeOptimizer opt;
  opt.copy_propagation(blocks);
  opt.fuse_compare_branches(blocks);
  opt.dead_code_elimination(blocks);

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[1]->type() == Type::JumpEqualImmediate);
  REQUIRE_FALSE(has_instruction_type(blocks, Type::Move));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::EqualImmediate));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::JumpConditional));

  const auto &jump = static_cast<const Bytecode::Instruction::JumpEqualImmediate &>(
      *blocks[0].instructions[1]);
  REQUIRE(jump.src == 0);
  REQUIRE(jump.value == 10);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 7);
}

TEST_CASE("synergy_fold_aggregate_literals_then_dce_prunes_constant_setup_loads") {
  std::vector<Bytecode::BasicBlock> blocks(1);

  blocks[0].append<Bytecode::Instruction::Load>(10, 3);
  blocks[0].append<Bytecode::Instruction::Load>(11, 4);
  blocks[0].append<Bytecode::Instruction::ArrayCreate>(
      12, std::vector<Bytecode::Register>{10, 11});
  blocks[0].append<Bytecode::Instruction::Load>(13, 1);
  blocks[0].append<Bytecode::Instruction::ArrayLoad>(14, 12, 13);
  blocks[0].append<Bytecode::Instruction::Return>(14);

  BytecodeOptimizer opt;
  opt.fold_aggregate_literals(blocks);
  opt.dead_code_elimination(blocks);

  REQUIRE(blocks[0].instructions.size() == 3);
  REQUIRE(blocks[0].instructions[0]->type() == Type::ArrayLiteralCreate);
  REQUIRE(blocks[0].instructions[1]->type() == Type::ArrayLoadImmediate);
  REQUIRE(blocks[0].instructions[2]->type() == Type::Return);
  REQUIRE_FALSE(has_instruction_type(blocks, Type::Load));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::ArrayCreate));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::ArrayLoad));

  const auto &ali = static_cast<const Bytecode::Instruction::ArrayLoadImmediate &>(
      *blocks[0].instructions[1]);
  REQUIRE(ali.index == 1);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 4);
}

TEST_CASE("synergy_tco_then_cfg_cleanup_trims_tailcall_following_dead_instructions") {
  std::vector<Bytecode::BasicBlock> blocks(3);

  blocks[0].append<Bytecode::Instruction::Load>(0, 7);
  blocks[0].append<Bytecode::Instruction::Call>(
      2, 1, std::vector<Bytecode::Register>{0}, std::vector<Bytecode::Register>{1});
  blocks[0].append<Bytecode::Instruction::Return>(2);
  blocks[0].append<Bytecode::Instruction::Load>(9, 999);  // dead after tail-call terminator

  blocks[1].append<Bytecode::Instruction::AddImmediate>(3, 1, 1);
  blocks[1].append<Bytecode::Instruction::Return>(3);

  blocks[2].append<Bytecode::Instruction::Load>(4, 12345);  // unreachable
  blocks[2].append<Bytecode::Instruction::Return>(4);

  BytecodeOptimizer opt;
  opt.tail_call_optimization(blocks);
  opt.cfg_cleanup(blocks);

  REQUIRE(blocks.size() == 2);
  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[1]->type() == Type::TailCall);
  REQUIRE_FALSE(has_instruction_type(blocks, Type::Call));

  const auto &tail_call = static_cast<const Bytecode::Instruction::TailCall &>(
      *blocks[0].instructions[1]);
  REQUIRE(tail_call.label == 1);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 8);
}

TEST_CASE("synergy_const_condition_simplification_then_cfg_cleanup_removes_dead_branch") {
  std::vector<Bytecode::BasicBlock> blocks(3);

  blocks[0].append<Bytecode::Instruction::Load>(0, 0);  // false
  blocks[0].append<Bytecode::Instruction::JumpConditional>(0, 1, 2);

  blocks[1].append<Bytecode::Instruction::Load>(1, 11);  // should be removed as unreachable
  blocks[1].append<Bytecode::Instruction::Return>(1);

  blocks[2].append<Bytecode::Instruction::Load>(2, 22);
  blocks[2].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.simplify_constant_conditions(blocks);
  opt.cfg_cleanup(blocks);

  REQUIRE(blocks.size() == 2);
  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[1]->type() == Type::Jump);
  REQUIRE_FALSE(has_instruction_type(blocks, Type::JumpConditional));

  const auto &jump =
      static_cast<const Bytecode::Instruction::Jump &>(*blocks[0].instructions[1]);
  REQUIRE(jump.label == 1);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 22);
}
