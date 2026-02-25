#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

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
    ExpectedStructFieldColon,
    InvalidNumericLiteral,
    ExpectedPrimaryExpression,
    ExpectedSemicolon,
    ExpectedEquals,
    ExpectedOpeningParenthesis,
    ExpectedClosingParenthesis,
    ExpectedClosingSquareBracket,
    ExpectedBlock,
    UnexpectedChar,
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

  std::string format_error() const override {
    std::string msg = "unexpected character '";
    msg += ch;
    msg += "'";
    return msg;
  }
};

struct ExpectedEndOfExpressionError final : public Error {
  explicit ExpectedEndOfExpressionError(SourceLocation location)
      : Error(Type::ExpectedEndOfExpression, location) {}

  std::string format_error() const override { return "expected end of expression"; }
};

struct ExpectedVariableError final : public Error {
  enum class Ctx {
    AsFunctionCallTarget,
    BeforePostfixIncrement,
  };

  Ctx ctx;

  ExpectedVariableError(SourceLocation location, Ctx ctx)
      : Error(Type::ExpectedVariable, location), ctx(ctx) {}

  std::string format_error() const override {
    std::string msg = "expected variable";
    switch (ctx) {
      case Ctx::AsFunctionCallTarget:
        msg += " as function call target";
        break;
      case Ctx::BeforePostfixIncrement:
        msg += " before postfix '++'";
        break;
    }

    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
};

struct InvalidAssignmentTargetError final : public Error {
  explicit InvalidAssignmentTargetError(SourceLocation location)
      : Error(Type::InvalidAssignmentTarget, location) {}

  std::string format_error() const override {
    std::string msg =
        "invalid assignment target; expected variable or index expression before '='";
    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
};

struct ExpectedIdentifierError final : public Error {
  enum class Ctx {
    AfterDotInFieldAccess,
  };

  Ctx ctx;

  ExpectedIdentifierError(SourceLocation location, Ctx ctx)
      : Error(Type::ExpectedIdentifier, location), ctx(ctx) {}

  std::string format_error() const override {
    std::string msg = "expected identifier";
    switch (ctx) {
      case Ctx::AfterDotInFieldAccess:
        msg += " after '.' in field access";
        break;
    }

    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
};

struct ExpectedFunctionIdentifierError final : public Error {
  enum class Ctx {
    AfterFnKeyword,
    InParameterList,
  };

  Ctx ctx;

  ExpectedFunctionIdentifierError(SourceLocation location, Ctx ctx)
      : Error(Type::ExpectedFunctionIdentifier, location), ctx(ctx) {}

  std::string format_error() const override {
    std::string msg = "expected ";
    switch (ctx) {
      case Ctx::AfterFnKeyword:
        msg += "function name after 'fn'";
        break;
      case Ctx::InParameterList:
        msg += "parameter name in function declaration";
        break;
    }

    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
};

struct InvalidNumericLiteralError final : public Error {
  explicit InvalidNumericLiteralError(SourceLocation location)
      : Error(Type::InvalidNumericLiteral, location) {}

  std::string format_error() const override {
    std::string msg = "invalid numeric literal";
    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }

    msg += " '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
};

struct ExpectedLetVariableNameError final : public Error {
  explicit ExpectedLetVariableNameError(SourceLocation location)
      : Error(Type::ExpectedLetVariableName, location) {}

  std::string format_error() const override {
    std::string msg = "expected variable name after 'let'";
    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
};

struct ExpectedStructFieldColonError final : public Error {
  std::optional<SourceLocation> field_name_location;

  explicit ExpectedStructFieldColonError(
      SourceLocation location,
      std::optional<SourceLocation> field_name_location = std::nullopt)
      : Error(Type::ExpectedStructFieldColon, location),
        field_name_location(field_name_location) {}

  std::string format_error() const override {
    std::string msg = "expected ':'";
    if (field_name_location.has_value() && !field_name_location->text().empty()) {
      msg += " after field name '";
      msg += std::string(field_name_location->text());
      msg += "'";
    } else {
      msg += " after struct field name";
    }
    msg += " in struct literal";

    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
};

struct ExpectedPrimaryExpressionError final : public Error {
  explicit ExpectedPrimaryExpressionError(SourceLocation location)
      : Error(Type::ExpectedPrimaryExpression, location) {}

  std::string format_error() const override {
    std::string msg = "expected primary expression";
    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
};

struct ExpectedSemicolonError final : public Error {
  explicit ExpectedSemicolonError(SourceLocation location)
      : Error(Type::ExpectedSemicolon, location) {}

  std::string format_error() const override {
    std::string msg = "expected ';' after statement";
    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
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

  std::string format_error() const override {
    std::string msg = "expected '='";
    switch (ctx) {
      case Ctx::AfterLetVariableName: {
        const std::string_view variable_name_text =
            context_location.has_value() && !context_location->text().empty()
                ? context_location->text()
                : std::string_view("name");
        msg += " after variable '";
        msg += std::string(variable_name_text);
        msg += "' in 'let' declaration";
        break;
      }
    }

    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
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

  std::string format_error() const override {
    std::string msg = "expected '('";
    switch (ctx) {
      case Ctx::AfterWhile: {
        const std::string_view while_text =
            context_location.has_value() && !context_location->text().empty()
                ? context_location->text()
                : std::string_view("while");
        msg += " after '";
        msg += std::string(while_text);
        msg += "'";
        break;
      }
      case Ctx::AfterIf: {
        const std::string_view if_text =
            context_location.has_value() && !context_location->text().empty()
                ? context_location->text()
                : std::string_view("if");
        msg += " after '";
        msg += std::string(if_text);
        msg += "'";
        break;
      }
      case Ctx::AfterFunctionNameInDeclaration:
      {
        const std::string_view function_name_text =
            context_location.has_value() && !context_location->text().empty()
                ? context_location->text()
                : std::string_view("function name");
        msg += " after function name '";
        msg += std::string(function_name_text);
        msg += "' in declaration";
        break;
      }
    }

    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
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

  std::string format_error() const override {
    std::string msg = "expected ')'";
    switch (ctx) {
      case Ctx::ToCloseWhileCondition: {
        const std::string_view while_text =
            context_location.has_value() && !context_location->text().empty()
                ? context_location->text()
                : std::string_view("while");
        msg += " to close '";
        msg += std::string(while_text);
        msg += "' condition";
        break;
      }
      case Ctx::ToCloseIfCondition: {
        const std::string_view if_text =
            context_location.has_value() && !context_location->text().empty()
                ? context_location->text()
                : std::string_view("if");
        msg += " to close '";
        msg += std::string(if_text);
        msg += "' condition";
        break;
      }
      case Ctx::ToCloseFunctionParameterList:
        msg += " to close function parameter list";
        break;
      case Ctx::ToCloseFunctionCallArguments:
        msg += " to close function call arguments";
        break;
      case Ctx::ToCloseGroupedExpression:
        msg += " to close grouped expression";
        break;
    }

    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
};

struct ExpectedClosingSquareBracketError final : public Error {
  enum class Ctx {
    ToCloseIndexExpression,
  };

  Ctx ctx;

  ExpectedClosingSquareBracketError(SourceLocation location, Ctx ctx)
      : Error(Type::ExpectedClosingSquareBracket, location), ctx(ctx) {}

  std::string format_error() const override {
    std::string msg = "expected ']'";
    switch (ctx) {
      case Ctx::ToCloseIndexExpression:
        msg += " to close index expression";
        break;
    }

    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
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

  std::string format_error() const override {
    std::string msg =
        (boundary == Boundary::OpeningBrace)
            ? "expected '{' to start "
            : "expected '}' to close ";
    if (block_token.has_value()) {
      msg += (*block_token).sv();
      msg += " ";
    }
    msg += "block";
    const std::string_view found = location.text();
    if (found.empty()) {
      msg += ", found end of input";
      return msg;
    }
    msg += ", found '";
    msg += std::string(found);
    msg += "'";
    return msg;
  }
};

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
