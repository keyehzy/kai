#pragma once

#include "ast.h"
#include "error_reporter.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace kai {

struct Shape {
  enum class Kind {
    Unknown,
    Non_Struct,
    Struct_Literal,
  };

  struct Unknown;
  struct Non_Struct;
  struct Struct_Literal;

  explicit Shape(Kind kind) : kind(kind) {}
  virtual ~Shape() = default;

  Kind kind;

  virtual std::string describe() const = 0;
};

struct Shape::Unknown final : public Shape {
  Unknown() : Shape(Kind::Unknown) {}
  std::string describe() const override { return "Unknown"; }
};

struct Shape::Non_Struct final : public Shape {
  Non_Struct() : Shape(Kind::Non_Struct) {}
  std::string describe() const override { return "Non_Struct"; }
};

struct Shape::Struct_Literal final : public Shape {
  explicit Struct_Literal(std::unordered_set<std::string> fields)
      : Shape(Kind::Struct_Literal), fields_(std::move(fields)) {}
  std::string describe() const override { return "Struct_Literal"; }
  std::unordered_set<std::string> fields_;
};

template <typename T>
T& derived_cast(Shape& shape) {
  assert(dynamic_cast<T*>(&shape) != nullptr);
  return static_cast<T&>(shape);
}

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

  void visit_program(const ast::Ast::Block& program);

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

  void visit_statement(const ast::Ast* node);
  void visit_block(const ast::Ast::Block& block);
  Shape* visit_expression(const ast::Ast* node);
};

}  // namespace kai
