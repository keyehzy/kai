#pragma once

#include "derived_cast.h"

#include <string>
#include <string_view>
#include <unordered_set>

namespace kai {

struct Shape {
  enum class Kind {
    Unknown,
    Non_Struct,
    Struct_Literal,
    Array,
    Function,
    Pointer,
  };

  struct Unknown;
  struct Non_Struct;
  struct Struct_Literal;
  struct Array;
  struct Function;
  struct Pointer;

  explicit Shape(Kind kind) : kind(kind) {}
  virtual ~Shape() = default;

  Kind kind;
};

struct Shape::Unknown final : public Shape {
  Unknown() : Shape(Kind::Unknown) {}
};

struct Shape::Non_Struct final : public Shape {
  Non_Struct() : Shape(Kind::Non_Struct) {}
};

struct Shape::Struct_Literal final : public Shape {
  explicit Struct_Literal(std::unordered_set<std::string> fields)
      : Shape(Kind::Struct_Literal), fields_(std::move(fields)) {}
  std::unordered_set<std::string> fields_;
};

struct Shape::Array final : public Shape {
  Array() : Shape(Kind::Array) {}
};

struct Shape::Function final : public Shape {
  Function() : Shape(Kind::Function) {}
};

struct Shape::Pointer final : public Shape {
  explicit Pointer(Shape* pointee) : Shape(Kind::Pointer), pointee_(pointee) {}

  Shape* pointee_;
};

std::string_view describe(Shape::Kind kind);

}  // namespace kai
