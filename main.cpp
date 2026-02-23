#include "ast.h"
#include "bytecode.h"

#include <initializer_list>

using namespace kai::ast;

std::unique_ptr<Ast> lit(int v) { return std::make_unique<Ast::Literal>(v); }

std::unique_ptr<Ast> var(const char *name) { return std::make_unique<Ast::Variable>(name); }

std::unique_ptr<Ast::Variable> var_ref(const char *name) { return std::make_unique<Ast::Variable>(name); }

std::unique_ptr<Ast::Add> add(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::Add>(std::move(left), std::move(right));
}

std::unique_ptr<Ast::Multiply> mul(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::Multiply>(std::move(left), std::move(right));
}

std::unique_ptr<Ast::Divide> div(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::Divide>(std::move(left), std::move(right));
}

std::unique_ptr<Ast::Subtract> sub(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::Subtract>(std::move(left), std::move(right));
}

std::unique_ptr<Ast::LessThan> lt(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::LessThan>(std::move(left), std::move(right));
}

std::unique_ptr<Ast::VariableDeclaration> decl(const char *name, std::unique_ptr<Ast> initializer) {
  return std::make_unique<Ast::VariableDeclaration>(name, std::move(initializer));
}

std::unique_ptr<Ast::Assignment> assign(const char *name, std::unique_ptr<Ast> value) {
  return std::make_unique<Ast::Assignment>(name, std::move(value));
}

std::unique_ptr<Ast::Increment> inc(const char *name) { return std::make_unique<Ast::Increment>(var_ref(name)); }

std::unique_ptr<Ast::Return> ret(std::unique_ptr<Ast> value) {
  return std::make_unique<Ast::Return>(std::move(value));
}

std::unique_ptr<Ast::FunctionCall> call(const char *name) {
  return std::make_unique<Ast::FunctionCall>(name);
}

std::unique_ptr<Ast::While> while_loop(std::unique_ptr<Ast::LessThan> condition,
                                       std::unique_ptr<Ast::Block> body) {
  return std::make_unique<Ast::While>(std::move(condition), std::move(body));
}

std::unique_ptr<Ast::IfElse> if_else(std::unique_ptr<Ast::LessThan> condition,
                                     std::unique_ptr<Ast::Block> body,
                                     std::unique_ptr<Ast::Block> else_body) {
  return std::make_unique<Ast::IfElse>(std::move(condition), std::move(body),
                                       std::move(else_body));
}

std::unique_ptr<Ast::ArrayLiteral> arr(std::initializer_list<int> values) {
  std::vector<std::unique_ptr<Ast>> elements;
  elements.reserve(values.size());
  for (int value : values) {
    elements.emplace_back(lit(value));
  }
  return std::make_unique<Ast::ArrayLiteral>(std::move(elements));
}

std::unique_ptr<Ast::Index> idx(std::unique_ptr<Ast> array_expr,
                                std::unique_ptr<Ast> index_expr) {
  return std::make_unique<Ast::Index>(std::move(array_expr), std::move(index_expr));
}

Ast::Block array_indexing_minimal_example() {
  auto decl_body = std::make_unique<Ast::Block>();
  decl_body->append(decl("values", arr({11, 22, 33})));
  decl_body->append(ret(idx(var("values"), lit(1))));
  return std::move(*decl_body);
}

Ast::Block recursion_function_calls_minimal_example() {
  auto program = std::make_unique<Ast::Block>();
  program->append(decl("n", lit(5)));

  auto fact_body = std::make_unique<Ast::Block>();
  auto base_case = std::make_unique<Ast::Block>();
  base_case->append(ret(lit(1)));

  auto recursive_case = std::make_unique<Ast::Block>();
  recursive_case->append(decl("saved_n", var("n")));
  recursive_case->append(assign("n", sub(var("n"), lit(1))));
  recursive_case->append(ret(mul(var("saved_n"), call("fact"))));

  fact_body->append(if_else(lt(var("n"), lit(2)), std::move(base_case),
                            std::move(recursive_case)));
  program->append(std::make_unique<Ast::FunctionDeclaration>("fact", std::move(fact_body)));
  program->append(ret(call("fact")));
  return std::move(*program);
}

void test_quicksort_minimal_example() {
  auto quicksort_two = [] {
    auto decl_body = std::make_unique<Ast::Block>();
    auto append_conditional_swap = [&](const char *lhs, const char *rhs, const char *tmp) {
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
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 12);
}

void test_quicksort() {
  auto quicksort_three = [] {
    auto decl_body = std::make_unique<Ast::Block>();
    auto append_conditional_swap = [&](const char *lhs, const char *rhs, const char *tmp) {
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

    // Specialized 3-element quicksort: partitioning fully unrolled as
    // compare-and-swap steps.
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
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 123);
}

void test_fibonacci() {
  auto fib = [] {
    auto decl_body = std::make_unique<Ast::Block>();

    decl_body->append(decl("n", lit(10)));
    decl_body->append(decl("t1", lit(0)));
    decl_body->append(decl("t2", lit(1)));
    decl_body->append(decl("t3", lit(0)));

    decl_body->append(decl("i", lit(0)));
    auto block = std::make_unique<Ast::Block>();
    block->append(assign("t3", add(var("t1"), var("t2"))));
    block->append(assign("t1", var("t2")));
    block->append(assign("t2", var("t3")));
    block->append(inc("i"));
    decl_body->append(while_loop(lt(var("i"), var("n")), std::move(block)));

    decl_body->append(ret(var("t1")));
    return std::move(*decl_body);
  }();
  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(fib);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 55);
}

void test_factorial() {
  auto factorial = [] {
    auto decl_body = std::make_unique<Ast::Block>();
    decl_body->append(decl("n", lit(5)));
    decl_body->append(decl("result", lit(1)));
    decl_body->append(decl("i", lit(1)));

    auto block = std::make_unique<Ast::Block>();
    block->append(assign("result", mul(var("result"), var("i"))));
    block->append(inc("i"));
    decl_body->append(while_loop(lt(var("i"), add(var("n"), lit(1))), std::move(block)));
    decl_body->append(ret(var("result")));
    return std::move(*decl_body);
  }();
  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(factorial);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 120);
}

void test_function_declaration() {
  auto program = [] {
    auto decl_body = std::make_unique<Ast::Block>();
    auto sum_body = std::make_unique<Ast::Block>();
    sum_body->append(decl("a", lit(4)));
    sum_body->append(decl("b", lit(2)));
    sum_body->append(ret(add(var("a"), var("b"))));
    decl_body->append(std::make_unique<Ast::FunctionDeclaration>("sum", std::move(sum_body)));
    decl_body->append(ret(call("sum")));
    return std::move(*decl_body);
  }();
  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(program);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 6);
}

void test_function_call_recursion() {
  auto program = recursion_function_calls_minimal_example();

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(program);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 120);
}

void test_bug_missing_function_block() {
  // let dummy = 0;
  // function f() { return 42; }
  // return f();
  auto program = std::make_unique<Ast::Block>();
  program->append(decl("dummy", lit(0)));

  auto f_body = std::make_unique<Ast::Block>();
  f_body->append(ret(lit(42)));
  program->append(std::make_unique<Ast::FunctionDeclaration>("f", std::move(f_body)));

  program->append(ret(call("f")));

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(*program);
  gen.finalize();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 42);
}

void test_bug_implicit_fallthrough() {
  // function f() { let a = 1; /* no return */ }
  // function g() { return 50; }
  // return f();
  auto program = std::make_unique<Ast::Block>();

  auto f_body = std::make_unique<Ast::Block>();
  f_body->append(decl("a", lit(1)));
  program->append(std::make_unique<Ast::FunctionDeclaration>("f", std::move(f_body)));

  auto g_body = std::make_unique<Ast::Block>();
  g_body->append(ret(lit(50)));
  program->append(std::make_unique<Ast::FunctionDeclaration>("g", std::move(g_body)));

  program->append(ret(call("f")));

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(*program);
  gen.finalize();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 0);
}

void test_if_else() {
  auto max = [] {
    auto decl_body = std::make_unique<Ast::Block>();
    decl_body->append(decl("a", lit(4)));
    decl_body->append(decl("b", lit(7)));
    decl_body->append(decl("max", lit(0)));

    auto if_body = std::make_unique<Ast::Block>();
    if_body->append(assign("max", var("a")));

    auto else_body = std::make_unique<Ast::Block>();
    else_body->append(assign("max", var("b")));

    decl_body->append(
        if_else(lt(var("b"), var("a")), std::move(if_body), std::move(else_body)));
    decl_body->append(ret(var("max")));
    return std::move(*decl_body);
  }();
  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(max);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 7);
}

void test_subtract() {
  auto difference = [] {
    auto decl_body = std::make_unique<Ast::Block>();
    decl_body->append(decl("a", lit(20)));
    decl_body->append(decl("b", lit(8)));
    decl_body->append(ret(sub(var("a"), var("b"))));
    return std::move(*decl_body);
  }();
  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(difference);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 12);
}

void test_divide() {
  auto quotient = [] {
    auto decl_body = std::make_unique<Ast::Block>();
    decl_body->append(decl("a", lit(20)));
    decl_body->append(decl("b", lit(5)));
    decl_body->append(ret(div(var("a"), var("b"))));
    return std::move(*decl_body);
  }();
  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(quotient);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 4);
}

void test_array_indexing() {
  auto program = array_indexing_minimal_example();

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(program);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 22);
}

void test_top_level_block_example() {
  Ast::Block program;
  program.append(ret(lit(42)));

  kai::bytecode::BytecodeGenerator gen;
  gen.visit(program);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 42);
}

void test_nested_if_inside_while() {
  auto program = [] {
    auto decl_body = std::make_unique<Ast::Block>();
    decl_body->append(decl("n", lit(3)));
    decl_body->append(decl("i", lit(0)));
    decl_body->append(decl("x", lit(0)));

    auto while_body = std::make_unique<Ast::Block>();
    auto if_body = std::make_unique<Ast::Block>();
    if_body->append(assign("x", add(var("x"), lit(10))));
    auto else_body = std::make_unique<Ast::Block>();
    else_body->append(assign("x", add(var("x"), lit(1))));
    while_body->append(if_else(lt(var("i"), lit(1)), std::move(if_body), std::move(else_body)));
    while_body->append(inc("i"));

    decl_body->append(while_loop(lt(var("i"), var("n")), std::move(while_body)));
    decl_body->append(ret(var("x")));
    return std::move(*decl_body);
  }();

  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(program);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 12);
}

void test_interpreter_reuse_across_programs() {
  auto with_loop = [] {
    auto decl_body = std::make_unique<Ast::Block>();
    decl_body->append(decl("i", lit(0)));

    auto while_body = std::make_unique<Ast::Block>();
    while_body->append(inc("i"));
    decl_body->append(while_loop(lt(var("i"), lit(1)), std::move(while_body)));
    decl_body->append(ret(var("i")));
    return std::move(*decl_body);
  }();

  Ast::Block simple;
  simple.append(ret(lit(7)));

  kai::bytecode::BytecodeGenerator gen_with_loop;
  gen_with_loop.visit_block(with_loop);
  gen_with_loop.finalize();

  kai::bytecode::BytecodeGenerator gen_simple;
  gen_simple.visit(simple);
  gen_simple.finalize();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen_with_loop.blocks()) == 1);
  assert(interp.interpret(gen_simple.blocks()) == 7);
}

int main() {
  test_quicksort_minimal_example();
  test_quicksort();
  test_fibonacci();
  test_factorial();
  test_function_declaration();
  test_function_call_recursion();
  test_bug_missing_function_block();
  test_bug_implicit_fallthrough();
  test_if_else();
  test_subtract();
  test_divide();
  test_array_indexing();
  test_top_level_block_example();
  test_nested_if_inside_while();
  test_interpreter_reuse_across_programs();
  return 0;
}
