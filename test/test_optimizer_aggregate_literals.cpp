#include "test_optimizer_helpers.h"

// ============================================================
// Pass 1.5: Aggregate Literal Folding
// ============================================================

TEST_CASE("folds_array_create_to_array_literal_create_when_all_inputs_are_constant_loads") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(10, 3);
  blocks[0].append<Bytecode::Instruction::Load>(12, 4);
  blocks[0].append<Bytecode::Instruction::ArrayCreate>(
      20, std::vector<Bytecode::Register>{10, 12});
  blocks[0].append<Bytecode::Instruction::Return>(20);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE_FALSE(has_instruction_type(blocks, Type::ArrayCreate));
  REQUIRE(has_instruction_type(blocks, Type::ArrayLiteralCreate));

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::ArrayLiteralCreate);
  const auto &alc = static_cast<const Bytecode::Instruction::ArrayLiteralCreate &>(
      *blocks[0].instructions[0]);
  REQUIRE(alc.elements == std::vector<Bytecode::Value>{3, 4});
}

TEST_CASE("does_not_fold_array_create_when_any_input_is_not_constant_load") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 5);
  blocks[0].append<Bytecode::Instruction::Load>(1, 7);
  blocks[0].append<Bytecode::Instruction::Add>(2, 0, 1);  // non-Load producer
  blocks[0].append<Bytecode::Instruction::ArrayCreate>(
      3, std::vector<Bytecode::Register>{2, 1});
  blocks[0].append<Bytecode::Instruction::Return>(3);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(has_instruction_type(blocks, Type::ArrayCreate));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::ArrayLiteralCreate));
}

TEST_CASE("folds_array_load_with_constant_index_to_array_load_immediate") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::ArrayLiteralCreate>(
      0, std::vector<Bytecode::Value>{10, 20, 30});
  blocks[0].append<Bytecode::Instruction::Load>(1, 1);
  blocks[0].append<Bytecode::Instruction::ArrayLoad>(2, 0, 1);
  blocks[0].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE_FALSE(has_instruction_type(blocks, Type::ArrayLoad));
  REQUIRE(has_instruction_type(blocks, Type::ArrayLoadImmediate));
  REQUIRE(blocks[0].instructions.size() == 3);

  REQUIRE(blocks[0].instructions[1]->type() == Type::ArrayLoadImmediate);
  const auto &ali = static_cast<const Bytecode::Instruction::ArrayLoadImmediate &>(
      *blocks[0].instructions[1]);
  REQUIRE(ali.array == 0);
  REQUIRE(ali.index == 1);

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(blocks) == 20);
}

TEST_CASE("folds_struct_create_to_struct_literal_create_when_all_inputs_are_constant_loads") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 9);
  blocks[0].append<Bytecode::Instruction::Load>(1, 1);
  blocks[0].append<Bytecode::Instruction::StructCreate>(
      2, std::vector<std::pair<std::string, Bytecode::Register>>{{"x", 0}, {"y", 1}});
  blocks[0].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE_FALSE(has_instruction_type(blocks, Type::StructCreate));
  REQUIRE(has_instruction_type(blocks, Type::StructLiteralCreate));

  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::StructLiteralCreate);
  const auto &slc = static_cast<const Bytecode::Instruction::StructLiteralCreate &>(
      *blocks[0].instructions[0]);
  REQUIRE(slc.fields.size() == 2);
  REQUIRE(slc.fields[0].first == "x");
  REQUIRE(slc.fields[0].second == 9);
  REQUIRE(slc.fields[1].first == "y");
  REQUIRE(slc.fields[1].second == 1);
}

TEST_CASE("does_not_fold_struct_create_when_any_input_is_not_constant_load") {
  std::vector<Bytecode::BasicBlock> blocks(1);
  blocks[0].append<Bytecode::Instruction::Load>(0, 8);
  blocks[0].append<Bytecode::Instruction::Add>(1, 0, 0);  // non-Load producer
  blocks[0].append<Bytecode::Instruction::StructCreate>(
      2, std::vector<std::pair<std::string, Bytecode::Register>>{{"x", 1}});
  blocks[0].append<Bytecode::Instruction::Return>(2);

  BytecodeOptimizer opt;
  opt.optimize(blocks);

  REQUIRE(has_instruction_type(blocks, Type::StructCreate));
  REQUIRE_FALSE(has_instruction_type(blocks, Type::StructLiteralCreate));
}


