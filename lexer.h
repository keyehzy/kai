#pragma once

#include <cctype>
#include <string_view>

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
  static bool is_identifier_start(char ch) {
    return ch == '_' || std::isalpha(static_cast<unsigned char>(ch)) != 0;
  }

  static bool is_digit(char ch) {
    return std::isdigit(static_cast<unsigned char>(ch)) != 0;
  }

  static bool is_identifier_continue(char ch) {
    return ch == '_' || std::isalnum(static_cast<unsigned char>(ch)) != 0;
  }

  static bool is_space(char ch) {
    return std::isspace(static_cast<unsigned char>(ch)) != 0;
  }

  bool is_eof() const {
    return input_ == original_input_.end();
  }

  char next_char() const {
    if (is_eof() || input_ + 1 == original_input_.end()) {
      return '\0';
    }
    return input_[1];
  }

  void skip_whitespaces() {
    while(!is_eof() && is_space(input_[0])) {
      ++input_;
    }
  }

  void parse_identifier() {
    last_token_.type = Token::Type::identifier;
    last_token_.begin = input_;
    ++input_;
    while(!is_eof() && is_identifier_continue(input_[0])) {
      ++input_;
    }
    last_token_.end = input_;
  }

  void parse_number() {
    last_token_.type = Token::Type::number;
    last_token_.begin = input_;
    ++input_;
    while(!is_eof() && is_digit(input_[0])) {
      ++input_;
    }
    last_token_.end = input_;
  }

  void parse_string() {
    last_token_.type = Token::Type::string;
    last_token_.begin = input_;
    ++input_;
    while(!is_eof() && input_[0] != '"') {
      ++input_;
    }
    if (!is_eof()) {
      ++input_;
    }
    last_token_.end = input_;
  }

  void parse_current_token() {
    skip_whitespaces();

    if (is_eof()) {
      last_token_.type = Token::Type::end_of_file;
      last_token_.begin = input_;
      last_token_.end = input_;
      return;
    }

    if (is_identifier_start(input_[0])) {
      parse_identifier();
      return;
    }
    if (is_digit(input_[0])) {
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
    case '[':
    case ']':
    case ',':
    case ':':
    case '.':
    case ';':
    case '-':
    case '*':
    case '/':
    case '%':
      last_token_.type = static_cast<Token::Type>(input_[0]);
      last_token_.begin = input_;
      last_token_.end = input_ + 1;
      ++input_;
      break;
    case '<':
      last_token_.begin = input_;
      if (next_char() == '=') {
        last_token_.type = Token::Type::less_than_equals;
        input_ += 2;
      } else {
        last_token_.type = Token::Type::less_than;
        ++input_;
      }
      last_token_.end = input_;
      break;
    case '>':
      last_token_.begin = input_;
      if (next_char() == '=') {
        last_token_.type = Token::Type::greater_than_equals;
        input_ += 2;
      } else {
        last_token_.type = Token::Type::greater_than;
        ++input_;
      }
      last_token_.end = input_;
      break;
    case '=':
      last_token_.begin = input_;
      if (next_char() == '=') {
        last_token_.type = Token::Type::equals_equals;
        input_ += 2;
      } else {
        last_token_.type = Token::Type::equals;
        ++input_;
      }
      last_token_.end = input_;
      break;
    case '!':
      last_token_.begin = input_;
      if (next_char() == '=') {
        last_token_.type = Token::Type::bang_equals;
        input_ += 2;
      } else {
        last_token_.type = Token::Type::unknown;
        ++input_;
      }
      last_token_.end = input_;
      break;
    case '+':
      last_token_.begin = input_;
      if (next_char() == '+') {
        last_token_.type = Token::Type::plus_plus;
        input_ += 2;
        last_token_.end = input_;
      } else {
        last_token_.type = Token::Type::plus;
        ++input_;
        last_token_.end = input_;
      }
      break;
    default:
      last_token_.type = Token::Type::unknown;
      last_token_.begin = input_;
      ++input_;
      last_token_.end = input_;
      break;
    }
  }


  Token last_token_;
  const char *input_;
  std::string_view original_input_;
};
