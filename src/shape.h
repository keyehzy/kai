#pragma once

#include "derived_cast.h"

#include <string>
#include <unordered_set>

namespace kai {

struct Shape {
  enum class Kind {
    Unknown,
    Non_Struct,
    Struct_Literal,
    Array,
    Function,
  };

  struct Unknown;
  struct Non_Struct;
  struct Struct_Literal;
  struct Array;
  struct Function;

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

struct Shape::Array final : public Shape {
  Array() : Shape(Kind::Array) {}
  std::string describe() const override { return "Array"; }
};

struct Shape::Function final : public Shape {
  Function() : Shape(Kind::Function) {}
  std::string describe() const override { return "Function"; }
};

inline std::string_view describe(Shape::Kind kind) {
  switch (kind) {
    case Shape::Kind::Unknown:        return "Unknown";
    case Shape::Kind::Non_Struct:     return "Non_Struct";
    case Shape::Kind::Struct_Literal: return "Struct_Literal";
    case Shape::Kind::Array:          return "Array";
    case Shape::Kind::Function:       return "Function";
  }
}

}  // namespace kai
