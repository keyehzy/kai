#include "test_optimizer_helpers.h"

TEST_CASE("compare_branch_fusion_rewrites_supported_patterns") {
  std::vector<Bytecode::BasicBlock> blocks(5);

  blocks[0].append<Bytecode::Instruction::Load>(0, 10);
  blocks[0].append<Bytecode::Instruction::Load>(1, 8);
  blocks[0].append<Bytecode::Instruction::EqualImmediate>(2, 0, 10);
  blocks[0].append<Bytecode::Instruction::JumpConditional>(2, 1, 2);

  blocks[1].append<Bytecode::Instruction::GreaterThanImmediate>(3, 1, 5);
  blocks[1].append<Bytecode::Instruction::JumpConditional>(3, 3, 4);

  blocks[2].append<Bytecode::Instruction::LessThanOrEqual>(4, 0, 1);
  blocks[2].append<Bytecode::Instruction::JumpConditional>(4, 3, 4);

  blocks[3].append<Bytecode::Instruction::Return>(0);
  blocks[4].append<Bytecode::Instruction::Return>(1);

  BytecodeOptimizer opt;
  opt.fuse_compare_branches(blocks);

  REQUIRE(blocks[0].instructions.size() == 3);
  REQUIRE(blocks[1].instructions.size() == 1);
  REQUIRE(blocks[2].instructions.size() == 1);
  REQUIRE(blocks[3].instructions.size() == 1);
  REQUIRE(blocks[4].instructions.size() == 1);

  REQUIRE(has_instruction_type(blocks, Type::JumpEqualImmediate));
  REQUIRE(has_instruction_type(blocks, Type::JumpGreaterThanImmediate));
  REQUIRE(has_instruction_type(blocks, Type::JumpLessThanOrEqual));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::JumpConditional));

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 10);
}

TEST_CASE("compare_branch_fusion_skips_when_compare_result_is_reused") {
  std::vector<Bytecode::BasicBlock> blocks(3);

  blocks[0].append<Bytecode::Instruction::Load>(0, 7);
  blocks[0].append<Bytecode::Instruction::GreaterThanImmediate>(1, 0, 3);
  blocks[0].append<Bytecode::Instruction::JumpConditional>(1, 1, 2);

  blocks[1].append<Bytecode::Instruction::Return>(1);  // extra use of compare result
  blocks[2].append<Bytecode::Instruction::Load>(2, 0);
  blocks[2].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.fuse_compare_branches(blocks);

  REQUIRE(blocks[0].instructions.size() == 3);
  REQUIRE(blocks[1].instructions.size() == 1);
  REQUIRE(blocks[2].instructions.size() == 2);

  REQUIRE(has_instruction_type(blocks, Type::GreaterThanImmediate));
  REQUIRE(has_instruction_type(blocks, Type::JumpConditional));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::JumpGreaterThanImmediate));
}

TEST_CASE("jump_conditional_is_preserved_for_non_compare_boolean_conditions") {
  std::vector<Bytecode::BasicBlock> blocks(3);

  blocks[0].append<Bytecode::Instruction::Load>(0, 5);
  blocks[0].append<Bytecode::Instruction::AddImmediate>(1, 0, 0);
  blocks[0].append<Bytecode::Instruction::JumpConditional>(1, 1, 2);

  blocks[1].append<Bytecode::Instruction::Load>(2, 42);
  blocks[1].append<Bytecode::Instruction::Return>(2);

  blocks[2].append<Bytecode::Instruction::Load>(3, 7);
  blocks[2].append<Bytecode::Instruction::Return>(3);

  BytecodeOptimizer opt;
  opt.fuse_compare_branches(blocks);

  REQUIRE(blocks[0].instructions.size() == 3);
  REQUIRE(blocks[1].instructions.size() == 2);
  REQUIRE(blocks[2].instructions.size() == 2);

  REQUIRE(has_instruction_type(blocks, Type::JumpConditional));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::JumpEqualImmediate));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::JumpGreaterThanImmediate));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::JumpLessThanOrEqual));

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 42);
}
