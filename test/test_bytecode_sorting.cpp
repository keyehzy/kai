#include "catch.hpp"
#include "test_bytecode_cases.h"

TEST_CASE("test_bytecode_quicksort_minimal_example") {
    auto quicksort_two = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      auto append_conditional_swap =
          [&](const char *lhs, const char *rhs, const char *tmp) {
            auto swap_body = std::make_unique<Ast::Block>();
            swap_body->append(assign(tmp, var(lhs)));
            swap_body->append(assign(lhs, var(rhs)));
            swap_body->append(assign(rhs, var(tmp)));
            decl_body->append(if_else(lt(var(rhs), var(lhs)), std::move(swap_body),
                                      std::make_unique<Ast::Block>()));
          };

      decl_body->append(decl("a", lit(2)));
      decl_body->append(decl("b", lit(1)));
      decl_body->append(decl("tmp", lit(0)));

      append_conditional_swap("a", "b", "tmp");
      decl_body->append(ret(add(mul(var("a"), lit(10)), var("b"))));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(quicksort_two);
    gen.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 12);
}

TEST_CASE("test_bytecode_quicksort") {
    auto quicksort_three = [] {
      auto decl_body = std::make_unique<Ast::Block>();
      auto append_conditional_swap =
          [&](const char *lhs, const char *rhs, const char *tmp) {
            auto swap_body = std::make_unique<Ast::Block>();
            swap_body->append(assign(tmp, var(lhs)));
            swap_body->append(assign(lhs, var(rhs)));
            swap_body->append(assign(rhs, var(tmp)));
            decl_body->append(if_else(lt(var(rhs), var(lhs)), std::move(swap_body),
                                      std::make_unique<Ast::Block>()));
          };

      decl_body->append(decl("a", lit(3)));
      decl_body->append(decl("b", lit(1)));
      decl_body->append(decl("c", lit(2)));
      decl_body->append(decl("tmp", lit(0)));

      append_conditional_swap("a", "b", "tmp");
      append_conditional_swap("b", "c", "tmp");
      append_conditional_swap("a", "b", "tmp");

      decl_body->append(
          ret(add(add(mul(var("a"), lit(100)), mul(var("b"), lit(10))), var("c"))));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(quicksort_three);
    gen.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 123);
}

TEST_CASE("test_bytecode_quicksort_complete") {
    auto quicksort_five = [] {
      auto decl_body = std::make_unique<Ast::Block>();

      decl_body->append(decl("values", arr({4, 1, 5, 2, 3})));
      decl_body->append(decl("pivot_idx", lit(4)));
      decl_body->append(decl("pivot", idx(var("values"), var("pivot_idx"))));
      decl_body->append(decl("i", lit(0)));
      decl_body->append(decl("j", lit(0)));
      decl_body->append(decl("tmp", lit(0)));

      auto partition_body = std::make_unique<Ast::Block>();
      auto move_left_body = std::make_unique<Ast::Block>();
      move_left_body->append(assign("tmp", idx(var("values"), var("i"))));
      move_left_body->append(
          idx_assign(var("values"), var("i"), idx(var("values"), var("j"))));
      move_left_body->append(idx_assign(var("values"), var("j"), var("tmp")));
      move_left_body->append(inc("i"));
      partition_body->append(if_else(lt(idx(var("values"), var("j")), var("pivot")),
                                     std::move(move_left_body),
                                     std::make_unique<Ast::Block>()));
      partition_body->append(inc("j"));
      decl_body->append(
          while_loop(lt(var("j"), var("pivot_idx")), std::move(partition_body)));

      decl_body->append(assign("tmp", idx(var("values"), var("i"))));
      decl_body->append(
          idx_assign(var("values"), var("i"), idx(var("values"), var("pivot_idx"))));
      decl_body->append(idx_assign(var("values"), var("pivot_idx"), var("tmp")));

      auto left_swap_body = std::make_unique<Ast::Block>();
      left_swap_body->append(assign("tmp", idx(var("values"), lit(0))));
      left_swap_body->append(
          idx_assign(var("values"), lit(0), idx(var("values"), lit(1))));
      left_swap_body->append(idx_assign(var("values"), lit(1), var("tmp")));
      auto left_partition_body = std::make_unique<Ast::Block>();
      left_partition_body->append(
          if_else(lt(idx(var("values"), lit(1)), idx(var("values"), lit(0))),
                  std::move(left_swap_body), std::make_unique<Ast::Block>()));
      decl_body->append(if_else(lt(lit(1), var("i")), std::move(left_partition_body),
                                std::make_unique<Ast::Block>()));

      decl_body->append(decl("right_lo", add(var("i"), lit(1))));
      auto right_swap_body = std::make_unique<Ast::Block>();
      right_swap_body->append(assign("tmp", idx(var("values"), var("right_lo"))));
      right_swap_body->append(
          idx_assign(var("values"), var("right_lo"), idx(var("values"), lit(4))));
      right_swap_body->append(idx_assign(var("values"), lit(4), var("tmp")));
      auto right_partition_body = std::make_unique<Ast::Block>();
      right_partition_body->append(
          if_else(lt(idx(var("values"), lit(4)), idx(var("values"), var("right_lo"))),
                  std::move(right_swap_body), std::make_unique<Ast::Block>()));
      decl_body->append(if_else(lt(var("right_lo"), lit(4)),
                                std::move(right_partition_body),
                                std::make_unique<Ast::Block>()));

      decl_body->append(ret(add(add(add(add(mul(idx(var("values"), lit(0)), lit(10000)),
                                           mul(idx(var("values"), lit(1)), lit(1000))),
                                       mul(idx(var("values"), lit(2)), lit(100))),
                                   mul(idx(var("values"), lit(3)), lit(10))),
                               idx(var("values"), lit(4)))));
      return std::move(*decl_body);
    }();

    kai::bytecode::BytecodeGenerator gen;
    gen.visit_block(quicksort_five);
    gen.finalize();

    kai::bytecode::BytecodeInterpreter interp;
    REQUIRE(interp.interpret(gen.blocks()) == 12345);
}
