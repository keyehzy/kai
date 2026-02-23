#pragma once

#include "../ast.h"
#include "../bytecode.h"

#include <initializer_list>

using namespace kai::ast;

inline std::unique_ptr<Ast> lit(int v) { return std::make_unique<Ast::Literal>(v); }

inline std::unique_ptr<Ast> var(const char *name) {
  return std::make_unique<Ast::Variable>(name);
}

inline std::unique_ptr<Ast::Variable> var_ref(const char *name) {
  return std::make_unique<Ast::Variable>(name);
}

inline std::unique_ptr<Ast::Add> add(std::unique_ptr<Ast> left,
                                     std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::Add>(std::move(left), std::move(right));
}

inline std::unique_ptr<Ast::Multiply> mul(std::unique_ptr<Ast> left,
                                          std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::Multiply>(std::move(left), std::move(right));
}

inline std::unique_ptr<Ast::Divide> div(std::unique_ptr<Ast> left,
                                        std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::Divide>(std::move(left), std::move(right));
}

inline std::unique_ptr<Ast::Modulo> mod(std::unique_ptr<Ast> left,
                                        std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::Modulo>(std::move(left), std::move(right));
}

inline std::unique_ptr<Ast::Subtract> sub(std::unique_ptr<Ast> left,
                                          std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::Subtract>(std::move(left), std::move(right));
}

inline std::unique_ptr<Ast::LessThan> lt(std::unique_ptr<Ast> left,
                                         std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::LessThan>(std::move(left), std::move(right));
}

inline std::unique_ptr<Ast::VariableDeclaration> decl(
    const char *name, std::unique_ptr<Ast> initializer) {
  return std::make_unique<Ast::VariableDeclaration>(name, std::move(initializer));
}

inline std::unique_ptr<Ast::Assignment> assign(const char *name,
                                               std::unique_ptr<Ast> value) {
  return std::make_unique<Ast::Assignment>(name, std::move(value));
}

inline std::unique_ptr<Ast::Increment> inc(const char *name) {
  return std::make_unique<Ast::Increment>(var_ref(name));
}

inline std::unique_ptr<Ast::Return> ret(std::unique_ptr<Ast> value) {
  return std::make_unique<Ast::Return>(std::move(value));
}

inline std::unique_ptr<Ast::FunctionCall> call(const char *name) {
  return std::make_unique<Ast::FunctionCall>(name);
}

inline std::unique_ptr<Ast::While> while_loop(
    std::unique_ptr<Ast::LessThan> condition, std::unique_ptr<Ast::Block> body) {
  return std::make_unique<Ast::While>(std::move(condition), std::move(body));
}

inline std::unique_ptr<Ast::IfElse> if_else(
    std::unique_ptr<Ast::LessThan> condition, std::unique_ptr<Ast::Block> body,
    std::unique_ptr<Ast::Block> else_body) {
  return std::make_unique<Ast::IfElse>(std::move(condition), std::move(body),
                                       std::move(else_body));
}

inline std::unique_ptr<Ast::ArrayLiteral> arr(std::initializer_list<int> values) {
  std::vector<std::unique_ptr<Ast>> elements;
  elements.reserve(values.size());
  for (int value : values) {
    elements.emplace_back(lit(value));
  }
  return std::make_unique<Ast::ArrayLiteral>(std::move(elements));
}

inline std::unique_ptr<Ast::Index> idx(std::unique_ptr<Ast> array_expr,
                                       std::unique_ptr<Ast> index_expr) {
  return std::make_unique<Ast::Index>(std::move(array_expr), std::move(index_expr));
}

inline std::unique_ptr<Ast::IndexAssignment> idx_assign(
    std::unique_ptr<Ast> array_expr, std::unique_ptr<Ast> index_expr,
    std::unique_ptr<Ast> value_expr) {
  return std::make_unique<Ast::IndexAssignment>(std::move(array_expr),
                                                std::move(index_expr),
                                                std::move(value_expr));
}
