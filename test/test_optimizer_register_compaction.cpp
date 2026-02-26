#include "test_optimizer_helpers.h"

// ============================================================
// Pass 5: Register Compaction
// ============================================================

TEST_CASE("register_compaction_removes_gaps_left_by_copy_prop") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 40);
  blocks[0].append<Bytecode::Instruction::Load>(1, 2);
  blocks[0].append<Bytecode::Instruction::Move>(2, 0);   // removed after copy-prop + DCE
  blocks[0].append<Bytecode::Instruction::Add>(3, 2, 1); // src1 rewritten to r0
  blocks[0].append<Bytecode::Instruction::Return>(3);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions.size() == 4);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[1]->type() == Type::Load);
  REQUIRE(blocks[0].instructions[2]->type() == Type::Add);
  REQUIRE(blocks[0].instructions[3]->type() == Type::Return);

  const auto &add =
      static_cast<const Bytecode::Instruction::Add &>(*blocks[0].instructions[2]);
  const auto &ret =
      static_cast<const Bytecode::Instruction::Return &>(*blocks[0].instructions[3]);

  // After compaction, live registers are dense: r0, r1, r2.
  REQUIRE(add.dst == 2);
  REQUIRE(add.src1 == 0);
  REQUIRE(add.src2 == 1);
  REQUIRE(ret.reg == 2);
}

TEST_CASE("register_compaction_renumbers_call_parameter_slots_consistently") {
  std::vector<Bytecode::BasicBlock> blocks(2);

  // Entry: call block 1 with arg r0 and parameter slot r8, then use call result.
  blocks[0].append<Bytecode::Instruction::Load>(0, 7);
  blocks[0].append<Bytecode::Instruction::Call>(
      10, 1, std::vector<Bytecode::Register>{0}, std::vector<Bytecode::Register>{8});
  blocks[0].append<Bytecode::Instruction::AddImmediate>(11, 10, 0);
  blocks[0].append<Bytecode::Instruction::Return>(11);

  // Callee body uses the same parameter register slot.
  blocks[1].append<Bytecode::Instruction::AddImmediate>(9, 8, 1);
  blocks[1].append<Bytecode::Instruction::Return>(9);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(blocks[0].instructions[1]->type() == Type::Call);
  REQUIRE(blocks[1].instructions[0]->type() == Type::AddImmediate);

  const auto &call =
      static_cast<const Bytecode::Instruction::Call &>(*blocks[0].instructions[1]);
  const auto &entry_add_imm = static_cast<const Bytecode::Instruction::AddImmediate &>(
      *blocks[0].instructions[2]);
  const auto &add_imm = static_cast<const Bytecode::Instruction::AddImmediate &>(
      *blocks[1].instructions[0]);
  const auto &callee_ret =
      static_cast<const Bytecode::Instruction::Return &>(*blocks[1].instructions[1]);
  const auto &entry_ret =
      static_cast<const Bytecode::Instruction::Return &>(*blocks[0].instructions[3]);

  // Compaction must preserve call/callee wiring consistently.
  REQUIRE(call.arg_registers == std::vector<Bytecode::Register>{0});
  REQUIRE(call.param_registers.size() == 1);
  REQUIRE(entry_add_imm.src == call.dst);
  REQUIRE(add_imm.src == call.param_registers[0]);
  REQUIRE(entry_ret.reg == entry_add_imm.dst);
  REQUIRE(callee_ret.reg == add_imm.dst);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 8);
}

