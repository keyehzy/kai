#include "parser.h"

#include <cassert>
#include <charconv>
#include <system_error>
#include <utility>

namespace kai {

namespace {
bool token_is_identifier(const Token &token, std::string_view text) {
  return token.type == Token::Type::identifier && token.sv() == text;
}
}  // namespace

Parser::Parser(std::string_view input, ErrorReporter& error_reporter)
    : error_reporter_(error_reporter), lexer_(input, error_reporter_) {}

std::unique_ptr<ast::Ast::Block> Parser::parse_program() {
  auto program = std::make_unique<ast::Ast::Block>();
  while (lexer_.peek().type != Token::Type::end_of_file) {
    program->append(parse_statement());
  }
  return program;
}

std::unique_ptr<ast::Ast> Parser::parse_expression() {
  return parse_assignment();
}

std::unique_ptr<ast::Ast> Parser::parse_statement() {
  const Token &token = lexer_.peek();

  if (token_is_identifier(token, "let")) {
    lexer_.skip();
    if (lexer_.peek().type != Token::Type::identifier) {
      const Token &missing_name_token = lexer_.peek();
      error_reporter_.report<ExpectedLetVariableNameError>(
          missing_name_token.source_location());
      while (lexer_.peek().type != Token::Type::end_of_file &&
             lexer_.peek().type != Token::Type::rcurly &&
             lexer_.peek().type != Token::Type::semicolon) {
        lexer_.skip();
      }
      if (lexer_.peek().type == Token::Type::semicolon) {
        lexer_.skip();
      }
      return std::make_unique<ast::Ast::Literal>(0);
    }

    const Token variable_name_token = lexer_.peek();
    const std::string name(variable_name_token.sv());
    lexer_.skip();
    consume<ExpectedEqualsError>(Token::Type::equals,
                                 ExpectedEqualsError::Ctx::AfterLetVariableName,
                                 variable_name_token.source_location());
    std::unique_ptr<ast::Ast> initializer = parse_expression();
    consume_statement_terminator();
    return std::make_unique<ast::Ast::VariableDeclaration>(name, std::move(initializer));
  }

  if (token_is_identifier(token, "while")) {
    const Token while_token = token;
    lexer_.skip();
    consume<ExpectedOpeningParenthesisError>(Token::Type::lparen,
                                             ExpectedOpeningParenthesisError::Ctx::AfterWhile,
                                             while_token.source_location());
    std::unique_ptr<ast::Ast> condition = parse_expression();
    consume<ExpectedClosingParenthesisError>(
        Token::Type::rparen, ExpectedClosingParenthesisError::Ctx::ToCloseWhileCondition,
        while_token.source_location());
    std::unique_ptr<ast::Ast::Block> body = parse_block(while_token);
    return std::make_unique<ast::Ast::While>(std::move(condition), std::move(body));
  }

  if (token_is_identifier(token, "if")) {
    const Token if_token = token;
    lexer_.skip();
    consume<ExpectedOpeningParenthesisError>(Token::Type::lparen,
                                             ExpectedOpeningParenthesisError::Ctx::AfterIf,
                                             if_token.source_location());
    std::unique_ptr<ast::Ast> condition = parse_expression();
    consume<ExpectedClosingParenthesisError>(
        Token::Type::rparen, ExpectedClosingParenthesisError::Ctx::ToCloseIfCondition,
        if_token.source_location());

    std::unique_ptr<ast::Ast::Block> body = parse_block(if_token);
    std::unique_ptr<ast::Ast::Block> else_body;
    if (token_is_identifier(lexer_.peek(), "else")) {
      const Token else_token = lexer_.peek();
      lexer_.skip();
      else_body = parse_block(else_token);
    } else {
      else_body = std::make_unique<ast::Ast::Block>();
    }

    return std::make_unique<ast::Ast::IfElse>(std::move(condition), std::move(body),
                                               std::move(else_body));
  }

  if (token_is_identifier(token, "return")) {
    lexer_.skip();
    std::unique_ptr<ast::Ast> value = parse_expression();
    consume_statement_terminator();
    return std::make_unique<ast::Ast::Return>(std::move(value));
  }

  if (token_is_identifier(token, "fn")) {
    const Token fn_token = token;
    lexer_.skip();
    assert(lexer_.peek().type == Token::Type::identifier);
    const Token function_name_token = lexer_.peek();
    const std::string name(lexer_.peek().sv());
    lexer_.skip();

    consume<ExpectedOpeningParenthesisError>(
        Token::Type::lparen,
        ExpectedOpeningParenthesisError::Ctx::AfterFunctionNameInDeclaration,
        function_name_token.source_location());
    std::vector<std::string> parameters;
    if (lexer_.peek().type != Token::Type::rparen) {
      while (true) {
        assert(lexer_.peek().type == Token::Type::identifier);
        parameters.emplace_back(lexer_.peek().sv());
        lexer_.skip();
        if (lexer_.peek().type != Token::Type::comma) {
          break;
        }
        lexer_.skip();
      }
    }
    consume<ExpectedClosingParenthesisError>(
        Token::Type::rparen,
        ExpectedClosingParenthesisError::Ctx::ToCloseFunctionParameterList);

    std::unique_ptr<ast::Ast::Block> body = parse_block(fn_token);
    return std::make_unique<ast::Ast::FunctionDeclaration>(name, std::move(parameters),
                                                           std::move(body));
  }

  if (token.type == Token::Type::lcurly) {
    return parse_block(std::nullopt);
  }

  std::unique_ptr<ast::Ast> expr = parse_expression();
  consume_statement_terminator();
  return expr;
}

void Parser::consume_statement_terminator() {
  if (lexer_.peek().type == Token::Type::semicolon) {
    lexer_.skip();
    return;
  }

  const Token &token = lexer_.peek();
  error_reporter_.report<ExpectedSemicolonError>(token.source_location());
  while (lexer_.peek().type != Token::Type::end_of_file &&
         lexer_.peek().type != Token::Type::rcurly &&
         lexer_.peek().type != Token::Type::semicolon) {
    lexer_.skip();
  }
  if (lexer_.peek().type == Token::Type::semicolon) {
    lexer_.skip();
  }
}

std::unique_ptr<ast::Ast::Block> Parser::parse_block(std::optional<Token> block_owner) {
  if (lexer_.peek().type != Token::Type::lcurly) {
    const Token &token = lexer_.peek();
    error_reporter_.report<ExpectedBlockError>(
        token.source_location(), block_owner,
        ExpectedBlockError::Boundary::OpeningBrace);
    return std::make_unique<ast::Ast::Block>();
  }
  lexer_.skip();

  auto block = std::make_unique<ast::Ast::Block>();
  while (lexer_.peek().type != Token::Type::rcurly &&
         lexer_.peek().type != Token::Type::end_of_file) {
    block->append(parse_statement());
  }

  if (lexer_.peek().type == Token::Type::rcurly) {
    lexer_.skip();
    return block;
  }

  const Token &token = lexer_.peek();
  error_reporter_.report<ExpectedBlockError>(
      token.source_location(), block_owner,
      ExpectedBlockError::Boundary::ClosingBrace);
  return block;
}

std::unique_ptr<ast::Ast> Parser::parse_assignment() {
  std::unique_ptr<ast::Ast> left = parse_equality();

  if (lexer_.peek().type != Token::Type::equals) {
    return left;
  }

  const Token equals_token = lexer_.peek();
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

  error_reporter_.report<InvalidAssignmentTargetError>(equals_token.source_location());
  return left;
}

std::unique_ptr<ast::Ast> Parser::parse_equality() {
  std::unique_ptr<ast::Ast> left = parse_comparison();

  while (true) {
    const Token::Type op = lexer_.peek().type;
    if (op != Token::Type::equals_equals && op != Token::Type::bang_equals) {
      break;
    }

    lexer_.skip();
    std::unique_ptr<ast::Ast> right = parse_comparison();
    if (op == Token::Type::equals_equals) {
      left = std::make_unique<ast::Ast::Equal>(std::move(left), std::move(right));
    } else {
      left = std::make_unique<ast::Ast::NotEqual>(std::move(left), std::move(right));
    }
  }

  return left;
}

std::unique_ptr<ast::Ast> Parser::parse_comparison() {
  std::unique_ptr<ast::Ast> left = parse_additive();

  while (true) {
    const Token::Type op = lexer_.peek().type;
    if (op != Token::Type::less_than && op != Token::Type::greater_than &&
        op != Token::Type::less_than_equals &&
        op != Token::Type::greater_than_equals) {
      return left;
    }

    lexer_.skip();
    std::unique_ptr<ast::Ast> right = parse_additive();
    if (op == Token::Type::less_than) {
      left = std::make_unique<ast::Ast::LessThan>(std::move(left), std::move(right));
    } else if (op == Token::Type::greater_than) {
      left = std::make_unique<ast::Ast::GreaterThan>(std::move(left), std::move(right));
    } else if (op == Token::Type::less_than_equals) {
      left =
          std::make_unique<ast::Ast::LessThanOrEqual>(std::move(left), std::move(right));
    } else {
      left = std::make_unique<ast::Ast::GreaterThanOrEqual>(std::move(left),
                                                            std::move(right));
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
  std::unique_ptr<ast::Ast> left = parse_unary();

  while (true) {
    const Token::Type op = lexer_.peek().type;
    if (op != Token::Type::star && op != Token::Type::slash &&
        op != Token::Type::percent) {
      return left;
    }

    lexer_.skip();
    std::unique_ptr<ast::Ast> right = parse_unary();
    if (op == Token::Type::star) {
      left = std::make_unique<ast::Ast::Multiply>(std::move(left), std::move(right));
    } else if (op == Token::Type::slash) {
      left = std::make_unique<ast::Ast::Divide>(std::move(left), std::move(right));
    } else {
      left = std::make_unique<ast::Ast::Modulo>(std::move(left), std::move(right));
    }
  }
}

std::unique_ptr<ast::Ast> Parser::parse_unary() {
  if (lexer_.peek().type == Token::Type::minus) {
    lexer_.skip();
    return std::make_unique<ast::Ast::Negate>(parse_unary());
  }
  if (lexer_.peek().type == Token::Type::plus) {
    lexer_.skip();
    return std::make_unique<ast::Ast::UnaryPlus>(parse_unary());
  }
  if (lexer_.peek().type == Token::Type::bang) {
    lexer_.skip();
    return std::make_unique<ast::Ast::LogicalNot>(parse_unary());
  }
  return parse_postfix();
}

std::unique_ptr<ast::Ast> Parser::parse_postfix() {
  std::unique_ptr<ast::Ast> expr = parse_primary();

  while (true) {
    if (lexer_.peek().type == Token::Type::lparen) {
      if (expr->type != ast::Ast::Type::Variable) {
        const Token &token = lexer_.peek();
        error_reporter_.report<ExpectedVariableError>(
            token.source_location(), ExpectedVariableError::Ctx::AsFunctionCallTarget);
        break;
      }
      const std::string callee_name =
          ast::ast_cast<const ast::Ast::Variable &>(*expr).name;

      lexer_.skip();
      std::vector<std::unique_ptr<ast::Ast>> arguments;
      if (lexer_.peek().type != Token::Type::rparen) {
        while (true) {
          arguments.emplace_back(parse_expression());
          if (lexer_.peek().type != Token::Type::comma) {
            break;
          }
          lexer_.skip();
        }
      }
      consume<ExpectedClosingParenthesisError>(
          Token::Type::rparen,
          ExpectedClosingParenthesisError::Ctx::ToCloseFunctionCallArguments);
      expr = std::make_unique<ast::Ast::FunctionCall>(callee_name, std::move(arguments));
      continue;
    }

    if (lexer_.peek().type == Token::Type::lsquare) {
      lexer_.skip();
      std::unique_ptr<ast::Ast> index = parse_expression();
      consume<ExpectedClosingSquareBracketError>(
          Token::Type::rsquare,
          ExpectedClosingSquareBracketError::Ctx::ToCloseIndexExpression);
      expr = std::make_unique<ast::Ast::Index>(std::move(expr), std::move(index));
      continue;
    }

    if (lexer_.peek().type == Token::Type::dot) {
      lexer_.skip();
      if (lexer_.peek().type != Token::Type::identifier) {
        const Token& token = lexer_.peek();
        error_reporter_.report<ExpectedIdentifierError>(
            token.source_location(), ExpectedIdentifierError::Ctx::AfterDotInFieldAccess);
        break;
      }
      const std::string field_name(lexer_.peek().sv());
      lexer_.skip();
      expr = std::make_unique<ast::Ast::FieldAccess>(std::move(expr), field_name);
      continue;
    }

    if (lexer_.peek().type == Token::Type::plus_plus) {
      if (expr->type != ast::Ast::Type::Variable) {
        const Token &token = lexer_.peek();
        error_reporter_.report<ExpectedVariableError>(
            token.source_location(), ExpectedVariableError::Ctx::BeforePostfixIncrement);
        lexer_.skip();
        continue;
      }
      const std::string name = ast::ast_cast<const ast::Ast::Variable &>(*expr).name;
      lexer_.skip();
      expr = std::make_unique<ast::Ast::Increment>(
          std::make_unique<ast::Ast::Variable>(name));
      continue;
    }

    break;
  }

  return expr;
}

std::unique_ptr<ast::Ast> Parser::parse_array_literal() {
  assert(lexer_.peek().type == Token::Type::lsquare);
  lexer_.skip();

  std::vector<std::unique_ptr<ast::Ast>> elements;
  if (lexer_.peek().type != Token::Type::rsquare) {
    while (true) {
      elements.emplace_back(parse_expression());
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

std::unique_ptr<ast::Ast> Parser::parse_struct_literal() {
  assert(token_is_identifier(lexer_.peek(), "struct"));
  lexer_.skip();
  assert(lexer_.peek().type == Token::Type::lcurly);
  lexer_.skip();

  std::vector<std::pair<std::string, std::unique_ptr<ast::Ast>>> fields;
  if (lexer_.peek().type != Token::Type::rcurly) {
    while (true) {
      assert(lexer_.peek().type == Token::Type::identifier);
      std::string field_name(lexer_.peek().sv());
      lexer_.skip();
      assert(lexer_.peek().type == Token::Type::colon);
      lexer_.skip();
      fields.emplace_back(std::move(field_name), parse_expression());
      if (lexer_.peek().type != Token::Type::comma) {
        break;
      }
      lexer_.skip();
    }
  }

  assert(lexer_.peek().type == Token::Type::rcurly);
  lexer_.skip();
  return std::make_unique<ast::Ast::StructLiteral>(std::move(fields));
}

std::unique_ptr<ast::Ast> Parser::parse_primary() {
  const Token &token = lexer_.peek();
  if (token.type == Token::Type::number) {
    ast::Value value = 0;
    const std::string_view source = token.sv();
    const auto [ptr, ec] =
        std::from_chars(source.data(), source.data() + source.size(), value);
    if (ec != std::errc() || ptr != source.data() + source.size()) {
      error_reporter_.report<InvalidNumericLiteralError>(token.source_location());
      lexer_.skip();
      return std::make_unique<ast::Ast::Literal>(0);
    }
    lexer_.skip();
    return std::make_unique<ast::Ast::Literal>(value);
  }
  if (token.type == Token::Type::identifier) {
    if (token.sv() == "struct") {
      return parse_struct_literal();
    }
    const std::string name(token.sv());
    lexer_.skip();
    return std::make_unique<ast::Ast::Variable>(name);
  }
  if (token.type == Token::Type::lparen) {
    lexer_.skip();
    std::unique_ptr<ast::Ast> expr = parse_expression();
    consume<ExpectedClosingParenthesisError>(
        Token::Type::rparen, ExpectedClosingParenthesisError::Ctx::ToCloseGroupedExpression);
    return expr;
  }
  if (token.type == Token::Type::lsquare) {
    return parse_array_literal();
  }

  error_reporter_.report<ExpectedPrimaryExpressionError>(token.source_location());
  if (token.type != Token::Type::end_of_file) {
    lexer_.skip();
  }
  return std::make_unique<ast::Ast::Literal>(0);
}

}  // namespace kai
