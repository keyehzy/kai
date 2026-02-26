#include "test_optimizer_helpers.h"

// ============================================================
// Constant-Condition Simplification
// ============================================================

TEST_CASE("const_cond_jump_conditional_true_simplifies_to_jump_true_label") {
  std::vector<Bytecode::BasicBlock> blocks(3);
  blocks[0].append<Bytecode::Instruction::Load>(0, 1);
  blocks[0].append<Bytecode::Instruction::JumpConditional>(0, 1, 2);

  blocks[1].append<Bytecode::Instruction::Load>(1, 11);
  blocks[1].append<Bytecode::Instruction::Return>(1);

  blocks[2].append<Bytecode::Instruction::Load>(2, 22);
  blocks[2].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 1);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Jump);
  const auto &jump =
      static_cast<const Bytecode::Instruction::Jump &>(*blocks[0].instructions[0]);
  REQUIRE(jump.label == 1);
}

TEST_CASE("const_cond_jump_conditional_false_simplifies_to_jump_false_label") {
  std::vector<Bytecode::BasicBlock> blocks(3);
  blocks[0].append<Bytecode::Instruction::Load>(0, 0);
  blocks[0].append<Bytecode::Instruction::JumpConditional>(0, 1, 2);

  blocks[1].append<Bytecode::Instruction::Load>(1, 11);
  blocks[1].append<Bytecode::Instruction::Return>(1);

  blocks[2].append<Bytecode::Instruction::Load>(2, 22);
  blocks[2].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 1);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Jump);
  const auto &jump =
      static_cast<const Bytecode::Instruction::Jump &>(*blocks[0].instructions[0]);
  REQUIRE(jump.label == 2);
}

TEST_CASE("const_cond_no_simplify_when_condition_overwritten") {
  std::vector<Bytecode::BasicBlock> blocks(3);
  blocks[0].append<Bytecode::Instruction::Load>(0, 1);
  blocks[0].append<Bytecode::Instruction::AddImmediate>(0, 0, 1);
  blocks[0].append<Bytecode::Instruction::JumpConditional>(0, 1, 2);

  blocks[1].append<Bytecode::Instruction::Load>(1, 11);
  blocks[1].append<Bytecode::Instruction::Return>(1);

  blocks[2].append<Bytecode::Instruction::Load>(2, 22);
  blocks[2].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 3);
  REQUIRE(blocks[0].instructions[2]->type() == Type::JumpConditional);
}

TEST_CASE("const_cond_while_true_has_no_jump_conditional") {
  Ast::Block program;
  program.append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  program.append(while_loop(lit(1), std::move(while_body)));
  program.append(ret(var("i")));

  BytecodeGenerator gen;
  gen.visit_block(program);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.optimize(gen.blocks());

  REQUIRE_FALSE(has_instruction_type(gen.blocks(), Type::JumpConditional));
}
