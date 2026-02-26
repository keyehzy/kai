#include "test_optimizer_helpers.h"

// ============================================================
// Pass 3: Tail-Call Optimization
// ============================================================

TEST_CASE("tco_rewrites_call_followed_by_return") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 7);
  blocks[0].append<Bytecode::Instruction::Call>(
      2, 1, std::vector<Bytecode::Register>{0}, std::vector<Bytecode::Register>{1});
  blocks[0].append<Bytecode::Instruction::Return>(2);
  blocks[1].append<Bytecode::Instruction::AddImmediate>(3, 1, 1);
  blocks[1].append<Bytecode::Instruction::Return>(3);

  BytecodeOptimizer opt;
  opt.tail_call_optimization(blocks);

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[1]->type() == Type::TailCall);

  const auto &tail_call =
      static_cast<const Bytecode::Instruction::TailCall &>(*blocks[0].instructions[1]);
  REQUIRE(tail_call.arg_registers == std::vector<Bytecode::Register>{0});
  REQUIRE(tail_call.param_registers == std::vector<Bytecode::Register>{1});

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 8);
}

TEST_CASE("tco_does_not_rewrite_when_return_register_differs") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 7);
  blocks[0].append<Bytecode::Instruction::Call>(
      2, 1, std::vector<Bytecode::Register>{0}, std::vector<Bytecode::Register>{1});
  blocks[0].append<Bytecode::Instruction::Return>(0);  // not returning call destination
  blocks[1].append<Bytecode::Instruction::Return>(1);

  BytecodeOptimizer opt;
  opt.tail_call_optimization(blocks);

  REQUIRE(blocks[0].instructions[1]->type() == Type::Call);
  REQUIRE(blocks[0].instructions[2]->type() == Type::Return);
}

