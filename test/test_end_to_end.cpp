#include "../ast.h"
#include "../bytecode.h"
#include "../catch.hpp"
#include "../parser.h"

#include <iostream>

TEST_CASE("test_parser_expression_end_to_end_literal_42") {
  kai::Parser parser("42");
  std::unique_ptr<kai::ast::Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 42);

  kai::ast::Ast::Block program;
  program.append(std::make_unique<kai::ast::Ast::Return>(std::move(parsed_expression)));

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 42);
}

TEST_CASE("test_parser_expression_end_to_end_modulo_minimal") {
  kai::Parser parser("20 % 6 + 1");
  std::unique_ptr<kai::ast::Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 3);

  kai::ast::Ast::Block program;
  program.append(std::make_unique<kai::ast::Ast::Return>(std::move(parsed_expression)));

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 3);
}

TEST_CASE("test_parser_expression_end_to_end_equality_minimal") {
  kai::Parser parser("20 % 6 == 2");
  std::unique_ptr<kai::ast::Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  kai::ast::Ast::Block program;
  program.append(std::make_unique<kai::ast::Ast::Return>(std::move(parsed_expression)));

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_parser_expression_end_to_end_not_equal_minimal") {
  kai::Parser parser("20 % 6 != 3");
  std::unique_ptr<kai::ast::Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  kai::ast::Ast::Block program;
  program.append(std::make_unique<kai::ast::Ast::Return>(std::move(parsed_expression)));

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_parser_expression_end_to_end_less_than") {
  kai::Parser parser("1 < 2");
  std::unique_ptr<kai::ast::Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  kai::ast::Ast::Block program;
  program.append(std::make_unique<kai::ast::Ast::Return>(std::move(parsed_expression)));

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_parser_expression_end_to_end_greater_than") {
  kai::Parser parser("3 > 2");
  std::unique_ptr<kai::ast::Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  kai::ast::Ast::Block program;
  program.append(std::make_unique<kai::ast::Ast::Return>(std::move(parsed_expression)));

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_parser_expression_end_to_end_equal") {
  kai::Parser parser("17+3 == 20");
  std::unique_ptr<kai::ast::Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  kai::ast::Ast::Block program;
  program.append(std::make_unique<kai::ast::Ast::Return>(std::move(parsed_expression)));

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_parser_expression_end_to_end_array_index_assignment") {
  kai::Parser parser("[7, 8, 9][1]");
  std::unique_ptr<kai::ast::Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 8);

  kai::ast::Ast::Block program;
  program.append(std::make_unique<kai::ast::Ast::Return>(std::move(parsed_expression)));

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 8);
}

TEST_CASE("test_parser_expression_end_to_end_less_than_or_equal_minimal") {
  kai::Parser parser("2 <= 2");
  std::unique_ptr<kai::ast::Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  kai::ast::Ast::Block program;
  program.append(std::make_unique<kai::ast::Ast::Return>(std::move(parsed_expression)));

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_program_end_to_end_count_to_ten_while_loop") {
  kai::Parser parser(R"(
let i = 0;
while (i < 10) {
  i++;
}
return i;
)");
  std::unique_ptr<kai::ast::Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 10);

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 10);
}

TEST_CASE("test_program_end_to_end_count_down_from_ten_to_one_while_loop") {
  kai::Parser parser(R"(
let i = 10;
while (i > 1) {
  i = i - 1;
}
return i;
)");
  std::unique_ptr<kai::ast::Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 1);

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_program_end_to_end_if_without_else") {
  kai::Parser parser(R"(
let x = 0;
if (2 < 1) {
  x = 99;
}
if (1 < 2) {
  x = 42;
}
return x;
)");
  std::unique_ptr<kai::ast::Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 42);

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 42);
}

TEST_CASE("test_program_end_to_end_return_exits_program_early") {
  kai::Parser parser(R"(
let x = 7;
return x;
x = 99;
)");
  std::unique_ptr<kai::ast::Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 7);

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 7);
}

TEST_CASE("test_program_end_to_end_return_exits_function_early") {
  kai::Parser parser(R"(
fn early() {
  let x = 1;
  return x;
  x = 2;
}
return early();
)");
  std::unique_ptr<kai::ast::Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 1);

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_program_end_to_end_return_exits_loop_and_function_early") {
  kai::Parser parser(R"(
fn find_three() {
  let i_val = 0;
  while (i_val < 10) {
    if (i_val == 3) {
      return i_val;
    }
    i_val++;
  }
  return 99;
}
return find_three();
)");
  std::unique_ptr<kai::ast::Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 3);

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 3);
}

TEST_CASE("test_program_end_to_end_function_parameters") {
  kai::Parser parser(R"(
fn add(a, b) {
  return a + b;
}
return add(40, 2);
)");
  std::unique_ptr<kai::ast::Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 42);

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 42);
}

TEST_CASE("test_program_end_to_end_function_recursion_fibonacci") {
  kai::Parser parser(R"(
fn fib(n) {
  if (n < 2) {
    return n;
  } else {
    return fib(n - 1) + fib(n - 2);
  }
}
return fib(10);
)");
  std::unique_ptr<kai::ast::Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 55);

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 55);
}

TEST_CASE("test_program_end_to_end_function_recursion_fibonacci_minimal") {
  kai::Parser parser(R"(
fn fib(n) {
  if (n < 2) {
    return n;
  } else {
    return fib(n - 1) + fib(n - 2);
  }
}
return fib(2);
)");
  std::unique_ptr<kai::ast::Ast::Block> program = parser.parse_program();
  program->dump(std::cout);

  REQUIRE(program != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 1);

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_program_end_to_end_quicksort") {
  kai::Parser parser(R"(
fn partition(values, low, high) {
  let pivot = values[high];
  let i = low;
  let j = low;
  let tmp = 0;
  while (j <= high - 1) {
    if (values[j] < pivot) {
      tmp = values[i];
      values[i] = values[j];
      values[j] = tmp;
      i++;
    }
    j++;
  }
  tmp = values[i];
  values[i] = values[high];
  values[high] = tmp;
  return i;
}

fn quicksort(values, low, high) {
  if (low < high) {
    let p = partition(values, low, high);
    if (low < p) {
      quicksort(values, low, p - 1);
    }
    if (p < high) {
      quicksort(values, p + 1, high);
    }
  }
  return 0;
}

let values = [4, 1, 5, 2, 3];
quicksort(values, 0, 4);
return values[0] * 10000 + values[1] * 1000 + values[2] * 100 + values[3] * 10 +
       values[4];
)");
  std::unique_ptr<kai::ast::Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 12345);

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 12345);
}

TEST_CASE("test_program_end_to_end_structs_minimal") {
  kai::Parser parser(R"(
let point = struct { x: 40, y: 2 };
return point.x + point.y;
)");
  std::unique_ptr<kai::ast::Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  kai::ast::AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 42);

  kai::bytecode::BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  kai::bytecode::BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 42);
}
