#include "../src/ast.h"
#include "../src/bytecode.h"
#include "catch.hpp"
#include "../src/lexer.h"
#include "../src/optimizer.h"
#include "../src/parser.h"

using namespace kai;

namespace {

std::vector<Token::Type> lex_token_types(std::string_view source) {
  ErrorReporter reporter;
  Lexer lexer(source, reporter);
  std::vector<Token::Type> tokens;
  while (true) {
    tokens.push_back(lexer.peek().type);
    if (lexer.peek().type == Token::Type::end_of_file) {
      break;
    }
    lexer.skip();
  }
  return tokens;
}

}  // namespace

TEST_CASE("test_parser_expression_end_to_end_literal_42") {
  ErrorReporter reporter;
  Parser parser("42", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 42);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 42);
}

TEST_CASE("test_parser_expression_end_to_end_modulo_minimal") {
  ErrorReporter reporter;
  Parser parser("20 % 6 + 1", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 3);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 3);
}

TEST_CASE("test_parser_expression_end_to_end_equality_minimal") {
  ErrorReporter reporter;
  Parser parser("20 % 6 == 2", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_parser_expression_end_to_end_not_equal_minimal") {
  ErrorReporter reporter;
  Parser parser("20 % 6 != 3", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_parser_expression_end_to_end_less_than") {
  ErrorReporter reporter;
  Parser parser("1 < 2", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_parser_expression_end_to_end_greater_than") {
  ErrorReporter reporter;
  Parser parser("3 > 2", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_parser_expression_end_to_end_equal") {
  ErrorReporter reporter;
  Parser parser("17+3 == 20", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_parser_expression_end_to_end_array_index_assignment") {
  ErrorReporter reporter;
  Parser parser("[7, 8, 9][1]", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 8);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 8);
}

TEST_CASE("test_parser_expression_end_to_end_less_than_or_equal_minimal") {
  ErrorReporter reporter;
  Parser parser("2 <= 2", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_logical_ops_pipeline_lexer_parser_ast_bytecode_short_circuit") {
  const auto tokens = lex_token_types("1 || 0 && 1");
  REQUIRE(tokens == std::vector<Token::Type>{
                        Token::Type::number,
                        Token::Type::pipe_pipe,
                        Token::Type::number,
                        Token::Type::ampersand_ampersand,
                        Token::Type::number,
                        Token::Type::end_of_file,
                    });

  ErrorReporter expression_reporter;
  Parser expression_parser("1 || 0 && 1", expression_reporter);
  std::unique_ptr<Ast> parsed_expression = expression_parser.parse_expression();
  REQUIRE(parsed_expression != nullptr);
  REQUIRE(parsed_expression->type == Ast::Type::LogicalOr);
  const auto &logical_or = derived_cast<const Ast::LogicalOr &>(*parsed_expression);
  REQUIRE(logical_or.right->type == Ast::Type::LogicalAnd);

  ErrorReporter reporter;
  Parser parser(R"(
let x = 0;
let y = 0;
x = 0 && (y = 1);
x = 1 || (y = 2);
x = 1 && (y = 3);
x = 0 || (y = 4);
return y;
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();
  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 4);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 4);
}

TEST_CASE("test_program_end_to_end_count_to_ten_while_loop") {
  ErrorReporter reporter;
  Parser parser(R"(
let i = 0;
while (i < 10) {
  i++;
}
return i;
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 10);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 10);
}

TEST_CASE("test_program_end_to_end_count_down_from_ten_to_one_while_loop") {
  ErrorReporter reporter;
  Parser parser(R"(
let i = 10;
while (i > 1) {
  i = i - 1;
}
return i;
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 1);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_program_end_to_end_if_without_else") {
  ErrorReporter reporter;
  Parser parser(R"(
let x = 0;
if (2 < 1) {
  x = 99;
}
if (1 < 2) {
  x = 42;
}
return x;
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 42);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 42);
}

TEST_CASE("test_program_end_to_end_return_exits_program_early") {
  ErrorReporter reporter;
  Parser parser(R"(
let x = 7;
return x;
x = 99;
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 7);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 7);
}

TEST_CASE("test_program_end_to_end_return_exits_function_early") {
  ErrorReporter reporter;
  Parser parser(R"(
fn early() {
  let x = 1;
  return x;
  x = 2;
}
return early();
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 1);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_program_end_to_end_return_exits_loop_and_function_early") {
  ErrorReporter reporter;
  Parser parser(R"(
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
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 3);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 3);
}

TEST_CASE("test_program_end_to_end_function_parameters") {
  ErrorReporter reporter;
  Parser parser(R"(
fn add(a, b) {
  return a + b;
}
return add(40, 2);
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 42);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 42);
}

TEST_CASE("test_program_end_to_end_function_recursion_fibonacci") {
  ErrorReporter reporter;
  Parser parser(R"(
fn fib(n) {
  if (n < 2) {
    return n;
  } else {
    return fib(n - 1) + fib(n - 2);
  }
}
return fib(10);
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 55);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 55);
}

TEST_CASE("test_program_end_to_end_function_recursion_fibonacci_minimal") {
  ErrorReporter reporter;
  Parser parser(R"(
fn fib(n) {
  if (n < 2) {
    return n;
  } else {
    return fib(n - 1) + fib(n - 2);
  }
}
return fib(2);
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();
  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 1);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_program_end_to_end_tail_recursion_uses_tail_call") {
  ErrorReporter reporter;
  Parser parser(R"(
fn sum_down(n, acc) {
  if (n < 1) {
    return acc;
  } else {
    return sum_down(n - 1, acc + n);
  }
}
return sum_down(10000, 0);
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();
  REQUIRE(program != nullptr);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeOptimizer optimizer;
  optimizer.optimize(generator.blocks());

  bool has_tail_call = false;
  for (const auto &block : generator.blocks()) {
    for (const auto &instr : block.instructions) {
      if (instr->type() == Bytecode::Instruction::Type::TailCall) {
        has_tail_call = true;
      }
    }
  }
  REQUIRE(has_tail_call);

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 50005000);
}

TEST_CASE("test_program_end_to_end_quicksort") {
  ErrorReporter reporter;
  Parser parser(R"(
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
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 12345);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 12345);
}

TEST_CASE("test_parser_expression_end_to_end_logical_not_zero") {
  ErrorReporter reporter;
  Parser parser("!0", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 1);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 1);
}

TEST_CASE("test_parser_expression_end_to_end_logical_not_nonzero") {
  ErrorReporter reporter;
  Parser parser("!1", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 0);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 0);
}

TEST_CASE("test_parser_expression_end_to_end_negate_minimal") {
  ErrorReporter reporter;
  Parser parser("-5 + 10", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 5);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 5);
}

TEST_CASE("test_parser_expression_end_to_end_unary_plus") {
  ErrorReporter reporter;
  Parser parser("+5", reporter);
  std::unique_ptr<Ast> parsed_expression = parser.parse_expression();

  REQUIRE(parsed_expression != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*parsed_expression) == 5);

  Ast::Block program;
  program.append(std::make_unique<Ast::Return>(std::move(parsed_expression)));

  BytecodeGenerator generator;
  generator.visit_block(program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 5);
}

TEST_CASE("test_program_end_to_end_structs_minimal") {
  ErrorReporter reporter;
  Parser parser(R"(
let point = struct { x: 40, y: 2 };
return point.x + point.y;
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);

  AstInterpreter ast_interpreter;
  REQUIRE(ast_interpreter.interpret(*program) == 42);

  BytecodeGenerator generator;
  generator.visit_block(*program);
  generator.finalize();

  BytecodeInterpreter bytecode_interpreter;
  REQUIRE(bytecode_interpreter.interpret(generator.blocks()) == 42);
}
