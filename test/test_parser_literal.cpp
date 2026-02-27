#include "../src/ast.h"
#include "catch.hpp"
#include "../src/parser.h"

using namespace kai;

TEST_CASE("test_parser_parses_number_literal_42") {
  ErrorReporter reporter;
  Parser parser("42", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Literal);

  const auto &literal = derived_cast<const Ast::Literal &>(*parsed);
  REQUIRE(literal.value == 42);
}

TEST_CASE("test_parser_parses_identifier_variable") {
  ErrorReporter reporter;
  Parser parser("value", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Variable);

  const auto &variable = derived_cast<const Ast::Variable &>(*parsed);
  REQUIRE(variable.name == "value");
}

TEST_CASE("test_parser_parses_identifier_variable_with_underscore") {
  ErrorReporter reporter;
  Parser parser("_value_2", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Variable);

  const auto &variable = derived_cast<const Ast::Variable &>(*parsed);
  REQUIRE(variable.name == "_value_2");
}

TEST_CASE("test_parser_precedence_multiply_before_add") {
  ErrorReporter reporter;
  Parser parser("1 + 2 * 3", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Add);

  const auto &add = derived_cast<const Ast::Add &>(*parsed);
  REQUIRE(add.left->type == Ast::Type::Literal);
  REQUIRE(add.right->type == Ast::Type::Multiply);

  const auto &left_literal = derived_cast<const Ast::Literal &>(*add.left);
  REQUIRE(left_literal.value == 1);

  const auto &multiply = derived_cast<const Ast::Multiply &>(*add.right);
  REQUIRE(multiply.left->type == Ast::Type::Literal);
  REQUIRE(multiply.right->type == Ast::Type::Literal);
  REQUIRE(derived_cast<const Ast::Literal &>(*multiply.left).value == 2);
  REQUIRE(derived_cast<const Ast::Literal &>(*multiply.right).value == 3);
}

TEST_CASE("test_parser_subtract_is_left_associative") {
  ErrorReporter reporter;
  Parser parser("8 - 3 - 1", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Subtract);

  const auto &outer_subtract = derived_cast<const Ast::Subtract &>(*parsed);
  REQUIRE(outer_subtract.left->type == Ast::Type::Subtract);
  REQUIRE(outer_subtract.right->type == Ast::Type::Literal);

  const auto &inner_subtract = derived_cast<const Ast::Subtract &>(*outer_subtract.left);
  REQUIRE(inner_subtract.left->type == Ast::Type::Literal);
  REQUIRE(inner_subtract.right->type == Ast::Type::Literal);
  REQUIRE(derived_cast<const Ast::Literal &>(*inner_subtract.left).value == 8);
  REQUIRE(derived_cast<const Ast::Literal &>(*inner_subtract.right).value == 3);
  REQUIRE(derived_cast<const Ast::Literal &>(*outer_subtract.right).value == 1);
}

TEST_CASE("test_parser_mixed_identifier_operators_with_precedence") {
  ErrorReporter reporter;
  Parser parser("a * b + c / d", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Add);

  const auto &add = derived_cast<const Ast::Add &>(*parsed);
  REQUIRE(add.left->type == Ast::Type::Multiply);
  REQUIRE(add.right->type == Ast::Type::Divide);

  const auto &left = derived_cast<const Ast::Multiply &>(*add.left);
  REQUIRE(derived_cast<const Ast::Variable &>(*left.left).name == "a");
  REQUIRE(derived_cast<const Ast::Variable &>(*left.right).name == "b");

  const auto &right = derived_cast<const Ast::Divide &>(*add.right);
  REQUIRE(derived_cast<const Ast::Variable &>(*right.left).name == "c");
  REQUIRE(derived_cast<const Ast::Variable &>(*right.right).name == "d");
}

TEST_CASE("test_parser_modulo_has_multiplicative_precedence") {
  ErrorReporter reporter;
  Parser parser("8 + 9 % 5", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Add);

  const auto &add = derived_cast<const Ast::Add &>(*parsed);
  REQUIRE(add.left->type == Ast::Type::Literal);
  REQUIRE(add.right->type == Ast::Type::Modulo);

  const auto &modulo = derived_cast<const Ast::Modulo &>(*add.right);
  REQUIRE(modulo.left->type == Ast::Type::Literal);
  REQUIRE(modulo.right->type == Ast::Type::Literal);
  REQUIRE(derived_cast<const Ast::Literal &>(*modulo.left).value == 9);
  REQUIRE(derived_cast<const Ast::Literal &>(*modulo.right).value == 5);
}

TEST_CASE("test_parser_equality_has_lower_precedence_than_additive") {
  ErrorReporter reporter;
  Parser parser("1 + 2 == 3", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Equal);

  const auto &equal = derived_cast<const Ast::Equal &>(*parsed);
  REQUIRE(equal.left->type == Ast::Type::Add);
  REQUIRE(equal.right->type == Ast::Type::Literal);

  const auto &add = derived_cast<const Ast::Add &>(*equal.left);
  REQUIRE(derived_cast<const Ast::Literal &>(*add.left).value == 1);
  REQUIRE(derived_cast<const Ast::Literal &>(*add.right).value == 2);
  REQUIRE(derived_cast<const Ast::Literal &>(*equal.right).value == 3);
}

TEST_CASE("test_parser_not_equal_has_lower_precedence_than_additive") {
  ErrorReporter reporter;
  Parser parser("1 + 2 != 4", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::NotEqual);

  const auto &not_equal = derived_cast<const Ast::NotEqual &>(*parsed);
  REQUIRE(not_equal.left->type == Ast::Type::Add);
  REQUIRE(not_equal.right->type == Ast::Type::Literal);

  const auto &add = derived_cast<const Ast::Add &>(*not_equal.left);
  REQUIRE(derived_cast<const Ast::Literal &>(*add.left).value == 1);
  REQUIRE(derived_cast<const Ast::Literal &>(*add.right).value == 2);
  REQUIRE(derived_cast<const Ast::Literal &>(*not_equal.right).value == 4);
}

TEST_CASE("test_parser_parses_logical_and_expression") {
  ErrorReporter reporter;
  Parser parser("1 == 1 && 2 == 2", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::LogicalAnd);

  const auto &logical_and = derived_cast<const Ast::LogicalAnd &>(*parsed);
  REQUIRE(logical_and.left->type == Ast::Type::Equal);
  REQUIRE(logical_and.right->type == Ast::Type::Equal);
}

TEST_CASE("test_parser_logical_or_has_lower_precedence_than_logical_and") {
  ErrorReporter reporter;
  Parser parser("1 || 0 && 0", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::LogicalOr);

  const auto &logical_or = derived_cast<const Ast::LogicalOr &>(*parsed);
  REQUIRE(logical_or.left->type == Ast::Type::Literal);
  REQUIRE(logical_or.right->type == Ast::Type::LogicalAnd);
}

TEST_CASE("test_parser_parses_greater_than_expression") {
  ErrorReporter reporter;
  Parser parser("1 + 3 > 3", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::GreaterThan);

  const auto &greater_than = derived_cast<const Ast::GreaterThan &>(*parsed);
  REQUIRE(greater_than.left->type == Ast::Type::Add);
  REQUIRE(greater_than.right->type == Ast::Type::Literal);

  const auto &add = derived_cast<const Ast::Add &>(*greater_than.left);
  REQUIRE(derived_cast<const Ast::Literal &>(*add.left).value == 1);
  REQUIRE(derived_cast<const Ast::Literal &>(*add.right).value == 3);
  REQUIRE(derived_cast<const Ast::Literal &>(*greater_than.right).value == 3);
}

TEST_CASE("test_parser_parses_less_than_or_equal_expression") {
  ErrorReporter reporter;
  Parser parser("1 + 3 <= 4", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::LessThanOrEqual);

  const auto &less_than_or_equal = derived_cast<const Ast::LessThanOrEqual &>(*parsed);
  REQUIRE(less_than_or_equal.left->type == Ast::Type::Add);
  REQUIRE(less_than_or_equal.right->type == Ast::Type::Literal);
}

TEST_CASE("test_parser_parses_greater_than_or_equal_expression") {
  ErrorReporter reporter;
  Parser parser("5 >= 3 + 2", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::GreaterThanOrEqual);

  const auto &greater_than_or_equal = derived_cast<const Ast::GreaterThanOrEqual &>(*parsed);
  REQUIRE(greater_than_or_equal.left->type == Ast::Type::Literal);
  REQUIRE(greater_than_or_equal.right->type == Ast::Type::Add);
}

TEST_CASE("test_parser_parses_while_with_greater_than_condition") {
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
  REQUIRE(program->children.size() == 3);
  REQUIRE(program->children[1]->type == Ast::Type::While);

  const auto &while_loop = derived_cast<const Ast::While &>(*program->children[1]);
  REQUIRE(while_loop.condition != nullptr);
  REQUIRE(while_loop.condition->type == Ast::Type::GreaterThan);
}

TEST_CASE("test_parser_parses_function_call_with_arguments") {
  ErrorReporter reporter;
  Parser parser("sum(1, 2 + 3)", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::FunctionCall);

  const auto &function_call = derived_cast<const Ast::FunctionCall &>(*parsed);
  REQUIRE(function_call.name == "sum");
  REQUIRE(function_call.arguments.size() == 2);
  REQUIRE(function_call.arguments[0]->type == Ast::Type::Literal);
  REQUIRE(function_call.arguments[1]->type == Ast::Type::Add);
}

TEST_CASE("test_parser_parses_function_declaration_with_parameters") {
  ErrorReporter reporter;
  Parser parser(R"(
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
      derived_cast<const Ast::FunctionDeclaration &>(*program->children[0]);
  REQUIRE(function_declaration.name == "add");
  REQUIRE(function_declaration.parameters == std::vector<std::string>{"a", "b"});

  const auto &return_statement = derived_cast<const Ast::Return &>(*program->children[1]);
  REQUIRE(return_statement.value->type == Ast::Type::FunctionCall);
  const auto &function_call = derived_cast<const Ast::FunctionCall &>(*return_statement.value);
  REQUIRE(function_call.name == "add");
  REQUIRE(function_call.arguments.size() == 2);
}

TEST_CASE("test_parser_parses_recursive_fibonacci_if_else") {
  ErrorReporter reporter;
  Parser parser(R"(
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

  const auto &fib_decl = derived_cast<const Ast::FunctionDeclaration &>(*program->children[0]);
  REQUIRE(fib_decl.name == "fib");
  REQUIRE(fib_decl.parameters == std::vector<std::string>{"n"});
  REQUIRE(fib_decl.body->children.size() == 1);
  REQUIRE(fib_decl.body->children[0]->type == Ast::Type::IfElse);

  const auto &if_else = derived_cast<const Ast::IfElse &>(*fib_decl.body->children[0]);
  REQUIRE(if_else.condition->type == Ast::Type::LessThan);
  REQUIRE(if_else.body->children.size() == 1);
  REQUIRE(if_else.else_body->children.size() == 1);
}

TEST_CASE("test_parser_parses_if_without_else") {
  ErrorReporter reporter;
  Parser parser(R"(
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

  const auto &if_else = derived_cast<const Ast::IfElse &>(*program->children[1]);
  REQUIRE(if_else.condition->type == Ast::Type::LessThan);
  REQUIRE(if_else.body->children.size() == 1);
  REQUIRE(if_else.else_body->children.empty());
}

TEST_CASE("test_parser_parses_if_without_else_before_if_else") {
  ErrorReporter reporter;
  Parser parser(R"(
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

  const auto &if_without_else = derived_cast<const Ast::IfElse &>(*program->children[1]);
  REQUIRE(if_without_else.else_body->children.empty());

  const auto &if_with_else = derived_cast<const Ast::IfElse &>(*program->children[2]);
  REQUIRE(if_with_else.body->children.size() == 1);
  REQUIRE(if_with_else.else_body->children.size() == 1);
}

TEST_CASE("test_parser_parses_struct_literal_field_access_expression") {
  ErrorReporter reporter;
  Parser parser("struct { x: 40, y: 2 }.x", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::FieldAccess);

  const auto &field_access = derived_cast<const Ast::FieldAccess &>(*parsed);
  REQUIRE(field_access.field == "x");
  REQUIRE(field_access.object->type == Ast::Type::StructLiteral);

  const auto &struct_literal = derived_cast<const Ast::StructLiteral &>(*field_access.object);
  REQUIRE(struct_literal.fields.size() == 2);
  REQUIRE(struct_literal.fields[0].first == "x");
  REQUIRE(struct_literal.fields[1].first == "y");
  REQUIRE(struct_literal.fields[0].second->type == Ast::Type::Literal);
  REQUIRE(struct_literal.fields[1].second->type == Ast::Type::Literal);
}

TEST_CASE("test_parser_reports_missing_colon_in_struct_literal_field") {
  ErrorReporter reporter;
  Parser parser("struct { x 1 }", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::StructLiteral);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedStructFieldColon);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected ':' after field name 'x' in struct literal, found '1'");
  REQUIRE(reporter.errors()[0]->location.text() == "1");
}

TEST_CASE("test_parser_reports_missing_field_name_in_struct_literal") {
  ErrorReporter reporter;
  Parser parser("struct { : 1 }", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::StructLiteral);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedStructFieldName);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected field name in struct literal, found ':'");
  REQUIRE(reporter.errors()[0]->location.text() == ":");
}

TEST_CASE("test_parser_reports_missing_opening_brace_in_struct_literal") {
  ErrorReporter reporter;
  Parser parser("struct x: 1 }", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::StructLiteral);
  const auto& struct_literal = derived_cast<const Ast::StructLiteral&>(*parsed);
  REQUIRE(struct_literal.fields.empty());
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedStructLiteralBrace);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected '{' to start struct literal, found 'x'");
}

TEST_CASE("test_parser_reports_missing_closing_brace_in_struct_literal") {
  ErrorReporter reporter;
  Parser parser("struct { x: 1", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::StructLiteral);
  const auto& struct_literal = derived_cast<const Ast::StructLiteral&>(*parsed);
  REQUIRE(struct_literal.fields.size() == 1);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedStructLiteralBrace);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected '}' to close struct literal, found end of input");
}

TEST_CASE("test_parser_expression_stops_at_trailing_tokens") {
  ErrorReporter reporter;
  Parser parser("1 2", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Literal);
  REQUIRE(derived_cast<const Ast::Literal&>(*parsed).value == 1);
  REQUIRE_FALSE(reporter.has_errors());
}

TEST_CASE("test_parser_program_reports_missing_semicolon_after_statement") {
  ErrorReporter reporter;
  Parser parser("1 2", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(program->children.size() == 1);
  REQUIRE(program->children[0]->type == Ast::Type::Literal);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedSemicolon);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected ';' after statement, found '2'");
  REQUIRE(reporter.errors()[0]->location.text() == "2");
}

TEST_CASE("test_parser_reports_missing_equals_in_let_declaration") {
  ErrorReporter reporter;
  Parser parser("let x 1;", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(program->children.size() == 1);
  REQUIRE(program->children[0]->type == Ast::Type::VariableDeclaration);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedEquals);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected '=' after variable 'x' in 'let' declaration, found '1'");
  REQUIRE(reporter.errors()[0]->location.text() == "1");
}

TEST_CASE("test_parser_reports_missing_variable_name_in_let_declaration") {
  ErrorReporter reporter;
  Parser parser("let = 1;", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(program->children.size() == 1);
  REQUIRE(program->children[0]->type == Ast::Type::Literal);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedLetVariableName);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected variable name after 'let', found '='");
  REQUIRE(reporter.errors()[0]->location.text() == "=");
}

TEST_CASE("test_parser_reports_invalid_assignment_target") {
  ErrorReporter reporter;
  Parser parser("1 = 2", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Literal);
  REQUIRE(derived_cast<const Ast::Literal&>(*parsed).value == 1);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::InvalidAssignmentTarget);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "invalid assignment target; expected variable or index expression before '=', found '='");
  REQUIRE(reporter.errors()[0]->location.text() == "=");
}

TEST_CASE("test_parser_reports_expected_primary_expression_for_standalone_semicolon") {
  ErrorReporter reporter;
  Parser parser(";", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Literal);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedPrimaryExpression);
  REQUIRE(reporter.errors()[0]->format_error() == "expected primary expression, found ';'");
  REQUIRE(reporter.errors()[0]->location.text() == ";");
}

TEST_CASE("test_parser_reports_expected_variable_for_function_call_target") {
  ErrorReporter reporter;
  Parser parser("1()", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Literal);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedVariable);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected variable as function call target, found '('");
}

TEST_CASE("test_parser_reports_expected_variable_before_postfix_increment") {
  ErrorReporter reporter;
  Parser parser("1++", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Literal);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedVariable);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected variable before postfix '++', found '++'");
}

TEST_CASE("test_parser_reports_expected_identifier_after_dot_in_field_access") {
  ErrorReporter reporter;
  Parser parser("value.", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Variable);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedIdentifier);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected identifier after '.' in field access, found end of input");
}

TEST_CASE("test_parser_reports_missing_closing_square_bracket_in_index_expression") {
  ErrorReporter reporter;
  Parser parser("value[1", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Index);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedClosingSquareBracket);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected ']' to close index expression, found end of input");
}

TEST_CASE("test_parser_reports_missing_closing_square_bracket_in_array_literal") {
  ErrorReporter reporter;
  Parser parser("[1", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::ArrayLiteral);
  const auto& array_literal = derived_cast<const Ast::ArrayLiteral&>(*parsed);
  REQUIRE(array_literal.elements.size() == 1);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedClosingSquareBracket);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected ']' to close array literal, found end of input");
}

TEST_CASE("test_parser_reports_invalid_numeric_literal") {
  ErrorReporter reporter;
  Parser parser("99999999999999999999999999999999999999", reporter);
  std::unique_ptr<Ast> parsed = parser.parse_expression();

  REQUIRE(parsed != nullptr);
  REQUIRE(parsed->type == Ast::Type::Literal);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::InvalidNumericLiteral);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "invalid numeric literal '99999999999999999999999999999999999999'");
}

TEST_CASE("test_parser_reports_missing_while_block_with_context") {
  ErrorReporter reporter;
  Parser parser("while (1) return 1;", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedBlock);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected '{' to start while block, found 'return'");
}

TEST_CASE("test_parser_reports_missing_else_block_with_context") {
  ErrorReporter reporter;
  Parser parser("if (1) { return 1; } else return 2;", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedBlock);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected '{' to start else block, found 'return'");
}

TEST_CASE("test_parser_reports_unterminated_anonymous_block") {
  ErrorReporter reporter;
  Parser parser("{ return 1;", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedBlock);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected '}' to close block, found end of input");
}

TEST_CASE("test_parser_reports_missing_closing_parenthesis_in_while_condition") {
  ErrorReporter reporter;
  Parser parser("while (1 { return 1; }", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedClosingParenthesis);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected ')' to close 'while' condition, found '{'");
}

TEST_CASE("test_parser_reports_missing_opening_parenthesis_in_while_condition") {
  ErrorReporter reporter;
  Parser parser("while 1) { return 1; }", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedOpeningParenthesis);
  REQUIRE(reporter.errors()[0]->format_error() == "expected '(' after 'while', found '1'");
}

TEST_CASE("test_parser_reports_missing_function_name_after_fn_keyword") {
  ErrorReporter reporter;
  Parser parser("fn (a) { return a; }", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedFunctionIdentifier);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected function name after 'fn', found '('");
}

TEST_CASE("test_parser_reports_missing_opening_parenthesis_after_function_name") {
  ErrorReporter reporter;
  Parser parser("fn add a) { return a; }", reporter);
  std::unique_ptr<Ast::Block> program = parser.parse_program();

  REQUIRE(program != nullptr);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0]->type == Error::Type::ExpectedOpeningParenthesis);
  REQUIRE(reporter.errors()[0]->format_error() ==
          "expected '(' after function name 'add' in declaration, found 'a'");
}
