#pragma once

#include "ast.h"
#include "error_reporter.h"
#include "shape.h"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace kai {

class Env {
public:
  void push_scope();
  void pop_scope();
  void bind_variable(const std::string& name, Shape* shape);
  Shape** lookup_variable(const std::string& name);

  void declare_function(const std::string& name, size_t arity);
  size_t* lookup_function(const std::string& name);

private:
  std::vector<std::unordered_map<std::string, Shape*>> var_scopes_;
  std::unordered_map<std::string, size_t> functions_;
};

class TypeChecker {
 public:
  explicit TypeChecker(ErrorReporter& reporter);

  void visit_program(const Ast::Block& program);

 private:
  ErrorReporter& reporter_;
  Env env_;
  std::vector<std::unique_ptr<Shape>> arena_;

  template <typename T, typename... Args>
  Shape* make_shape(Args&&... args) {
    auto s = std::make_unique<T>(std::forward<Args>(args)...);
    auto* ptr = s.get();
    arena_.push_back(std::move(s));
    return ptr;
  }

  static SourceLocation no_loc();

  void visit_statement(const Ast* node);
  void visit_block(const Ast::Block& block);
  Shape* visit_expression(const Ast* node);
};

}  // namespace kai
