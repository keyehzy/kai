#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include "ast.h"
#include "lexer.h"

namespace kai {

class Parser {
 public:
  explicit Parser(std::string_view input, ErrorReporter& error_reporter);

  std::unique_ptr<ast::Ast::Block> parse_program();
  std::unique_ptr<ast::Ast> parse_expression();

 private:
  std::unique_ptr<ast::Ast> parse_statement();
  std::unique_ptr<ast::Ast::Block> parse_block(std::optional<Token> block_owner);
  std::unique_ptr<ast::Ast> parse_assignment();
  std::unique_ptr<ast::Ast> parse_equality();
  std::unique_ptr<ast::Ast> parse_comparison();
  std::unique_ptr<ast::Ast> parse_additive();
  std::unique_ptr<ast::Ast> parse_multiplicative();
  std::unique_ptr<ast::Ast> parse_unary();
  std::unique_ptr<ast::Ast> parse_postfix();
  std::unique_ptr<ast::Ast> parse_array_literal();
  std::unique_ptr<ast::Ast> parse_struct_literal();
  std::unique_ptr<ast::Ast> parse_primary();
  bool consume_closing_parenthesis(std::string_view context);
  void consume_statement_terminator();

  ErrorReporter& error_reporter_;
  Lexer lexer_;
};

}  // namespace kai
