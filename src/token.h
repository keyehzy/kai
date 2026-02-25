#pragma once

#include <string>
#include <string_view>

#include "source_location.h"

namespace kai {
struct Token {
  enum class Type {
    end_of_file,
    identifier,
    number,
    string,
    unknown,
    plus_plus,
    equals_equals,
    bang_equals,
    bang,
    less_than_equals,
    greater_than_equals,

    lparen = '(',
    rparen = ')',
    lcurly = '{',
    rcurly = '}',
    lsquare = '[',
    rsquare = ']',
    comma = ',',
    colon = ':',
    dot = '.',
    semicolon = ';',

    equals = '=',
    plus = '+',
    minus = '-',
    star = '*',
    slash = '/',
    percent = '%',
    less_than = '<',
    greater_than = '>',
  };

  Type type;
  const char* begin;
  const char* end;

  std::string_view sv() const { return std::string_view(begin, end - begin); }
  SourceLocation source_location() const { return SourceLocation{begin, end}; }

  Token() = default;
};
} // namespace kai
