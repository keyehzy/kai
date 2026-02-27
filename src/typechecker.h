#pragma once

#include "ast.h"
#include "error_reporter.h"
#include "shape.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kai {

class Env {
public:
  struct VariableBinding {
    Shape* shape = nullptr;
    bool may_reference_local = false;
    bool may_reference_argument = false;
    std::unordered_set<size_t> referenced_argument_indices;
  };

  void push_scope();
  void pop_scope();
  void enter_function_scope();
  void exit_function_scope();
  bool inside_function() const;

  void bind_variable(const std::string& name, Shape* shape,
                     bool may_reference_local = false,
                     bool may_reference_argument = false,
                     std::unordered_set<size_t> referenced_argument_indices = {});
  std::optional<VariableBinding> lookup_variable(const std::string& name) const;
  VariableBinding* lookup_variable_mut(const std::string& name);

  void declare_function(const std::string& name, size_t arity);
  std::optional<size_t> lookup_function(const std::string& name);

private:
  size_t variable_lookup_floor() const;

  std::vector<std::unordered_map<std::string, VariableBinding>> var_scopes_;
  std::vector<size_t> function_scope_starts_;
  std::unordered_map<std::string, size_t> functions_;
};

class TypeChecker {
 public:
  explicit TypeChecker(ErrorReporter& reporter);

  void visit_program(const Ast::Block& program);

 private:
  struct ExprInfo {
    Shape* shape = nullptr;
    bool may_reference_local = false;
    bool may_reference_argument = false;
    std::unordered_set<size_t> referenced_argument_indices;
  };

  struct FunctionSummary {
    size_t arity = 0;
    bool returns_local_reference = false;
    std::unordered_set<size_t> returned_argument_indices;
  };

  ErrorReporter& reporter_;
  Env env_;
  std::vector<std::unique_ptr<Shape>> arena_;
  std::unordered_map<std::string, FunctionSummary> function_summaries_;
  std::vector<std::string> function_stack_;

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
  ExprInfo visit_expression(const Ast* node);
};

}  // namespace kai
