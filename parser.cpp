#include "parser.h"

#include <cassert>
#include <charconv>
#include <system_error>

namespace kai {

Parser::Parser(std::string_view input) : lexer_(input) {}

std::unique_ptr<ast::Ast> Parser::parse_expression() {
  std::unique_ptr<ast::Ast> result = parse_additive();
  assert(lexer_.peek().type == Token::Type::end_of_file);
  return result;
}

std::unique_ptr<ast::Ast> Parser::parse_additive() {
  std::unique_ptr<ast::Ast> left = parse_multiplicative();

  while (true) {
    const Token::Type op = lexer_.peek().type;
    if (op != Token::Type::plus && op != Token::Type::minus) {
      return left;
    }

    lexer_.skip();
    std::unique_ptr<ast::Ast> right = parse_multiplicative();
    if (op == Token::Type::plus) {
      left = std::make_unique<ast::Ast::Add>(std::move(left), std::move(right));
    } else {
      left = std::make_unique<ast::Ast::Subtract>(std::move(left), std::move(right));
    }
  }
}

std::unique_ptr<ast::Ast> Parser::parse_multiplicative() {
  std::unique_ptr<ast::Ast> left = parse_primary();

  while (true) {
    const Token::Type op = lexer_.peek().type;
    if (op != Token::Type::star && op != Token::Type::slash) {
      return left;
    }

    lexer_.skip();
    std::unique_ptr<ast::Ast> right = parse_primary();
    if (op == Token::Type::star) {
      left = std::make_unique<ast::Ast::Multiply>(std::move(left), std::move(right));
    } else {
      left = std::make_unique<ast::Ast::Divide>(std::move(left), std::move(right));
    }
  }
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
  if (token.type == Token::Type::identifier) {
    const std::string name(token.sv());
    lexer_.skip();
    return std::make_unique<ast::Ast::Variable>(name);
  }
  if (token.type == Token::Type::lparen) {
    lexer_.skip();
    std::unique_ptr<ast::Ast> expr = parse_additive();
    assert(lexer_.peek().type == Token::Type::rparen);
    lexer_.skip();
    return expr;
  }

  assert(false);
  return nullptr;
}

}  // namespace kai
