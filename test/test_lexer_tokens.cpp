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
