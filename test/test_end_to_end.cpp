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
