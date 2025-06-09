#pragma once

#include <cassert>
#include <string_view>

struct Token {
  enum class Type {
    end_of_file,
    identifier,
    number,
    string,

    lparen = '(',
    rparen = ')',
    lcurly = '{',
    rcurly = '}',
    semicolon = ';',

    equals = '=',
    plus = '+',
  };

  Type type;
  const char* begin;
  const char* end;

  std::string_view sv() const { return std::string_view(begin, end); }

  Token() = default;
};

class Lexer {
public:
  Lexer(std::string_view input) : input_(input.data()), original_input_(input) {
    last_token_.end = input_;
    parse_current_token();
  }

  const Token& peek() const { return last_token_; }

  void skip() { parse_current_token(); }

private:
  void skip_whitespaces() {
    while(isspace(*input_++)) { }
  }

  void parse_identifier() {
    last_token_.begin = input_;
    ++input_;
    while(isalnum(*input_++)) { }
    last_token_.end = input_;
  }

  void parse_number() {
    last_token_.begin = input_;
    ++input_;
    while(isdigit(*input_++)) { }
    last_token_.end = input_;
  }

  void parse_string() {
    last_token_.begin = input_;
    ++input_;
    while(*input_++ != '"' && input_ != original_input_.end()) { }
    if (input_ == original_input_.end()) {
      // fixme: handle error
    }
    last_token_.end = input_;
  }

  void parse_current_token() {
    skip_whitespaces();

    if (isalpha(input_[0])) {
      parse_identifier();
      return;
    }
    if (isdigit(input_[0])) {
      parse_number();
      return;
    }

    switch(input_[0]) {
    case '"':
      parse_string();
      break;
    case '(':
    case ')':
    case '{':
    case '}':
    case ';':
    case '=':
    case '+':
      last_token_.type = static_cast<Token::Type>(input_[0]);
      last_token_.begin = input_;
      last_token_.end = input_ + 1;
      ++input_;
      break;
    case '\0':
      if (input_ == original_input_.end()) {
        last_token_.type = Token::Type::end_of_file;
        last_token_.end = input_;
        break;
      } else {
        [[fallthrough]];
      }
    default:
      assert(0);
      break;
    }
  }


  Token last_token_;
  const char *input_;
  std::string_view original_input_;
};
