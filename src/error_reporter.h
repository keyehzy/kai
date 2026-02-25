#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace kai {

// A half-open byte range [begin, end) into the original source string.
struct SourceLocation {
  const char* begin;
  const char* end;

  std::string_view text() const {
    return {begin, static_cast<size_t>(end - begin)};
  }
};

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

  std::string format_error() const override { return "expected expression"; }
};

struct ExpectedSemicolonError final : public Error {
  explicit ExpectedSemicolonError(SourceLocation location)
      : Error(Type::ExpectedSemicolon, location) {}

  std::string format_error() const override {
    std::string msg = "expected ';' after statement";
    const std::string_view found = location.text();
    if (found.empty()) {
      msg += " found end of input";
      return msg;
    }
    msg += " found '";
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
