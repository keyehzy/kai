#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "shape.h"
#include "token.h"

namespace kai {

// Line and column numbers (both 1-based) for a position in source.
struct LineColumn {
  int line;
  int column;
};

// Compute the line/column of `pos` within `source`.
inline LineColumn line_column(std::string_view source, const char* pos) {
  LineColumn lc{1, 1};
  const size_t stop = static_cast<size_t>(pos - source.data());
  for (size_t i = 0; i < stop && i < source.size(); ++i) {
    if (source[i] == '\n') {
      ++lc.line;
      lc.column = 1;
    } else {
      ++lc.column;
    }
  }
  return lc;
}

struct Error {
  enum class Type {
    ExpectedEndOfExpression,
    ExpectedVariable,
    InvalidAssignmentTarget,
    ExpectedIdentifier,
    ExpectedFunctionIdentifier,
    ExpectedLetVariableName,
    ExpectedStructFieldName,
    ExpectedStructFieldColon,
    ExpectedStructLiteralBrace,
    InvalidNumericLiteral,
    ExpectedPrimaryExpression,
    ExpectedSemicolon,
    ExpectedEquals,
    ExpectedOpeningParenthesis,
    ExpectedClosingParenthesis,
    ExpectedClosingSquareBracket,
    ExpectedLiteralStart,
    ExpectedBlock,
    UnexpectedChar,
    // Type errors
    TypeMismatch,
    UndefinedVariable,
    UndefinedFunction,
    WrongArgCount,
    NotAStruct,
    UndefinedField,
    NotCallable,
    NotIndexable,
  };

  Type type;
  SourceLocation location;

  Error(Type type, SourceLocation location) : type(type), location(location) {}

  virtual ~Error() = default;

  virtual std::string format_error() const = 0;
};

struct UnexpectedCharError final : public Error {
  char ch;

  UnexpectedCharError(SourceLocation location, char ch)
      : Error(Type::UnexpectedChar, location), ch(ch) {}

  std::string format_error() const override;
};

struct ExpectedEndOfExpressionError final : public Error {
  explicit ExpectedEndOfExpressionError(SourceLocation location)
      : Error(Type::ExpectedEndOfExpression, location) {}

  std::string format_error() const override;
};

struct ExpectedVariableError final : public Error {
  enum class Ctx {
    AsFunctionCallTarget,
    BeforePostfixIncrement,
  };

  Ctx ctx;

  ExpectedVariableError(SourceLocation location, Ctx ctx)
      : Error(Type::ExpectedVariable, location), ctx(ctx) {}

  std::string format_error() const override;
};

struct InvalidAssignmentTargetError final : public Error {
  explicit InvalidAssignmentTargetError(SourceLocation location)
      : Error(Type::InvalidAssignmentTarget, location) {}

  std::string format_error() const override;
};

struct ExpectedIdentifierError final : public Error {
  enum class Ctx {
    AfterDotInFieldAccess,
  };

  Ctx ctx;

  ExpectedIdentifierError(SourceLocation location, Ctx ctx)
      : Error(Type::ExpectedIdentifier, location), ctx(ctx) {}

  std::string format_error() const override;
};

struct ExpectedFunctionIdentifierError final : public Error {
  enum class Ctx {
    AfterFnKeyword,
    InParameterList,
  };

  Ctx ctx;

  ExpectedFunctionIdentifierError(SourceLocation location, Ctx ctx)
      : Error(Type::ExpectedFunctionIdentifier, location), ctx(ctx) {}

  std::string format_error() const override;
};

struct InvalidNumericLiteralError final : public Error {
  explicit InvalidNumericLiteralError(SourceLocation location)
      : Error(Type::InvalidNumericLiteral, location) {}

  std::string format_error() const override;
};

struct ExpectedLetVariableNameError final : public Error {
  explicit ExpectedLetVariableNameError(SourceLocation location)
      : Error(Type::ExpectedLetVariableName, location) {}

  std::string format_error() const override;
};

struct ExpectedStructFieldNameError final : public Error {
  explicit ExpectedStructFieldNameError(SourceLocation location)
      : Error(Type::ExpectedStructFieldName, location) {}

  std::string format_error() const override;
};

struct ExpectedStructFieldColonError final : public Error {
  std::optional<SourceLocation> field_name_location;

  explicit ExpectedStructFieldColonError(
      SourceLocation location,
      std::optional<SourceLocation> field_name_location = std::nullopt)
      : Error(Type::ExpectedStructFieldColon, location),
        field_name_location(field_name_location) {}

  std::string format_error() const override;
};

struct ExpectedStructLiteralBraceError final : public Error {
  enum class Boundary {
    OpeningBrace,
    ClosingBrace,
  };

  Boundary boundary;

  ExpectedStructLiteralBraceError(SourceLocation location, Boundary boundary)
      : Error(Type::ExpectedStructLiteralBrace, location), boundary(boundary) {}

  std::string format_error() const override;
};

struct ExpectedLiteralStartError final : public Error {
  enum class Ctx {
    ArrayLiteral,
    StructLiteral,
  };

  Ctx ctx;

  ExpectedLiteralStartError(SourceLocation location, Ctx ctx)
      : Error(Type::ExpectedLiteralStart, location), ctx(ctx) {}

  std::string format_error() const override;
};

struct ExpectedPrimaryExpressionError final : public Error {
  explicit ExpectedPrimaryExpressionError(SourceLocation location)
      : Error(Type::ExpectedPrimaryExpression, location) {}

  std::string format_error() const override;
};

struct ExpectedSemicolonError final : public Error {
  explicit ExpectedSemicolonError(SourceLocation location)
      : Error(Type::ExpectedSemicolon, location) {}

  std::string format_error() const override;
};

struct ExpectedEqualsError final : public Error {
  enum class Ctx {
    AfterLetVariableName,
  };

  Ctx ctx;
  std::optional<SourceLocation> context_location;

  ExpectedEqualsError(SourceLocation location, Ctx ctx,
                      std::optional<SourceLocation> context_location = std::nullopt)
      : Error(Type::ExpectedEquals, location), ctx(ctx), context_location(context_location) {}

  std::string format_error() const override;
};

struct ExpectedOpeningParenthesisError final : public Error {
  enum class Ctx {
    AfterWhile,
    AfterIf,
    AfterFunctionNameInDeclaration,
  };

  Ctx ctx;
  std::optional<SourceLocation> context_location;

  ExpectedOpeningParenthesisError(SourceLocation location, Ctx ctx,
                                  std::optional<SourceLocation> context_location = std::nullopt)
      : Error(Type::ExpectedOpeningParenthesis, location),
        ctx(ctx),
        context_location(context_location) {}

  std::string format_error() const override;
};

struct ExpectedClosingParenthesisError final : public Error {
  enum class Ctx {
    ToCloseWhileCondition,
    ToCloseIfCondition,
    ToCloseFunctionParameterList,
    ToCloseFunctionCallArguments,
    ToCloseGroupedExpression,
  };

  Ctx ctx;
  std::optional<SourceLocation> context_location;

  ExpectedClosingParenthesisError(SourceLocation location, Ctx ctx,
                                  std::optional<SourceLocation> context_location = std::nullopt)
      : Error(Type::ExpectedClosingParenthesis, location),
        ctx(ctx),
        context_location(context_location) {}

  std::string format_error() const override;
};

struct ExpectedClosingSquareBracketError final : public Error {
  enum class Ctx {
    ToCloseIndexExpression,
    ToCloseArrayLiteral,
  };

  Ctx ctx;

  ExpectedClosingSquareBracketError(SourceLocation location, Ctx ctx)
      : Error(Type::ExpectedClosingSquareBracket, location), ctx(ctx) {}

  std::string format_error() const override;
};

struct ExpectedBlockError final : public Error {
  enum class Boundary {
    OpeningBrace,
    ClosingBrace,
  };

  std::optional<Token> block_token;
  Boundary boundary;

  ExpectedBlockError(SourceLocation location, std::optional<Token> block_token, Boundary boundary)
      : Error(Type::ExpectedBlock, location),
        block_token(block_token),
        boundary(boundary) {}

  std::string format_error() const override;
};

// ---------------------------------------------------------------------------
// Type errors
// ---------------------------------------------------------------------------

// Type mismatch: the checker expected one type but found another.
struct TypeMismatchError final : public Error {
  enum class Ctx {
    Assignment,  // RHS shape is incompatible with the declared variable shape.
  };

  Ctx ctx;
  std::string expected;
  std::string got;

  TypeMismatchError(SourceLocation location, Ctx ctx, std::string_view expected, std::string_view got)
      : Error(Type::TypeMismatch, location),
        ctx(ctx),
        expected(std::move(expected)),
        got(std::move(got)) {}

  std::string format_error() const override;
};

// A variable name was used but is not bound in any enclosing scope.
struct UndefinedVariableError final : public Error {
  std::string name;

  UndefinedVariableError(SourceLocation location, std::string name)
      : Error(Type::UndefinedVariable, location), name(std::move(name)) {}

  std::string format_error() const override;
};

// A function was called but is not defined.
struct UndefinedFunctionError final : public Error {
  std::string name;

  UndefinedFunctionError(SourceLocation location, std::string name)
      : Error(Type::UndefinedFunction, location), name(std::move(name)) {}

  std::string format_error() const override;
};

// A call site passed the wrong number of arguments.
struct WrongArgCountError final : public Error {
  std::string name;
  size_t expected;
  size_t got;

  WrongArgCountError(SourceLocation location, std::string name,
                     size_t expected, size_t got)
      : Error(Type::WrongArgCount, location),
        name(std::move(name)),
        expected(expected),
        got(got) {}

  std::string format_error() const override;
};

// Field access was attempted on a value whose type is not a struct.
struct NotAStructError final : public Error {
  std::string actual_type;

  NotAStructError(SourceLocation location, std::string_view actual_type)
      : Error(Type::NotAStruct, location), actual_type(std::move(actual_type)) {}

  std::string format_error() const override;
};

// A struct field name that does not exist on the struct type was accessed.
struct UndefinedFieldError final : public Error {
  std::string field;

  UndefinedFieldError(SourceLocation location, std::string field)
      : Error(Type::UndefinedField, location), field(std::move(field)) {}

  std::string format_error() const override;
};

// A call was attempted on a value that is not a function.
struct NotCallableError final : public Error {
  Shape::Kind kind;

  NotCallableError(SourceLocation location, Shape::Kind kind)
      : Error(Type::NotCallable, location), kind(kind) {}

  std::string format_error() const override;
};

// An index operation was attempted on a value that is not an array.
struct NotIndexableError final : public Error {
  Shape::Kind kind;

  NotIndexableError(SourceLocation location, Shape::Kind kind)
      : Error(Type::NotIndexable, location), kind(kind) {}

  std::string format_error() const override;
};

// ---------------------------------------------------------------------------

class ErrorReporter {
 public:
  template <typename ErrorT, typename... Args>
  void report(Args&&... args) {
    static_assert(std::is_base_of_v<Error, ErrorT>,
                  "ErrorReporter::report requires an Error subtype");
    errors_.push_back(std::make_unique<ErrorT>(std::forward<Args>(args)...));
  }

  bool has_errors() const { return !errors_.empty(); }

  const std::vector<std::unique_ptr<Error>>& errors() const { return errors_; }

 private:
  std::vector<std::unique_ptr<Error>> errors_;
};

}  // namespace kai
