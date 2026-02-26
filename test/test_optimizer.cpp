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
  REQUIRE(jump.label == 1);
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

// ============================================================
// Pass 4: Peephole Optimizations (non-immediate producers)
// ============================================================

TEST_CASE("peephole_folds_divide_move_non_immediate") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 84);
  blocks[0].append<Bytecode::Instruction::Load>(1, 7);
  blocks[0].append<Bytecode::Instruction::Divide>(2, 0, 1);
  blocks[0].append<Bytecode::Instruction::Move>(0, 2);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 4);
  REQUIRE(blocks[0].instructions[2]->type() == Type::Divide);
  const auto &div =
      static_cast<const Bytecode::Instruction::Divide &>(*blocks[0].instructions[2]);
  REQUIRE(div.dst == 0);
  REQUIRE(div.src1 == 0);
  REQUIRE(div.src2 == 1);
  REQUIRE_FALSE(has_instruction_type(blocks, Type::Move));
}

TEST_CASE("peephole_folds_multiply_move_non_immediate") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 6);
  blocks[0].append<Bytecode::Instruction::Load>(1, 7);
  blocks[0].append<Bytecode::Instruction::Multiply>(2, 0, 1);
  blocks[0].append<Bytecode::Instruction::Move>(0, 2);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 4);
  REQUIRE(blocks[0].instructions[2]->type() == Type::Multiply);
  const auto &mul =
      static_cast<const Bytecode::Instruction::Multiply &>(*blocks[0].instructions[2]);
  REQUIRE(mul.dst == 0);
  REQUIRE(mul.src1 == 0);
  REQUIRE(mul.src2 == 1);
  REQUIRE_FALSE(has_instruction_type(blocks, Type::Move));
}

TEST_CASE("peephole_non_immediate_fold_preserves_interpreter_result") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 84);
  blocks[0].append<Bytecode::Instruction::Load>(1, 7);
  blocks[0].append<Bytecode::Instruction::Divide>(2, 0, 1);
  blocks[0].append<Bytecode::Instruction::Move>(0, 2);  // becomes Divide r0, r0, r1
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 12);
}

TEST_CASE("peephole_non_immediate_not_applied_when_temp_has_multiple_uses") {
  std::vector<Bytecode::BasicBlock> blocks(4);
  blocks[0].append<Bytecode::Instruction::Load>(0, 6);
  blocks[0].append<Bytecode::Instruction::Load>(1, 7);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Multiply>(2, 0, 1);      // r2 = r0 * r1
  blocks[1].append<Bytecode::Instruction::Move>(3, 2);             // use #1
  blocks[1].append<Bytecode::Instruction::JumpConditional>(2, 2, 3);  // use #2

  blocks[2].append<Bytecode::Instruction::Return>(3);
  blocks[3].append<Bytecode::Instruction::Return>(3);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  bool multiply_dst_is_temp = false;
  for (const auto &instr_ptr : blocks[1].instructions) {
    if (instr_ptr->type() == Type::Multiply) {
      const auto &mul =
          static_cast<const Bytecode::Instruction::Multiply &>(*instr_ptr);
      if (mul.dst == 2) multiply_dst_is_temp = true;
    }
  }
  REQUIRE(multiply_dst_is_temp);
}
