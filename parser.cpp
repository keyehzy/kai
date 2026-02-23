#include "parser.h"

#include <cassert>
#include <charconv>
#include <system_error>

namespace kai {

Parser::Parser(std::string_view input) : lexer_(input) {}

std::unique_ptr<ast::Ast> Parser::parse_expression() {
  std::unique_ptr<ast::Ast> result = parse_assignment();
  assert(lexer_.peek().type == Token::Type::end_of_file);
  return result;
}

std::unique_ptr<ast::Ast> Parser::parse_assignment() {
  std::unique_ptr<ast::Ast> left = parse_equality();

  if (lexer_.peek().type != Token::Type::equals) {
    return left;
  }

  lexer_.skip();
  std::unique_ptr<ast::Ast> value = parse_assignment();

  if (left->type == ast::Ast::Type::Variable) {
    const std::string name = ast::ast_cast<const ast::Ast::Variable &>(*left).name;
    return std::make_unique<ast::Ast::Assignment>(name, std::move(value));
  }

  if (left->type == ast::Ast::Type::Index) {
    auto &index = ast::ast_cast<ast::Ast::Index &>(*left);
    return std::make_unique<ast::Ast::IndexAssignment>(
        std::move(index.array), std::move(index.index), std::move(value));
  }

  assert(false);
  return nullptr;
}

std::unique_ptr<ast::Ast> Parser::parse_equality() {
  std::unique_ptr<ast::Ast> left = parse_comparison();

  while (lexer_.peek().type == Token::Type::equals_equals) {
    lexer_.skip();
    std::unique_ptr<ast::Ast> right = parse_comparison();
    left = std::make_unique<ast::Ast::Equal>(std::move(left), std::move(right));
  }

  return left;
}

std::unique_ptr<ast::Ast> Parser::parse_comparison() {
  std::unique_ptr<ast::Ast> left = parse_additive();

  while (true) {
    const Token::Type op = lexer_.peek().type;
    if (op != Token::Type::less_than && op != Token::Type::greater_than) {
      return left;
    }

    lexer_.skip();
    std::unique_ptr<ast::Ast> right = parse_additive();
    if (op == Token::Type::less_than) {
      left = std::make_unique<ast::Ast::LessThan>(std::move(left), std::move(right));
    } else {
      left = std::make_unique<ast::Ast::GreaterThan>(std::move(left), std::move(right));
    }
  }
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
  std::unique_ptr<ast::Ast> left = parse_postfix();

  while (true) {
    const Token::Type op = lexer_.peek().type;
    if (op != Token::Type::star && op != Token::Type::slash &&
        op != Token::Type::percent) {
      return left;
    }

    lexer_.skip();
    std::unique_ptr<ast::Ast> right = parse_postfix();
    if (op == Token::Type::star) {
      left = std::make_unique<ast::Ast::Multiply>(std::move(left), std::move(right));
    } else if (op == Token::Type::slash) {
      left = std::make_unique<ast::Ast::Divide>(std::move(left), std::move(right));
    } else {
      left = std::make_unique<ast::Ast::Modulo>(std::move(left), std::move(right));
    }
  }
}

std::unique_ptr<ast::Ast> Parser::parse_postfix() {
  std::unique_ptr<ast::Ast> expr = parse_primary();

  while (lexer_.peek().type == Token::Type::lsquare) {
    lexer_.skip();
    std::unique_ptr<ast::Ast> index = parse_assignment();
    assert(lexer_.peek().type == Token::Type::rsquare);
    lexer_.skip();
    expr = std::make_unique<ast::Ast::Index>(std::move(expr), std::move(index));
  }

  return expr;
}

std::unique_ptr<ast::Ast> Parser::parse_array_literal() {
  assert(lexer_.peek().type == Token::Type::lsquare);
  lexer_.skip();

  std::vector<std::unique_ptr<ast::Ast>> elements;
  if (lexer_.peek().type != Token::Type::rsquare) {
    while (true) {
      elements.emplace_back(parse_assignment());
      if (lexer_.peek().type != Token::Type::comma) {
        break;
      }
      lexer_.skip();
    }
  }

  assert(lexer_.peek().type == Token::Type::rsquare);
  lexer_.skip();

  return std::make_unique<ast::Ast::ArrayLiteral>(std::move(elements));
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
    std::unique_ptr<ast::Ast> expr = parse_assignment();
    assert(lexer_.peek().type == Token::Type::rparen);
    lexer_.skip();
    return expr;
  }
  if (token.type == Token::Type::lsquare) {
    return parse_array_literal();
  }

  assert(false);
  return nullptr;
}

}  // namespace kai
