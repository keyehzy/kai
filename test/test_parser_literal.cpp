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
