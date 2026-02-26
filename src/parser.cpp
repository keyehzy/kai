#include "parser.h"

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

std::unique_ptr<Ast::Block> Parser::parse_program() {
  auto program = std::make_unique<Ast::Block>();
  while (lexer_.peek().type != Token::Type::end_of_file) {
    program->append(parse_statement());
  }
  return program;
}

std::unique_ptr<Ast> Parser::parse_expression() {
  return parse_assignment();
}

std::unique_ptr<Ast> Parser::parse_statement() {
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
      return std::make_unique<Ast::Literal>(0);
    }

    const Token variable_name_token = lexer_.peek();
    const std::string name(variable_name_token.sv());
    lexer_.skip();
    consume<ExpectedEqualsError>(Token::Type::equals,
                                 ExpectedEqualsError::Ctx::AfterLetVariableName,
                                 variable_name_token.source_location());
    std::unique_ptr<Ast> initializer = parse_expression();
    consume_statement_terminator();
    return std::make_unique<Ast::VariableDeclaration>(name, std::move(initializer));
  }

  if (token_is_identifier(token, "while")) {
    const Token while_token = token;
    lexer_.skip();
    consume<ExpectedOpeningParenthesisError>(Token::Type::lparen,
                                             ExpectedOpeningParenthesisError::Ctx::AfterWhile,
                                             while_token.source_location());
    std::unique_ptr<Ast> condition = parse_expression();
    consume<ExpectedClosingParenthesisError>(
        Token::Type::rparen, ExpectedClosingParenthesisError::Ctx::ToCloseWhileCondition,
        while_token.source_location());
    std::unique_ptr<Ast::Block> body = parse_block(while_token);
    return std::make_unique<Ast::While>(std::move(condition), std::move(body));
  }

  if (token_is_identifier(token, "if")) {
    const Token if_token = token;
    lexer_.skip();
    consume<ExpectedOpeningParenthesisError>(Token::Type::lparen,
                                             ExpectedOpeningParenthesisError::Ctx::AfterIf,
                                             if_token.source_location());
    std::unique_ptr<Ast> condition = parse_expression();
    consume<ExpectedClosingParenthesisError>(
        Token::Type::rparen, ExpectedClosingParenthesisError::Ctx::ToCloseIfCondition,
        if_token.source_location());

    std::unique_ptr<Ast::Block> body = parse_block(if_token);
    std::unique_ptr<Ast::Block> else_body;
    if (token_is_identifier(lexer_.peek(), "else")) {
      const Token else_token = lexer_.peek();
      lexer_.skip();
      else_body = parse_block(else_token);
    } else {
      else_body = std::make_unique<Ast::Block>();
    }

    return std::make_unique<Ast::IfElse>(std::move(condition), std::move(body),
                                               std::move(else_body));
  }

  if (token_is_identifier(token, "return")) {
    lexer_.skip();
    std::unique_ptr<Ast> value = parse_expression();
    consume_statement_terminator();
    return std::make_unique<Ast::Return>(std::move(value));
  }

  if (token_is_identifier(token, "fn")) {
    const Token fn_token = token;
    lexer_.skip();
    std::string name;
    std::optional<SourceLocation> function_name_location;
    if (lexer_.peek().type != Token::Type::identifier) {
      const Token& missing_name_token = lexer_.peek();
      error_reporter_.report<ExpectedFunctionIdentifierError>(
          missing_name_token.source_location(),
          ExpectedFunctionIdentifierError::Ctx::AfterFnKeyword);
    } else {
      const Token function_name_token = lexer_.peek();
      function_name_location = function_name_token.source_location();
      name = std::string(function_name_token.sv());
      lexer_.skip();
    }

    consume<ExpectedOpeningParenthesisError>(
        Token::Type::lparen,
        ExpectedOpeningParenthesisError::Ctx::AfterFunctionNameInDeclaration,
        function_name_location);
    std::vector<std::string> parameters;
    if (lexer_.peek().type != Token::Type::rparen) {
      while (true) {
        if (lexer_.peek().type != Token::Type::identifier) {
          const Token& token = lexer_.peek();
          error_reporter_.report<ExpectedFunctionIdentifierError>(
              token.source_location(),
              ExpectedFunctionIdentifierError::Ctx::InParameterList);
          while (lexer_.peek().type != Token::Type::end_of_file &&
                 lexer_.peek().type != Token::Type::rparen &&
                 lexer_.peek().type != Token::Type::comma) {
            lexer_.skip();
          }
          if (lexer_.peek().type == Token::Type::comma) {
            lexer_.skip();
            continue;
          }
          break;
        }
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

    std::unique_ptr<Ast::Block> body = parse_block(fn_token);
    return std::make_unique<Ast::FunctionDeclaration>(name, std::move(parameters),
                                                           std::move(body));
  }

  if (token.type == Token::Type::lcurly) {
    return parse_block(std::nullopt);
  }

  std::unique_ptr<Ast> expr = parse_expression();
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

std::unique_ptr<Ast::Block> Parser::parse_block(std::optional<Token> block_owner) {
  if (lexer_.peek().type != Token::Type::lcurly) {
    const Token &token = lexer_.peek();
    error_reporter_.report<ExpectedBlockError>(
        token.source_location(), block_owner,
        ExpectedBlockError::Boundary::OpeningBrace);
    return std::make_unique<Ast::Block>();
  }
  lexer_.skip();

  auto block = std::make_unique<Ast::Block>();
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

std::unique_ptr<Ast> Parser::parse_assignment() {
  std::unique_ptr<Ast> left = parse_equality();

  if (lexer_.peek().type != Token::Type::equals) {
    return left;
  }

  const Token equals_token = lexer_.peek();
  lexer_.skip();
  std::unique_ptr<Ast> value = parse_assignment();

  if (left->type == Ast::Type::Variable) {
    const std::string name = derived_cast<const Ast::Variable &>(*left).name;
    return std::make_unique<Ast::Assignment>(name, std::move(value));
  }

  if (left->type == Ast::Type::Index) {
    auto &index = derived_cast<Ast::Index &>(*left);
    return std::make_unique<Ast::IndexAssignment>(
        std::move(index.array), std::move(index.index), std::move(value));
  }

  error_reporter_.report<InvalidAssignmentTargetError>(equals_token.source_location());
  return left;
}

std::unique_ptr<Ast> Parser::parse_equality() {
  std::unique_ptr<Ast> left = parse_comparison();

  while (true) {
    const Token::Type op = lexer_.peek().type;
    if (op != Token::Type::equals_equals && op != Token::Type::bang_equals) {
      break;
    }

    lexer_.skip();
    std::unique_ptr<Ast> right = parse_comparison();
    if (op == Token::Type::equals_equals) {
      left = std::make_unique<Ast::Equal>(std::move(left), std::move(right));
    } else {
      left = std::make_unique<Ast::NotEqual>(std::move(left), std::move(right));
    }
  }

  return left;
}

std::unique_ptr<Ast> Parser::parse_comparison() {
  std::unique_ptr<Ast> left = parse_additive();

  while (true) {
    const Token::Type op = lexer_.peek().type;
    if (op != Token::Type::less_than && op != Token::Type::greater_than &&
        op != Token::Type::less_than_equals &&
        op != Token::Type::greater_than_equals) {
      return left;
    }

    lexer_.skip();
    std::unique_ptr<Ast> right = parse_additive();
    if (op == Token::Type::less_than) {
      left = std::make_unique<Ast::LessThan>(std::move(left), std::move(right));
    } else if (op == Token::Type::greater_than) {
      left = std::make_unique<Ast::GreaterThan>(std::move(left), std::move(right));
    } else if (op == Token::Type::less_than_equals) {
      left =
          std::make_unique<Ast::LessThanOrEqual>(std::move(left), std::move(right));
    } else {
      left = std::make_unique<Ast::GreaterThanOrEqual>(std::move(left),
                                                            std::move(right));
    }
  }
}

std::unique_ptr<Ast> Parser::parse_additive() {
  std::unique_ptr<Ast> left = parse_multiplicative();

  while (true) {
    const Token::Type op = lexer_.peek().type;
    if (op != Token::Type::plus && op != Token::Type::minus) {
      return left;
    }

    lexer_.skip();
    std::unique_ptr<Ast> right = parse_multiplicative();
    if (op == Token::Type::plus) {
      left = std::make_unique<Ast::Add>(std::move(left), std::move(right));
    } else {
      left = std::make_unique<Ast::Subtract>(std::move(left), std::move(right));
    }
  }
}

std::unique_ptr<Ast> Parser::parse_multiplicative() {
  std::unique_ptr<Ast> left = parse_unary();

  while (true) {
    const Token::Type op = lexer_.peek().type;
    if (op != Token::Type::star && op != Token::Type::slash &&
        op != Token::Type::percent) {
      return left;
    }

    lexer_.skip();
    std::unique_ptr<Ast> right = parse_unary();
    if (op == Token::Type::star) {
      left = std::make_unique<Ast::Multiply>(std::move(left), std::move(right));
    } else if (op == Token::Type::slash) {
      left = std::make_unique<Ast::Divide>(std::move(left), std::move(right));
    } else {
      left = std::make_unique<Ast::Modulo>(std::move(left), std::move(right));
    }
  }
}

std::unique_ptr<Ast> Parser::parse_unary() {
  if (lexer_.peek().type == Token::Type::minus) {
    lexer_.skip();
    return std::make_unique<Ast::Negate>(parse_unary());
  }
  if (lexer_.peek().type == Token::Type::plus) {
    lexer_.skip();
    return std::make_unique<Ast::UnaryPlus>(parse_unary());
  }
  if (lexer_.peek().type == Token::Type::bang) {
    lexer_.skip();
    return std::make_unique<Ast::LogicalNot>(parse_unary());
  }
  return parse_postfix();
}

std::unique_ptr<Ast> Parser::parse_postfix() {
  std::unique_ptr<Ast> expr = parse_primary();

  while (true) {
    if (lexer_.peek().type == Token::Type::lparen) {
      if (expr->type != Ast::Type::Variable) {
        const Token &token = lexer_.peek();
        error_reporter_.report<ExpectedVariableError>(
            token.source_location(), ExpectedVariableError::Ctx::AsFunctionCallTarget);
        break;
      }
      const std::string callee_name =
          derived_cast<const Ast::Variable &>(*expr).name;

      lexer_.skip();
      std::vector<std::unique_ptr<Ast>> arguments;
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
      expr = std::make_unique<Ast::FunctionCall>(callee_name, std::move(arguments));
      continue;
    }

    if (lexer_.peek().type == Token::Type::lsquare) {
      lexer_.skip();
      std::unique_ptr<Ast> index = parse_expression();
      consume<ExpectedClosingSquareBracketError>(
          Token::Type::rsquare,
          ExpectedClosingSquareBracketError::Ctx::ToCloseIndexExpression);
      expr = std::make_unique<Ast::Index>(std::move(expr), std::move(index));
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
      expr = std::make_unique<Ast::FieldAccess>(std::move(expr), field_name);
      continue;
    }

    if (lexer_.peek().type == Token::Type::plus_plus) {
      if (expr->type != Ast::Type::Variable) {
        const Token &token = lexer_.peek();
        error_reporter_.report<ExpectedVariableError>(
            token.source_location(), ExpectedVariableError::Ctx::BeforePostfixIncrement);
        lexer_.skip();
        continue;
      }
      const std::string name = derived_cast<const Ast::Variable &>(*expr).name;
      lexer_.skip();
      expr = std::make_unique<Ast::Increment>(
          std::make_unique<Ast::Variable>(name));
      continue;
    }

    break;
  }

  return expr;
}

std::unique_ptr<Ast> Parser::parse_array_literal() {
  if (lexer_.peek().type != Token::Type::lsquare) {
    const Token& token = lexer_.peek();
    error_reporter_.report<ExpectedLiteralStartError>(
        token.source_location(), ExpectedLiteralStartError::Ctx::ArrayLiteral);
    if (token.type != Token::Type::end_of_file) {
      lexer_.skip();
    }
    return std::make_unique<Ast::ArrayLiteral>(
        std::vector<std::unique_ptr<Ast>>{});
  }
  lexer_.skip();

  std::vector<std::unique_ptr<Ast>> elements;
  if (lexer_.peek().type != Token::Type::rsquare) {
    while (true) {
      elements.emplace_back(parse_expression());
      if (lexer_.peek().type != Token::Type::comma) {
        break;
      }
      lexer_.skip();
    }
  }

  consume<ExpectedClosingSquareBracketError>(
      Token::Type::rsquare, ExpectedClosingSquareBracketError::Ctx::ToCloseArrayLiteral);

  return std::make_unique<Ast::ArrayLiteral>(std::move(elements));
}

std::unique_ptr<Ast> Parser::parse_struct_literal() {
  if (!token_is_identifier(lexer_.peek(), "struct")) {
    const Token& token = lexer_.peek();
    error_reporter_.report<ExpectedLiteralStartError>(
        token.source_location(), ExpectedLiteralStartError::Ctx::StructLiteral);
    if (token.type != Token::Type::end_of_file) {
      lexer_.skip();
    }
    return std::make_unique<Ast::StructLiteral>(
        std::vector<std::pair<std::string, std::unique_ptr<Ast>>>{});
  }
  lexer_.skip();
  std::vector<std::pair<std::string, std::unique_ptr<Ast>>> fields;
  if (lexer_.peek().type != Token::Type::lcurly) {
    const Token& token = lexer_.peek();
    error_reporter_.report<ExpectedStructLiteralBraceError>(
        token.source_location(), ExpectedStructLiteralBraceError::Boundary::OpeningBrace);
    return std::make_unique<Ast::StructLiteral>(std::move(fields));
  }
  lexer_.skip();

  if (lexer_.peek().type != Token::Type::rcurly) {
    while (true) {
      if (lexer_.peek().type != Token::Type::identifier) {
        const Token& token = lexer_.peek();
        error_reporter_.report<ExpectedStructFieldNameError>(token.source_location());
        while (lexer_.peek().type != Token::Type::end_of_file &&
               lexer_.peek().type != Token::Type::rcurly &&
               lexer_.peek().type != Token::Type::comma) {
          lexer_.skip();
        }
        if (lexer_.peek().type == Token::Type::comma) {
          lexer_.skip();
          continue;
        }
        break;
      }
      const Token field_name_token = lexer_.peek();
      std::string field_name(field_name_token.sv());
      lexer_.skip();
      consume<ExpectedStructFieldColonError>(Token::Type::colon,
                                             field_name_token.source_location());
      fields.emplace_back(std::move(field_name), parse_expression());
      if (lexer_.peek().type != Token::Type::comma) {
        break;
      }
      lexer_.skip();
    }
  }

  if (lexer_.peek().type != Token::Type::rcurly) {
    const Token& token = lexer_.peek();
    error_reporter_.report<ExpectedStructLiteralBraceError>(
        token.source_location(), ExpectedStructLiteralBraceError::Boundary::ClosingBrace);
    return std::make_unique<Ast::StructLiteral>(std::move(fields));
  }
  lexer_.skip();
  return std::make_unique<Ast::StructLiteral>(std::move(fields));
}

std::unique_ptr<Ast> Parser::parse_primary() {
  const Token &token = lexer_.peek();
  if (token.type == Token::Type::number) {
    Value value = 0;
    const std::string_view source = token.sv();
    const auto [ptr, ec] =
        std::from_chars(source.data(), source.data() + source.size(), value);
    if (ec != std::errc() || ptr != source.data() + source.size()) {
      error_reporter_.report<InvalidNumericLiteralError>(token.source_location());
      lexer_.skip();
      return std::make_unique<Ast::Literal>(0);
    }
    lexer_.skip();
    return std::make_unique<Ast::Literal>(value);
  }
  if (token.type == Token::Type::identifier) {
    if (token.sv() == "struct") {
      return parse_struct_literal();
    }
    const std::string name(token.sv());
    lexer_.skip();
    return std::make_unique<Ast::Variable>(name);
  }
  if (token.type == Token::Type::lparen) {
    lexer_.skip();
    std::unique_ptr<Ast> expr = parse_expression();
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
  return std::make_unique<Ast::Literal>(0);
}

}  // namespace kai
