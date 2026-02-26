#pragma once

#include "ast.h"
#include "error_reporter.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kai {

struct Shape {
  enum class Kind {
    Unknown,
    NonStruct,
    Struct,
  };

  Kind kind = Kind::Unknown;
  std::unordered_set<std::string> fields;

  static Shape unknown();
  static Shape non_struct();
  static Shape struct_shape(std::unordered_set<std::string> fields);
  std::string describe() const;
};


class Env {
public:
  void push_scope();
  void pop_scope();
  void bind_variable(const std::string& name, Shape shape);
  Shape* lookup_variable(const std::string& name);

  void declare_function(const std::string& name, size_t arity);
  size_t* lookup_function(const std::string& name);

private:
  std::vector<std::unordered_map<std::string, Shape>> var_scopes_;
  std::unordered_map<std::string, size_t> functions_;
};

class TypeChecker {
 public:
  explicit TypeChecker(ErrorReporter& reporter);

  void visit_program(const ast::Ast::Block& program);

 private:
  ErrorReporter& reporter_;
  Env env_;

  static SourceLocation no_loc();

  void visit_statement(const ast::Ast* node);
  void visit_block(const ast::Ast::Block& block);
  Shape visit_expression(const ast::Ast* node);
};

}  // namespace kai
