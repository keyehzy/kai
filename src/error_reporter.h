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
    ExpectedExpression,
    ExpectedSemicolon,
    ExpectedOpeningParenthesis,
    ExpectedClosingParenthesis,
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

struct ExpectedExpressionError final : public Error {
  explicit ExpectedExpressionError(SourceLocation location)
      : Error(Type::ExpectedExpression, location) {}

  std::string format_error() const override {
    std::string msg = "expected expression";
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
        msg += " after function name in declaration";
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
