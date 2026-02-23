#include "../catch.hpp"
#include "../lexer.h"

#include <cstddef>
#include <utility>
#include <vector>

namespace {

using LexedToken = std::pair<Token::Type, std::string_view>;
using TokenType = Token::Type;

std::vector<LexedToken> lex_all(std::string_view source) {
  Lexer lexer(source);
  std::vector<LexedToken> tokens;

  while (true) {
    const Token &token = lexer.peek();
    tokens.emplace_back(token.type, token.sv());
    if (token.type == Token::Type::end_of_file) {
      break;
    }
    lexer.skip();
  }

  return tokens;
}

std::vector<TokenType> lex_types(std::string_view source) {
  const auto tokens = lex_all(source);
  std::vector<TokenType> types;
  types.reserve(tokens.size());

  for (const auto &token : tokens) {
    types.push_back(token.first);
  }

  return types;
}

} // namespace

TEST_CASE("test_lexer_for_loop_has_no_unknown_tokens") {
  const auto tokens = lex_all("for (int i = 0; i < n; i++) { t = t + 1; }");

  REQUIRE(!tokens.empty());
  REQUIRE(tokens.back().first == Token::Type::end_of_file);

  for (const auto &token : tokens) {
    REQUIRE(token.first != Token::Type::unknown);
  }
}

TEST_CASE("test_lexer_recognizes_less_than") {
  const auto types = lex_types("i < n");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::less_than,
                       Token::Type::identifier,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_greater_than") {
  const auto types = lex_types("n > i");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::greater_than,
                       Token::Type::identifier,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_less_than_or_equal") {
  const auto types = lex_types("i <= n");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::less_than_equals,
                       Token::Type::identifier,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_greater_than_or_equal") {
  const auto types = lex_types("n >= i");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::greater_than_equals,
                       Token::Type::identifier,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_countdown_while_loop_tokens") {
  const auto types = lex_types("while (i > 1) { i = i - 1; }");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::lparen,
                       Token::Type::identifier,
                       Token::Type::greater_than,
                       Token::Type::number,
                       Token::Type::rparen,
                       Token::Type::lcurly,
                       Token::Type::identifier,
                       Token::Type::equals,
                       Token::Type::identifier,
                       Token::Type::minus,
                       Token::Type::number,
                       Token::Type::semicolon,
                       Token::Type::rcurly,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_increment_operator") {
  const auto types = lex_types("i++");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::plus_plus,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_array_index_assignment_tokens") {
  const auto types = lex_types("values[1] = 42;");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::lsquare,
                       Token::Type::number,
                       Token::Type::rsquare,
                       Token::Type::equals,
                       Token::Type::number,
                       Token::Type::semicolon,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_struct_literal_and_field_access_tokens") {
  const auto types = lex_types("struct { x: 1, y: 2 }.x");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::lcurly,
                       Token::Type::identifier,
                       Token::Type::colon,
                       Token::Type::number,
                       Token::Type::comma,
                       Token::Type::identifier,
                       Token::Type::colon,
                       Token::Type::number,
                       Token::Type::rcurly,
                       Token::Type::dot,
                       Token::Type::identifier,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_commas") {
  const auto types = lex_types("[1, 2, 3]");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::lsquare,
                       Token::Type::number,
                       Token::Type::comma,
                       Token::Type::number,
                       Token::Type::comma,
                       Token::Type::number,
                       Token::Type::rsquare,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_basic_arithmetic_operators") {
  const auto types = lex_types("a - b * c / d");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::minus,
                       Token::Type::identifier,
                       Token::Type::star,
                       Token::Type::identifier,
                       Token::Type::slash,
                       Token::Type::identifier,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_modulo_operator") {
  const auto types = lex_types("a % b");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::percent,
                       Token::Type::identifier,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_equality_operator") {
  const auto types = lex_types("a == b");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::equals_equals,
                       Token::Type::identifier,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_not_equal_operator") {
  const auto types = lex_types("a != b");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::identifier,
                       Token::Type::bang_equals,
                       Token::Type::identifier,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_array_index_assignment_comparison_expression") {
  const auto types = lex_types("([7, 8, 9][1] = 42) < 50");

  REQUIRE(types == std::vector<TokenType>{
                       Token::Type::lparen,
                       Token::Type::lsquare,
                       Token::Type::number,
                       Token::Type::comma,
                       Token::Type::number,
                       Token::Type::comma,
                       Token::Type::number,
                       Token::Type::rsquare,
                       Token::Type::lsquare,
                       Token::Type::number,
                       Token::Type::rsquare,
                       Token::Type::equals,
                       Token::Type::number,
                       Token::Type::rparen,
                       Token::Type::less_than,
                       Token::Type::number,
                       Token::Type::end_of_file,
                   });
}

TEST_CASE("test_lexer_recognizes_if_else_recursive_fibonacci_tokens") {
  const auto tokens =
      lex_all("if (n < 2) { return n; } else { return fib(n - 1) + fib(n - 2); }");

  REQUIRE(!tokens.empty());
  REQUIRE(tokens.back().first == Token::Type::end_of_file);

  for (const auto &token : tokens) {
    REQUIRE(token.first != Token::Type::unknown);
  }
}

TEST_CASE("test_lexer_recognizes_identifiers_with_underscores") {
  const auto tokens = lex_all("_tmp1 = foo_bar + baz_2;");

  REQUIRE(tokens == std::vector<LexedToken>{
                       {Token::Type::identifier, "_tmp1"},
                       {Token::Type::equals, "="},
                       {Token::Type::identifier, "foo_bar"},
                       {Token::Type::plus, "+"},
                       {Token::Type::identifier, "baz_2"},
                       {Token::Type::semicolon, ";"},
                       {Token::Type::end_of_file, ""},
                   });
}
