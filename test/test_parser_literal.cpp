#include "../ast.h"
#include "../catch.hpp"
#include "../parser.h"

using namespace kai::ast;

TEST_CASE("test_parser_parses_number_literal_42") {
  kai::Parser parser("42");
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Literal);

  const auto &literal = ast_cast<const Ast::Literal &>(*parsed);
  REQUIRE(literal.value == 42);
}

TEST_CASE("test_parser_parses_identifier_variable") {
  kai::Parser parser("value");
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Variable);

  const auto &variable = ast_cast<const Ast::Variable &>(*parsed);
  REQUIRE(variable.name == "value");
}

TEST_CASE("test_parser_precedence_multiply_before_add") {
  kai::Parser parser("1 + 2 * 3");
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Add);

  const auto &add = ast_cast<const Ast::Add &>(*parsed);
  REQUIRE(add.left->type == Ast::Type::Literal);
  REQUIRE(add.right->type == Ast::Type::Multiply);

  const auto &left_literal = ast_cast<const Ast::Literal &>(*add.left);
  REQUIRE(left_literal.value == 1);

  const auto &multiply = ast_cast<const Ast::Multiply &>(*add.right);
  REQUIRE(multiply.left->type == Ast::Type::Literal);
  REQUIRE(multiply.right->type == Ast::Type::Literal);
  REQUIRE(ast_cast<const Ast::Literal &>(*multiply.left).value == 2);
  REQUIRE(ast_cast<const Ast::Literal &>(*multiply.right).value == 3);
}

TEST_CASE("test_parser_subtract_is_left_associative") {
  kai::Parser parser("8 - 3 - 1");
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Subtract);

  const auto &outer_subtract = ast_cast<const Ast::Subtract &>(*parsed);
  REQUIRE(outer_subtract.left->type == Ast::Type::Subtract);
  REQUIRE(outer_subtract.right->type == Ast::Type::Literal);

  const auto &inner_subtract = ast_cast<const Ast::Subtract &>(*outer_subtract.left);
  REQUIRE(inner_subtract.left->type == Ast::Type::Literal);
  REQUIRE(inner_subtract.right->type == Ast::Type::Literal);
  REQUIRE(ast_cast<const Ast::Literal &>(*inner_subtract.left).value == 8);
  REQUIRE(ast_cast<const Ast::Literal &>(*inner_subtract.right).value == 3);
  REQUIRE(ast_cast<const Ast::Literal &>(*outer_subtract.right).value == 1);
}

TEST_CASE("test_parser_mixed_identifier_operators_with_precedence") {
  kai::Parser parser("a * b + c / d");
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Add);

  const auto &add = ast_cast<const Ast::Add &>(*parsed);
  REQUIRE(add.left->type == Ast::Type::Multiply);
  REQUIRE(add.right->type == Ast::Type::Divide);

  const auto &left = ast_cast<const Ast::Multiply &>(*add.left);
  REQUIRE(ast_cast<const Ast::Variable &>(*left.left).name == "a");
  REQUIRE(ast_cast<const Ast::Variable &>(*left.right).name == "b");

  const auto &right = ast_cast<const Ast::Divide &>(*add.right);
  REQUIRE(ast_cast<const Ast::Variable &>(*right.left).name == "c");
  REQUIRE(ast_cast<const Ast::Variable &>(*right.right).name == "d");
}

TEST_CASE("test_parser_modulo_has_multiplicative_precedence") {
  kai::Parser parser("8 + 9 % 5");
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Add);

  const auto &add = ast_cast<const Ast::Add &>(*parsed);
  REQUIRE(add.left->type == Ast::Type::Literal);
  REQUIRE(add.right->type == Ast::Type::Modulo);

  const auto &modulo = ast_cast<const Ast::Modulo &>(*add.right);
  REQUIRE(modulo.left->type == Ast::Type::Literal);
  REQUIRE(modulo.right->type == Ast::Type::Literal);
  REQUIRE(ast_cast<const Ast::Literal &>(*modulo.left).value == 9);
  REQUIRE(ast_cast<const Ast::Literal &>(*modulo.right).value == 5);
}

TEST_CASE("test_parser_equality_has_lower_precedence_than_additive") {
  kai::Parser parser("1 + 2 == 3");
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Equal);

  const auto &equal = ast_cast<const Ast::Equal &>(*parsed);
  REQUIRE(equal.left->type == Ast::Type::Add);
  REQUIRE(equal.right->type == Ast::Type::Literal);

  const auto &add = ast_cast<const Ast::Add &>(*equal.left);
  REQUIRE(ast_cast<const Ast::Literal &>(*add.left).value == 1);
  REQUIRE(ast_cast<const Ast::Literal &>(*add.right).value == 2);
  REQUIRE(ast_cast<const Ast::Literal &>(*equal.right).value == 3);
}

TEST_CASE("test_parser_not_equal_has_lower_precedence_than_additive") {
  kai::Parser parser("1 + 2 != 4");
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::NotEqual);

  const auto &not_equal = ast_cast<const Ast::NotEqual &>(*parsed);
  REQUIRE(not_equal.left->type == Ast::Type::Add);
  REQUIRE(not_equal.right->type == Ast::Type::Literal);

  const auto &add = ast_cast<const Ast::Add &>(*not_equal.left);
  REQUIRE(ast_cast<const Ast::Literal &>(*add.left).value == 1);
  REQUIRE(ast_cast<const Ast::Literal &>(*add.right).value == 2);
  REQUIRE(ast_cast<const Ast::Literal &>(*not_equal.right).value == 4);
}

TEST_CASE("test_parser_parses_greater_than_expression") {
  kai::Parser parser("1 + 3 > 3");
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::GreaterThan);

  const auto &greater_than = ast_cast<const Ast::GreaterThan &>(*parsed);
  REQUIRE(greater_than.left->type == Ast::Type::Add);
  REQUIRE(greater_than.right->type == Ast::Type::Literal);

  const auto &add = ast_cast<const Ast::Add &>(*greater_than.left);
  REQUIRE(ast_cast<const Ast::Literal &>(*add.left).value == 1);
  REQUIRE(ast_cast<const Ast::Literal &>(*add.right).value == 3);
  REQUIRE(ast_cast<const Ast::Literal &>(*greater_than.right).value == 3);
}

TEST_CASE("test_parser_parses_while_with_greater_than_condition") {
  kai::Parser parser(R"(
let i = 10;
while (i > 1) {
  i = i - 1;
}
return i;
)");
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(program->children.size() == 3);
  REQUIRE(program->children[1]->type == Ast::Type::While);

  const auto &while_loop = ast_cast<const Ast::While &>(*program->children[1]);
  REQUIRE(while_loop.condition != nullptr);
  REQUIRE(while_loop.condition->type == Ast::Type::GreaterThan);
}
