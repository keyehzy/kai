#pragma once

#include <cstddef>
#include <string_view>

namespace kai {

// A half-open byte range [begin, end) into the original source string.
struct SourceLocation {
  const char* begin;
  const char* end;

  std::string_view text() const {
    return {begin, static_cast<size_t>(end - begin)};
  }
};

}  // namespace kai
