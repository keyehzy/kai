#pragma once

#include <cassert>
#include <string>
#include <unordered_set>

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

}  // namespace kai
