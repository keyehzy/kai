#include "../ast.h"
#include "../bytecode.h"
#include "../catch.hpp"
#include "../parser.h"

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
