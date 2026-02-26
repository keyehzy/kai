#include "catch.hpp"

#include "../src/error_reporter.h"
#include "../src/parser.h"
#include "../src/typechecker.h"

#include <string_view>
#include <vector>

namespace {

std::vector<kai::Error::Type> typecheck_source(std::string_view source) {
  kai::ErrorReporter parse_reporter;
  kai::Parser parser(source, parse_reporter);
  auto program = parser.parse_program();
  REQUIRE(!parse_reporter.has_errors());

  kai::ErrorReporter type_reporter;
  kai::TypeChecker checker(type_reporter);
  checker.visit_program(*program);

  std::vector<kai::Error::Type> types;
  types.reserve(type_reporter.errors().size());
  for (const auto& error : type_reporter.errors()) {
    types.push_back(error->type);
  }
  return types;
}

}  // namespace

TEST_CASE("type_checker_reports_undefined_variable") {
  const auto errors = typecheck_source("x + 1;");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::UndefinedVariable});
}

TEST_CASE("type_checker_reports_undefined_function") {
  const auto errors = typecheck_source("missing(1, 2);");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::UndefinedFunction});
}

TEST_CASE("type_checker_reports_wrong_arg_count") {
  const auto errors = typecheck_source(R"(
fn add(a, b) { return a + b; }
add(1);
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::WrongArgCount});
}

TEST_CASE("type_checker_reports_not_a_struct") {
  const auto errors = typecheck_source(R"(
let value = 1;
value.x;
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::NotAStruct});
}

TEST_CASE("type_checker_reports_undefined_field") {
  const auto errors = typecheck_source(R"(
let point = struct { x: 1 };
point.y;
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::UndefinedField});
}

TEST_CASE("type_checker_reports_type_mismatch_assigning_non_struct_to_struct") {
  const auto errors = typecheck_source(R"(
let point = struct { x: 1 };
point = 5;
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::TypeMismatch});
}

TEST_CASE("type_checker_reports_type_mismatch_assigning_struct_to_non_struct") {
  const auto errors = typecheck_source(R"(
let value = 5;
value = struct { x: 1 };
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::TypeMismatch});
}

TEST_CASE("type_checker_reports_type_mismatch_assigning_struct_with_different_fields") {
  const auto errors = typecheck_source(R"(
let point = struct { x: 1 };
point = struct { y: 1 };
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::TypeMismatch});
}

TEST_CASE("type_checker_assignment_mismatch_carries_assignment_ctx") {
  kai::ErrorReporter parse_reporter;
  kai::Parser parser("let point = struct { x: 1 };\npoint = 5;\n", parse_reporter);
  auto program = parser.parse_program();
  REQUIRE(!parse_reporter.has_errors());

  kai::ErrorReporter type_reporter;
  kai::TypeChecker checker(type_reporter);
  checker.visit_program(*program);

  REQUIRE(type_reporter.errors().size() == 1);
  auto* e = dynamic_cast<kai::TypeMismatchError*>(type_reporter.errors()[0].get());
  REQUIRE(e != nullptr);
  REQUIRE(e->ctx == kai::TypeMismatchError::Ctx::Assignment);
  REQUIRE(e->expected == "Struct_Literal");
  REQUIRE(e->got == "Non_Struct");
}

TEST_CASE("type_checker_accepts_assignment_of_same_struct_shape") {
  const auto errors = typecheck_source(R"(
let point = struct { x: 1 };
point = struct { x: 2 };
)");
  REQUIRE(errors.empty());
}

TEST_CASE("type_checker_accepts_assignment_of_non_struct_to_non_struct") {
  const auto errors = typecheck_source(R"(
let value = 1;
value = 2;
)");
  REQUIRE(errors.empty());
}

TEST_CASE("type_checker_accepts_valid_name_and_field_usage") {
  const auto errors = typecheck_source(R"(
fn add(a, b) { return a + b; }
let point = struct { x: 40, y: 2 };
add(point.x, point.y);
)");
  REQUIRE(errors.empty());
}
