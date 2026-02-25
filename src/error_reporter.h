#pragma once

#include <string>
#include <string_view>
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
  SourceLocation location;
  std::string message;
};

class ErrorReporter {
 public:
  void report(SourceLocation location, std::string message) {
    errors_.push_back({location, std::move(message)});
  }

  bool has_errors() const { return !errors_.empty(); }

  const std::vector<Error>& errors() const { return errors_; }

 private:
  std::vector<Error> errors_;
};

}  // namespace kai
