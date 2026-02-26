#include "test_optimizer_helpers.h"

// ============================================================
// Pass 4: Peephole Optimizations
// ============================================================

// AddImmediate r_tmp + Move r_var, r_tmp collapses to AddImmediate r_var
// when r_tmp is used only by the Move.  Mirrors the i++ increment pattern
// after copy propagation has already substituted the variable read.
TEST_CASE("peephole_folds_add_immediate_move") {
  // block 0: Load r0, 0; Jump @1
  // block 1: AddImmediate r2, r0, 1; Move r0, r2; Jump @1  (back edge → loop)
  // block 2: Return r0
  // r2 is used only by the Move → should be folded away.
  std::vector<Bytecode::BasicBlock> blocks(3);
  blocks[0].append<Bytecode::Instruction::Load>(0, 0);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::AddImmediate>(2, 0, 1);  // r2 = r0 + 1
  blocks[1].append<Bytecode::Instruction::Move>(0, 2);             // r0 = r2
  blocks[1].append<Bytecode::Instruction::Jump>(1);

  blocks[2].append<Bytecode::Instruction::Return>(0);

  BytecodeOptimizer opt;
  opt.peephole(blocks);

  // Peephole: AddImmediate r2, r0, 1 + Move r0, r2 → AddImmediate r0, r0, 1.
  // Block 1 must have exactly 2 instructions: AddImmediate + Jump.
  REQUIRE(blocks[1].instructions.size() == 2);
  REQUIRE(blocks[1].instructions[0]->type() == Type::AddImmediate);
  const auto &ai =
      static_cast<const Bytecode::Instruction::AddImmediate &>(*blocks[1].instructions[0]);
  REQUIRE(ai.dst == 0);    // was r2, now r0
  REQUIRE(ai.src == 0);    // source register unchanged
  REQUIRE(ai.value == 1);
}

TEST_CASE("peephole_folds_subtract_immediate_move") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 10);
  blocks[0].append<Bytecode::Instruction::SubtractImmediate>(2, 0, 3);  // r2 = r0 - 3
  blocks[0].append<Bytecode::Instruction::Move>(1, 2);                  // r1 = r2
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Return>(1);

  BytecodeOptimizer opt;
  opt.peephole(blocks);

  REQUIRE(blocks[0].instructions.size() == 3);
  REQUIRE(blocks[0].instructions[1]->type() == Type::SubtractImmediate);
  const auto &si = static_cast<const Bytecode::Instruction::SubtractImmediate &>(
      *blocks[0].instructions[1]);
  REQUIRE(si.dst == 1);
  REQUIRE(si.src == 0);
  REQUIRE(si.value == 3);
}

TEST_CASE("peephole_folds_compare_immediate_move") {
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 5);
  blocks[0].append<Bytecode::Instruction::LessThanImmediate>(2, 0, 8);  // r2 = r0 < 8
  blocks[0].append<Bytecode::Instruction::Move>(1, 2);                  // r1 = r2
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Return>(1);

  BytecodeOptimizer opt;
  opt.peephole(blocks);

  REQUIRE(blocks[0].instructions.size() == 3);
  REQUIRE(blocks[0].instructions[1]->type() == Type::LessThanImmediate);
  const auto &lti = static_cast<const Bytecode::Instruction::LessThanImmediate &>(
      *blocks[0].instructions[1]);
  REQUIRE(lti.dst == 1);
  REQUIRE(lti.lhs == 0);
  REQUIRE(lti.value == 8);
}

// Load r_tmp + Move r_var, r_tmp collapses to Load r_var when r_tmp is used
// only by the Move.  Mirrors the variable initializer pattern where the
// initializer loads into a fresh register and then Moves to the var slot.
TEST_CASE("peephole_folds_load_move") {
  // block 0: Load r0, 0; Move r1, r0; Jump @1
  // block 1: Return r1
  // r0 is used only by Move r1, r0 → collapse to Load r1, 0.
  std::vector<Bytecode::BasicBlock> blocks(2);
  blocks[0].append<Bytecode::Instruction::Load>(0, 0);
  blocks[0].append<Bytecode::Instruction::Move>(1, 0);   // r1 = r0
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::Return>(1);

  BytecodeOptimizer opt;
  opt.peephole(blocks);

  // Block 0 must have 2 instructions: Load r1, 0 and Jump @1.
  REQUIRE(blocks[0].instructions.size() == 2);
  REQUIRE(blocks[0].instructions[0]->type() == Type::Load);
  const auto &ld =
      static_cast<const Bytecode::Instruction::Load &>(*blocks[0].instructions[0]);
  REQUIRE(ld.dst == 1);
  REQUIRE(ld.value == 0);
}

// When the temporary register produced by AddImmediate is read by more than
// one instruction, the peephole must NOT fold — it would remove a use that
// is still needed.
TEST_CASE("peephole_not_applied_when_temp_has_multiple_uses") {
  // block 0: Load r0, 0; Jump @1
  // block 1: AddImmediate r2, r0, 1; Move r1, r2; JumpConditional r2, @2, @3
  //   r2 is used by both Move (as src) and JumpConditional (as cond) → count=2
  // blocks 2 & 3: Return r1
  std::vector<Bytecode::BasicBlock> blocks(4);
  blocks[0].append<Bytecode::Instruction::Load>(0, 0);
  blocks[0].append<Bytecode::Instruction::Jump>(1);

  blocks[1].append<Bytecode::Instruction::AddImmediate>(2, 0, 1);    // r2 = r0 + 1
  blocks[1].append<Bytecode::Instruction::Move>(1, 2);               // r1 = r2
  blocks[1].append<Bytecode::Instruction::JumpConditional>(2, 2, 3); // also reads r2

  blocks[2].append<Bytecode::Instruction::Return>(1);
  blocks[3].append<Bytecode::Instruction::Return>(1);

  BytecodeOptimizer opt;
  opt.peephole(blocks);

  // AddImmediate must still write to r2 — its dst must not have been changed.
  bool add_imm_dst_is_r2 = false;
  for (const auto &instr_ptr : blocks[1].instructions) {
    if (instr_ptr->type() == Type::AddImmediate) {
      const auto &ai =
          static_cast<const Bytecode::Instruction::AddImmediate &>(*instr_ptr);
      if (ai.dst == 2) add_imm_dst_is_r2 = true;
    }
  }
  REQUIRE(add_imm_dst_is_r2);
}

// Structural test: after peephole, the increment loop body contains
// an in-place AddImmediate immediately followed by a back-edge Jump.
TEST_CASE("peephole_increment_loop_body_has_in_place_add_before_jump") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), lit(10)), std::move(while_body)));
  body->append(ret(var("i")));

  BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  // Canonicalize loop increment shape to expose the immediate-op + Move pair
  // that peephole folds.
  opt.copy_propagation(gen.blocks());
  opt.peephole(gen.blocks());

  // Find any AddImmediate that is immediately followed by Jump and writes
  // to its own source (in-place increment).
  bool found_optimal_body = false;
  for (const auto &blk : gen.blocks()) {
    for (size_t i = 0; i + 1 < blk.instructions.size(); ++i) {
      if (blk.instructions[i]->type() != Type::AddImmediate ||
          blk.instructions[i + 1]->type() != Type::Jump) {
        continue;
      }
      const auto &ai =
          static_cast<const Bytecode::Instruction::AddImmediate &>(*blk.instructions[i]);
      if (ai.dst == ai.src) {
        found_optimal_body = true;
        break;
      }
    }
    if (found_optimal_body) {
      break;
    }
  }
  REQUIRE(found_optimal_body);
}

// Correctness: peephole must preserve the loop's computed value.
TEST_CASE("peephole_correctness_increment_loop") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), lit(7)), std::move(while_body)));
  body->append(ret(var("i")));

  BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.copy_propagation(gen.blocks());
  opt.peephole(gen.blocks());

  BytecodeInterpreter interp;
  REQUIRE(interp.interpret(gen.blocks()) == 7);
}

// Idempotency: running peephole() a second time must not
// change the instruction count.
TEST_CASE("peephole_is_idempotent") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), lit(5)), std::move(while_body)));
  body->append(ret(var("i")));

  BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.copy_propagation(gen.blocks());
  opt.peephole(gen.blocks());
  const size_t after_first = total_instr_count(gen.blocks());
  opt.peephole(gen.blocks());
  const size_t after_second = total_instr_count(gen.blocks());

  REQUIRE(after_second == after_first);
}

// Running peephole a second time on already-peephole-optimized code must not
// change the instruction count (idempotency).
TEST_CASE("peephole_is_idempotent_second_program") {
  auto body = std::make_unique<Ast::Block>();
  body->append(decl("i", lit(0)));
  auto while_body = std::make_unique<Ast::Block>();
  while_body->append(inc("i"));
  body->append(while_loop(lt(var("i"), lit(3)), std::move(while_body)));
  body->append(ret(var("i")));

  BytecodeGenerator gen;
  gen.visit_block(*body);
  gen.finalize();

  BytecodeOptimizer opt;
  opt.copy_propagation(gen.blocks());
  opt.peephole(gen.blocks());
  const size_t after_first  = total_instr_count(gen.blocks());
  opt.peephole(gen.blocks());
  const size_t after_second = total_instr_count(gen.blocks());

  REQUIRE(after_second == after_first);
}
