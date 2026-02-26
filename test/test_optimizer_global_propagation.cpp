#include "test_optimizer_helpers.h"

// ============================================================
// Pass 1: Global Copy + Constant Propagation
// ============================================================

TEST_CASE("global_copy_propagation_rewrites_cross_block_sources") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 41);
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::AddImmediate>(2, 1, 1);
  blocks[1].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE_FALSE(has_instruction_type(blocks, Type::Move));

  const Bytecode::Instruction::Load *load = nullptr;
  const Bytecode::Instruction::AddImmediate *add_imm = nullptr;
  for (const auto &block : blocks) {
    for (const auto &instr : block.instructions) {
      if (instr->type() == Type::Load && load == nullptr) {
        load = &static_cast<const Bytecode::Instruction::Load &>(*instr);
      }
      if (instr->type() == Type::AddImmediate && add_imm == nullptr) {
        add_imm = &static_cast<const Bytecode::Instruction::AddImmediate &>(*instr);
      }
    }
  }

  REQUIRE(load != nullptr);
  REQUIRE(add_imm != nullptr);
  REQUIRE(add_imm->src == load->dst);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 42);
}

TEST_CASE("global_constant_propagation_simplifies_cross_block_branch") {
  std::vector<Bytecode::BasicBlock> blocks(4);
  blocks[0].append<Bytecode::Instruction::Load>(0, 0);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::JumpConditional>(0, 2, 3);

  blocks[2].append<Bytecode::Instruction::Load>(1, 111);
  blocks[2].append<Bytecode::Instruction::Return>(1);

  blocks[3].append<Bytecode::Instruction::Load>(2, 222);
  blocks[3].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE_FALSE(has_instruction_type(blocks, Type::JumpConditional));

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 222);
}

TEST_CASE("global_propagation_join_requires_fact_on_all_predecessors") {
  std::vector<Bytecode::BasicBlock> blocks(4);
  blocks[0].append<Bytecode::Instruction::Equal>(9, 6, 7);
  blocks[0].append<Bytecode::Instruction::JumpConditional>(9, 1, 2);

  blocks[1].append<Bytecode::Instruction::Move>(5, 1);
  blocks[1].append<Bytecode::Instruction::Jump>(3);

  blocks[2].append<Bytecode::Instruction::Move>(5, 2);
  blocks[2].append<Bytecode::Instruction::Jump>(3);

  blocks[3].append<Bytecode::Instruction::Return>(5);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  std::vector<const Bytecode::Instruction::Move *> moves;
  const Bytecode::Instruction::Return *ret = nullptr;
  for (const auto &block : blocks) {
    for (const auto &instr : block.instructions) {
      if (instr->type() == Type::Move) {
        moves.push_back(&static_cast<const Bytecode::Instruction::Move &>(*instr));
      }
      if (instr->type() == Type::Return) {
        ret = &static_cast<const Bytecode::Instruction::Return &>(*instr);
      }
    }
  }

  REQUIRE(moves.size() == 2);
  REQUIRE(ret != nullptr);
  REQUIRE(moves[0]->dst == moves[1]->dst);
  REQUIRE(ret->reg == moves[0]->dst);
}
TEST_CASE("copy_propagation_rewrites_return_through_cross_block_alias") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(3, 99);
  blocks[0].append<Bytecode::Instruction::Move>(5, 3);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Return>(5);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  const Bytecode::Instruction::Load *load = nullptr;
  const Bytecode::Instruction::Return *ret = nullptr;
  for (const auto &block : blocks) {
    for (const auto &instr : block.instructions) {
      if (instr->type() == Type::Load && load == nullptr) {
        load = &static_cast<const Bytecode::Instruction::Load &>(*instr);
      }
      if (instr->type() == Type::Return && ret == nullptr) {
        ret = &static_cast<const Bytecode::Instruction::Return &>(*instr);
      }
    }
  }

  REQUIRE(ret != nullptr);
  REQUIRE(load != nullptr);
  REQUIRE(ret->reg == load->dst);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 99);
}

TEST_CASE("copy_propagation_rewrites_return_through_multi_hop_cross_block_alias") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(1, 7);
  blocks[0].append<Bytecode::Instruction::Move>(2, 1);
  blocks[0].append<Bytecode::Instruction::Move>(3, 2);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Return>(3);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  const Bytecode::Instruction::Load *load = nullptr;
  const Bytecode::Instruction::Return *ret = nullptr;
  for (const auto &block : blocks) {
    for (const auto &instr : block.instructions) {
      if (instr->type() == Type::Load && load == nullptr) {
        load = &static_cast<const Bytecode::Instruction::Load &>(*instr);
      }
      if (instr->type() == Type::Return && ret == nullptr) {
        ret = &static_cast<const Bytecode::Instruction::Return &>(*instr);
      }
    }
  }

  REQUIRE(ret != nullptr);
  REQUIRE(load != nullptr);
  REQUIRE(ret->reg == load->dst);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 7);
}

TEST_CASE("global_copy_propagation_handles_euler_style_cross_block_aliases") {
  std::vector<Bytecode::BasicBlock> blocks(3);
  blocks[0].append<Bytecode::Instruction::Load>(1, 10);
  blocks[0].append<Bytecode::Instruction::Load>(3, 6);
  blocks[0].append<Bytecode::Instruction::Move>(8, 3);
  blocks[0].append<Bytecode::Instruction::Move>(11, 3);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Modulo>(9, 1, 8);
  blocks[1].append<Bytecode::Instruction::Jump>(2);

  blocks[2].append<Bytecode::Instruction::Divide>(10, 1, 11);
  blocks[2].append<Bytecode::Instruction::Add>(12, 9, 10);
  blocks[2].append<Bytecode::Instruction::Return>(12);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE_FALSE(has_instruction_type(blocks, Type::Move));

  const Bytecode::Instruction::Modulo *modulo = nullptr;
  const Bytecode::Instruction::Divide *divide = nullptr;
  for (const auto &block : blocks) {
    for (const auto &instr : block.instructions) {
      if (instr->type() == Type::Modulo && modulo == nullptr) {
        modulo = &static_cast<const Bytecode::Instruction::Modulo &>(*instr);
      }
      if (instr->type() == Type::Divide && divide == nullptr) {
        divide = &static_cast<const Bytecode::Instruction::Divide &>(*instr);
      }
    }
  }

  REQUIRE(modulo != nullptr);
  REQUIRE(divide != nullptr);
  REQUIRE(modulo->src2 == divide->src2);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 5);
}
