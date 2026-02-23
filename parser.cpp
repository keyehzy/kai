#include "parser.h"

#include <cassert>
#include <charconv>
#include <system_error>

namespace kai {

Parser::Parser(std::string_view input) : lexer_(input) {}

std::unique_ptr<ast::Ast> Parser::parse_expression() {
  std::unique_ptr<ast::Ast> result = parse_primary();
  assert(lexer_.peek().type == Token::Type::end_of_file);
  return result;
}

std::unique_ptr<ast::Ast> Parser::parse_primary() {
  const Token &token = lexer_.peek();
  if (token.type == Token::Type::number) {
    ast::Value value = 0;
    const std::string_view source = token.sv();
    const auto [ptr, ec] =
        std::from_chars(source.data(), source.data() + source.size(), value);
    assert(ec == std::errc() && ptr == source.data() + source.size());
    lexer_.skip();
    return std::make_unique<ast::Ast::Literal>(value);
  }

  assert(false);
  return nullptr;
}

}  // namespace kai
