#include "../catch.hpp"
#include "../error_reporter.h"
#include "../lexer.h"

using kai::Lexer;
using kai::Token;

// --- line_column ---

TEST_CASE("test_line_column_at_start_of_source") {
  std::string_view src = "abc";
  auto lc = kai::line_column(src, src.data());
  REQUIRE(lc.line == 1);
  REQUIRE(lc.column == 1);
}

TEST_CASE("test_line_column_mid_first_line") {
  std::string_view src = "abc";
  auto lc = kai::line_column(src, src.data() + 2);
  REQUIRE(lc.line == 1);
  REQUIRE(lc.column == 3);
}

TEST_CASE("test_line_column_at_start_of_second_line") {
  std::string_view src = "ab\ncd";
  auto lc = kai::line_column(src, src.data() + 3);
  REQUIRE(lc.line == 2);
  REQUIRE(lc.column == 1);
}

TEST_CASE("test_line_column_mid_second_line") {
  std::string_view src = "ab\ncd";
  auto lc = kai::line_column(src, src.data() + 4);
  REQUIRE(lc.line == 2);
  REQUIRE(lc.column == 2);
}

TEST_CASE("test_line_column_tracks_multiple_newlines") {
  std::string_view src = "a\nb\nc";
  // 'c' is at offset 4, line 3, column 1
  auto lc = kai::line_column(src, src.data() + 4);
  REQUIRE(lc.line == 3);
  REQUIRE(lc.column == 1);
}

// --- SourceLocation ---

TEST_CASE("test_source_location_text_full_span") {
  std::string_view src = "abc";
  kai::SourceLocation loc{src.data(), src.data() + 3};
  REQUIRE(loc.text() == "abc");
}

TEST_CASE("test_source_location_text_partial_span") {
  std::string_view src = "let x = 1";
  kai::SourceLocation loc{src.data() + 4, src.data() + 5};
  REQUIRE(loc.text() == "x");
}

TEST_CASE("test_source_location_text_zero_width") {
  std::string_view src = "abc";
  kai::SourceLocation loc{src.data() + 1, src.data() + 1};
  REQUIRE(loc.text() == "");
}

// --- ErrorReporter ---

TEST_CASE("test_error_reporter_starts_empty") {
  kai::ErrorReporter reporter;
  REQUIRE(!reporter.has_errors());
  REQUIRE(reporter.errors().empty());
}

TEST_CASE("test_error_reporter_records_single_error") {
  std::string_view src = "abc";
  kai::ErrorReporter reporter;
  reporter.report({src.data(), src.data() + 3}, "something went wrong");
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0].message == "something went wrong");
  REQUIRE(reporter.errors()[0].location.text() == "abc");
}

TEST_CASE("test_error_reporter_records_multiple_errors_in_order") {
  std::string_view src = "abc def";
  kai::ErrorReporter reporter;
  reporter.report({src.data(), src.data() + 3}, "first error");
  reporter.report({src.data() + 4, src.data() + 7}, "second error");
  REQUIRE(reporter.errors().size() == 2);
  REQUIRE(reporter.errors()[0].message == "first error");
  REQUIRE(reporter.errors()[0].location.text() == "abc");
  REQUIRE(reporter.errors()[1].message == "second error");
  REQUIRE(reporter.errors()[1].location.text() == "def");
}

// --- Lexer + ErrorReporter integration ---

TEST_CASE("test_lexer_no_errors_for_valid_source") {
  std::string src = "let x = 42;";
  kai::ErrorReporter reporter;
  Lexer lexer(src, reporter);
  while (lexer.peek().type != Token::Type::end_of_file) {
    lexer.skip();
  }
  REQUIRE(!reporter.has_errors());
}

TEST_CASE("test_lexer_reports_unknown_character") {
  std::string src = "@";
  kai::ErrorReporter reporter;
  Lexer lexer(src, reporter);
  REQUIRE(lexer.peek().type == Token::Type::unknown);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
}

TEST_CASE("test_lexer_reports_unknown_character_message") {
  std::string src = "@";
  kai::ErrorReporter reporter;
  Lexer lexer(src, reporter);
  REQUIRE(reporter.errors()[0].message == "unexpected character '@'");
}

TEST_CASE("test_lexer_reports_unknown_character_as_single_char_span") {
  std::string src = "@";
  kai::ErrorReporter reporter;
  Lexer lexer(src, reporter);
  REQUIRE(reporter.errors()[0].location.text() == "@");
}

TEST_CASE("test_lexer_reports_unknown_character_line_column") {
  std::string src = "@";
  kai::ErrorReporter reporter;
  Lexer lexer(src, reporter);
  auto lc = kai::line_column(src, reporter.errors()[0].location.begin);
  REQUIRE(lc.line == 1);
  REQUIRE(lc.column == 1);
}

TEST_CASE("test_lexer_reports_unknown_character_mid_source") {
  std::string src = "abc @";
  kai::ErrorReporter reporter;
  Lexer lexer(src, reporter);
  // After construction: first token is identifier "abc", no error yet.
  REQUIRE(!reporter.has_errors());
  lexer.skip();  // advance past "abc"; next token will be '@'
  REQUIRE(lexer.peek().type == Token::Type::unknown);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors()[0].location.text() == "@");
  auto lc = kai::line_column(src, reporter.errors()[0].location.begin);
  REQUIRE(lc.line == 1);
  REQUIRE(lc.column == 5);
}

TEST_CASE("test_lexer_reports_standalone_bang") {
  std::string src = "!";
  kai::ErrorReporter reporter;
  Lexer lexer(src, reporter);
  REQUIRE(lexer.peek().type == Token::Type::unknown);
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors()[0].message == "unexpected character '!'");
  REQUIRE(reporter.errors()[0].location.text() == "!");
}

TEST_CASE("test_lexer_bang_equals_is_not_an_error") {
  std::string src = "!=";
  kai::ErrorReporter reporter;
  Lexer lexer(src, reporter);
  REQUIRE(lexer.peek().type == Token::Type::bang_equals);
  REQUIRE(!reporter.has_errors());
}

TEST_CASE("test_lexer_reports_unknown_character_on_second_line") {
  std::string src = "let x = 1;\n@";
  kai::ErrorReporter reporter;
  Lexer lexer(src, reporter);
  while (lexer.peek().type != Token::Type::end_of_file) {
    lexer.skip();
  }
  REQUIRE(reporter.has_errors());
  REQUIRE(reporter.errors().size() == 1);
  REQUIRE(reporter.errors()[0].location.text() == "@");
  auto lc = kai::line_column(src, reporter.errors()[0].location.begin);
  REQUIRE(lc.line == 2);
  REQUIRE(lc.column == 1);
}

TEST_CASE("test_lexer_reports_multiple_unknown_characters") {
  std::string src = "@ $";
  kai::ErrorReporter reporter;
  Lexer lexer(src, reporter);
  // '@' reported on construction
  REQUIRE(reporter.errors().size() == 1);
  lexer.skip();
  // '$' reported on skip
  REQUIRE(reporter.errors().size() == 2);
  REQUIRE(reporter.errors()[0].location.text() == "@");
  REQUIRE(reporter.errors()[1].location.text() == "$");
}
