#include "test_optimizer_helpers.h"

// ============================================================
// End-to-End: Full optimize() Pipeline
// ============================================================

TEST_CASE("optimize_pipeline_eliminates_unreachable_branch_after_constant_condition") {
  std::vector<Bytecode::BasicBlock> blocks(3);

  blocks[0].append<Bytecode::Instruction::Load>(10, 0);  // false branch
  blocks[0].append<Bytecode::Instruction::JumpConditional>(10, 1, 2);

  blocks[1].append<Bytecode::Instruction::Load>(20, 111);  // unreachable after simplify
  blocks[1].append<Bytecode::Instruction::Return>(20);

  blocks[2].append<Bytecode::Instruction::Load>(30, 222);
  blocks[2].append<Bytecode::Instruction::Return>(30);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks.size() == 2);
  REQUIRE(blocks[0].instructions.size() == 1);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Jump);
  REQUIRE_FALSE(has_instruction_type(blocks, Type::JumpConditional));

  const auto &jump =
      static_cast<const Bytecode::Instruction::Jump &>(*blocks[0].instructions[0]);
  REQUIRE(jump.label == 1);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 222);
}

TEST_CASE("optimize_pipeline_fuses_alias_compare_branch_and_compacts_registers") {
  std::vector<Bytecode::BasicBlock> blocks(3);

  blocks[0].append<Bytecode::Instruction::Load>(10, 9);
  blocks[0].append<Bytecode::Instruction::Move>(20, 10);                 // alias
  blocks[0].append<Bytecode::Instruction::GreaterThanImmediate>(30, 20, 5);
  blocks[0].append<Bytecode::Instruction::JumpConditional>(30, 1, 2);

  blocks[1].append<Bytecode::Instruction::Load>(40, 1);
  blocks[1].append<Bytecode::Instruction::Return>(40);

  blocks[2].append<Bytecode::Instruction::Load>(50, 0);
  blocks[2].append<Bytecode::Instruction::Return>(50);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[1]->type() == Type::JumpGreaterThanImmediate);
  REQUIRE_FALSE(has_instruction_type(blocks, Type::Move));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::GreaterThanImmediate));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::JumpConditional));

  const auto &load =
      static_cast<const Bytecode::Instruction::Load &>(*blocks[0].instructions[0]);
  const auto &jump = static_cast<const Bytecode::Instruction::JumpGreaterThanImmediate &>(
      *blocks[0].instructions[1]);
  REQUIRE(load.dst == 0);  // compacted from r10
  REQUIRE(jump.lhs == 0);  // compacted and propagated from alias
  REQUIRE(jump.value == 5);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 1);
}

TEST_CASE("optimize_pipeline_materializes_array_literals_and_removes_setup_registers") {
  std::vector<Bytecode::BasicBlock> blocks(1);

  blocks[0].append<Bytecode::Instruction::Load>(10, 3);
  blocks[0].append<Bytecode::Instruction::Load>(12, 4);
  blocks[0].append<Bytecode::Instruction::ArrayCreate>(
      20, std::vector<Bytecode::Register>{10, 12});
  blocks[0].append<Bytecode::Instruction::Load>(25, 1);
  blocks[0].append<Bytecode::Instruction::ArrayLoad>(30, 20, 25);
  blocks[0].append<Bytecode::Instruction::Return>(30);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 3);
  REQUIRE(blocks[0].instructions[0]->type() == Type::ArrayLiteralCreate);
  REQUIRE(blocks[0].instructions[1]->type() == Type::ArrayLoadImmediate);
  REQUIRE(blocks[0].instructions[2]->type() == Type::Return);
  REQUIRE_FALSE(has_instruction_type(blocks, Type::Load));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::ArrayCreate));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::ArrayLoad));

  const auto &array_lit =
      static_cast<const Bytecode::Instruction::ArrayLiteralCreate &>(
          *blocks[0].instructions[0]);
  const auto &array_load =
      static_cast<const Bytecode::Instruction::ArrayLoadImmediate &>(
          *blocks[0].instructions[1]);
  const auto &ret =
      static_cast<const Bytecode::Instruction::Return &>(*blocks[0].instructions[2]);

  REQUIRE(array_lit.dst == 0);      // compacted from r20
  REQUIRE(array_load.array == 0);   // reads compacted literal register
  REQUIRE(array_load.index == 1);   // folded immediate index
  REQUIRE(ret.reg == array_load.dst);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 4);
}

TEST_CASE("optimize_pipeline_emits_tailcall_and_removes_unreachable_blocks") {
  std::vector<Bytecode::BasicBlock> blocks(3);

  blocks[0].append<Bytecode::Instruction::Load>(10, 7);
  blocks[0].append<Bytecode::Instruction::Call>(
      20, 1, std::vector<Bytecode::Register>{10}, std::vector<Bytecode::Register>{11});
  blocks[0].append<Bytecode::Instruction::Return>(20);
  blocks[0].append<Bytecode::Instruction::Load>(30, 999);  // dead after terminator

  blocks[1].append<Bytecode::Instruction::AddImmediate>(21, 11, 1);
  blocks[1].append<Bytecode::Instruction::Return>(21);

  blocks[2].append<Bytecode::Instruction::Load>(40, 12345);  // unreachable
  blocks[2].append<Bytecode::Instruction::Return>(40);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks.size() == 2);
  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[1]->type() == Type::TailCall);
  REQUIRE_FALSE(has_instruction_type(blocks, Type::Call));

  const auto &tail_call = static_cast<const Bytecode::Instruction::TailCall &>(
      *blocks[0].instructions[1]);
  REQUIRE(tail_call.label == 1);
  REQUIRE(tail_call.arg_registers == std::vector<Bytecode::Register>{0});
  REQUIRE(tail_call.param_registers == std::vector<Bytecode::Register>{1});

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 8);
}
