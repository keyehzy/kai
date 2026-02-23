#pragma once

#include <memory>
#include <string_view>

#include "ast.h"
#include "lexer.h"

namespace kai {

class Parser {
 public:
  explicit Parser(std::string_view input);

  std::unique_ptr<ast::Ast> parse_expression();

 private:
  std::unique_ptr<ast::Ast> parse_equality();
  std::unique_ptr<ast::Ast> parse_additive();
  std::unique_ptr<ast::Ast> parse_multiplicative();
  std::unique_ptr<ast::Ast> parse_primary();

  Lexer lexer_;
};

}  // namespace kai
