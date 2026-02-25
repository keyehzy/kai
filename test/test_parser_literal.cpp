#include "../src/ast.h"
#include "catch.hpp"
#include "../src/parser.h"

using namespace kai::ast;

TEST_CASE("test_parser_parses_number_literal_42") {
  kai::ErrorReporter reporter;
  kai::Parser parser("42", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Literal);

  const auto &literal = ast_cast<const Ast::Literal &>(*parsed);
  REQUIRE(literal.value == 42);
}

TEST_CASE("test_parser_parses_identifier_variable") {
  kai::ErrorReporter reporter;
  kai::Parser parser("value", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Variable);

  const auto &variable = ast_cast<const Ast::Variable &>(*parsed);
  REQUIRE(variable.name == "value");
}

TEST_CASE("test_parser_parses_identifier_variable_with_underscore") {
  kai::ErrorReporter reporter;
  kai::Parser parser("_value_2", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Variable);

  const auto &variable = ast_cast<const Ast::Variable &>(*parsed);
  REQUIRE(variable.name == "_value_2");
}

TEST_CASE("test_parser_precedence_multiply_before_add") {
  kai::ErrorReporter reporter;
  kai::Parser parser("1 + 2 * 3", reporter);
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
  kai::ErrorReporter reporter;
  kai::Parser parser("8 - 3 - 1", reporter);
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
  kai::ErrorReporter reporter;
  kai::Parser parser("a * b + c / d", reporter);
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
  kai::ErrorReporter reporter;
  kai::Parser parser("8 + 9 % 5", reporter);
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
  kai::ErrorReporter reporter;
  kai::Parser parser("1 + 2 == 3", reporter);
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
  kai::ErrorReporter reporter;
  kai::Parser parser("1 + 2 != 4", reporter);
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
  kai::ErrorReporter reporter;
  kai::Parser parser("1 + 3 > 3", reporter);
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

TEST_CASE("test_parser_parses_less_than_or_equal_expression") {
  kai::ErrorReporter reporter;
  kai::Parser parser("1 + 3 <= 4", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::LessThanOrEqual);

  const auto &less_than_or_equal = ast_cast<const Ast::LessThanOrEqual &>(*parsed);
  REQUIRE(less_than_or_equal.left->type == Ast::Type::Add);
  REQUIRE(less_than_or_equal.right->type == Ast::Type::Literal);
}

TEST_CASE("test_parser_parses_greater_than_or_equal_expression") {
  kai::ErrorReporter reporter;
  kai::Parser parser("5 >= 3 + 2", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::GreaterThanOrEqual);

  const auto &greater_than_or_equal = ast_cast<const Ast::GreaterThanOrEqual &>(*parsed);
  REQUIRE(greater_than_or_equal.left->type == Ast::Type::Literal);
  REQUIRE(greater_than_or_equal.right->type == Ast::Type::Add);
}

TEST_CASE("test_parser_parses_while_with_greater_than_condition") {
  kai::ErrorReporter reporter;
  kai::Parser parser(R"(
let i = 10;
while (i > 1) {
  i = i - 1;
}
return i;
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(program->children.size() == 3);
  REQUIRE(program->children[1]->type == Ast::Type::While);

  const auto &while_loop = ast_cast<const Ast::While &>(*program->children[1]);
  REQUIRE(while_loop.condition != nullptr);
  REQUIRE(while_loop.condition->type == Ast::Type::GreaterThan);
}

TEST_CASE("test_parser_parses_function_call_with_arguments") {
  kai::ErrorReporter reporter;
  kai::Parser parser("sum(1, 2 + 3)", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::FunctionCall);

  const auto &function_call = ast_cast<const Ast::FunctionCall &>(*parsed);
  REQUIRE(function_call.name == "sum");
  REQUIRE(function_call.arguments.size() == 2);
  REQUIRE(function_call.arguments[0]->type == Ast::Type::Literal);
  REQUIRE(function_call.arguments[1]->type == Ast::Type::Add);
}

TEST_CASE("test_parser_parses_function_declaration_with_parameters") {
  kai::ErrorReporter reporter;
  kai::Parser parser(R"(
fn add(a, b) {
  return a + b;
}
return add(1, 2);
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(program->children.size() == 2);
  REQUIRE(program->children[0]->type == Ast::Type::FunctionDeclaration);
  REQUIRE(program->children[1]->type == Ast::Type::Return);

  const auto &function_declaration =
      ast_cast<const Ast::FunctionDeclaration &>(*program->children[0]);
  REQUIRE(function_declaration.name == "add");
  REQUIRE(function_declaration.parameters == std::vector<std::string>{"a", "b"});

  const auto &return_statement = ast_cast<const Ast::Return &>(*program->children[1]);
  REQUIRE(return_statement.value->type == Ast::Type::FunctionCall);
  const auto &function_call = ast_cast<const Ast::FunctionCall &>(*return_statement.value);
  REQUIRE(function_call.name == "add");
  REQUIRE(function_call.arguments.size() == 2);
}

TEST_CASE("test_parser_parses_recursive_fibonacci_if_else") {
  kai::ErrorReporter reporter;
  kai::Parser parser(R"(
fn fib(n) {
  if (n < 2) {
    return n;
  } else {
    return fib(n - 1) + fib(n - 2);
  }
}
return fib(5);
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(program->children.size() == 2);
  REQUIRE(program->children[0]->type == Ast::Type::FunctionDeclaration);

  const auto &fib_decl = ast_cast<const Ast::FunctionDeclaration &>(*program->children[0]);
  REQUIRE(fib_decl.name == "fib");
  REQUIRE(fib_decl.parameters == std::vector<std::string>{"n"});
  REQUIRE(fib_decl.body->children.size() == 1);
  REQUIRE(fib_decl.body->children[0]->type == Ast::Type::IfElse);

  const auto &if_else = ast_cast<const Ast::IfElse &>(*fib_decl.body->children[0]);
  REQUIRE(if_else.condition->type == Ast::Type::LessThan);
  REQUIRE(if_else.body->children.size() == 1);
  REQUIRE(if_else.else_body->children.size() == 1);
}

TEST_CASE("test_parser_parses_if_without_else") {
  kai::ErrorReporter reporter;
  kai::Parser parser(R"(
let x = 0;
if (1 < 2) {
  x = 7;
}
return x;
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(program->children.size() == 3);
  REQUIRE(program->children[1]->type == Ast::Type::IfElse);

  const auto &if_else = ast_cast<const Ast::IfElse &>(*program->children[1]);
  REQUIRE(if_else.condition->type == Ast::Type::LessThan);
  REQUIRE(if_else.body->children.size() == 1);
  REQUIRE(if_else.else_body->children.empty());
}

TEST_CASE("test_parser_parses_if_without_else_before_if_else") {
  kai::ErrorReporter reporter;
  kai::Parser parser(R"(
let x = 0;
if (1 < 2) {
  x = 1;
}
if (2 < 1) {
  x = 2;
} else {
  x = 3;
}
return x;
)", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(program->children.size() == 4);
  REQUIRE(program->children[1]->type == Ast::Type::IfElse);
  REQUIRE(program->children[2]->type == Ast::Type::IfElse);

  const auto &if_without_else = ast_cast<const Ast::IfElse &>(*program->children[1]);
  REQUIRE(if_without_else.else_body->children.empty());

  const auto &if_with_else = ast_cast<const Ast::IfElse &>(*program->children[2]);
  REQUIRE(if_with_else.body->children.size() == 1);
  REQUIRE(if_with_else.else_body->children.size() == 1);
}

TEST_CASE("test_parser_parses_struct_literal_field_access_expression") {
  kai::ErrorReporter reporter;
  kai::Parser parser("struct { x: 40, y: 2 }.x", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::FieldAccess);

  const auto &field_access = ast_cast<const Ast::FieldAccess &>(*parsed);
  REQUIRE(field_access.field == "x");
  REQUIRE(field_access.object->type == Ast::Type::StructLiteral);

  const auto &struct_literal = ast_cast<const Ast::StructLiteral &>(*field_access.object);
  REQUIRE(struct_literal.fields.size() == 2);
  REQUIRE(struct_literal.fields[0].first == "x");
  REQUIRE(struct_literal.fields[1].first == "y");
  REQUIRE(struct_literal.fields[0].second->type == Ast::Type::Literal);
  REQUIRE(struct_literal.fields[1].second->type == Ast::Type::Literal);
}
