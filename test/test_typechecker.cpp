#include "catch.hpp"

#include "../src/error_reporter.h"
#include "../src/parser.h"
#include "../src/shape.h"
#include "../src/typechecker.h"

#include <algorithm>
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

TEST_CASE("type_checker_reports_not_callable_struct") {
  const auto errors = typecheck_source(R"(
let s = struct { x: 1 };
s(42);
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::NotCallable});
}

TEST_CASE("type_checker_not_callable_struct_carries_struct_ctx") {
  kai::ErrorReporter parse_reporter;
  kai::Parser parser("let s = struct { x: 1 };\ns(42);\n", parse_reporter);
  auto program = parser.parse_program();
  REQUIRE(!parse_reporter.has_errors());

  kai::ErrorReporter type_reporter;
  kai::TypeChecker checker(type_reporter);
  checker.visit_program(*program);

  REQUIRE(type_reporter.errors().size() == 1);
  auto* e = dynamic_cast<kai::NotCallableError*>(type_reporter.errors()[0].get());
  REQUIRE(e != nullptr);
  REQUIRE(e->kind == kai::Shape::Kind::Struct_Literal);
}

TEST_CASE("type_checker_reports_not_callable_non_struct") {
  const auto errors = typecheck_source(R"(
let x = 1;
x(2);
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::NotCallable});
}

TEST_CASE("type_checker_not_callable_non_struct_carries_non_struct_ctx") {
  kai::ErrorReporter parse_reporter;
  kai::Parser parser("let x = 1;\nx(2);\n", parse_reporter);
  auto program = parser.parse_program();
  REQUIRE(!parse_reporter.has_errors());

  kai::ErrorReporter type_reporter;
  kai::TypeChecker checker(type_reporter);
  checker.visit_program(*program);

  REQUIRE(type_reporter.errors().size() == 1);
  auto* e = dynamic_cast<kai::NotCallableError*>(type_reporter.errors()[0].get());
  REQUIRE(e != nullptr);
  REQUIRE(e->kind == kai::Shape::Kind::Non_Struct);
}

TEST_CASE("type_checker_reports_not_callable_array") {
  const auto errors = typecheck_source(R"(
let arr = [1, 2, 3];
arr(1);
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::NotCallable});
}

TEST_CASE("type_checker_not_callable_array_carries_array_ctx") {
  kai::ErrorReporter parse_reporter;
  kai::Parser parser("let arr = [1, 2, 3];\narr(1);\n", parse_reporter);
  auto program = parser.parse_program();
  REQUIRE(!parse_reporter.has_errors());

  kai::ErrorReporter type_reporter;
  kai::TypeChecker checker(type_reporter);
  checker.visit_program(*program);

  REQUIRE(type_reporter.errors().size() == 1);
  auto* e = dynamic_cast<kai::NotCallableError*>(type_reporter.errors()[0].get());
  REQUIRE(e != nullptr);
  REQUIRE(e->kind == kai::Shape::Kind::Array);
}

TEST_CASE("type_checker_reports_not_indexable_struct") {
  const auto errors = typecheck_source(R"(
let s = struct { x: 1 };
s[0];
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::NotIndexable});
}

TEST_CASE("type_checker_not_indexable_struct_carries_struct_ctx") {
  kai::ErrorReporter parse_reporter;
  kai::Parser parser("let s = struct { x: 1 };\ns[0];\n", parse_reporter);
  auto program = parser.parse_program();
  REQUIRE(!parse_reporter.has_errors());

  kai::ErrorReporter type_reporter;
  kai::TypeChecker checker(type_reporter);
  checker.visit_program(*program);

  REQUIRE(type_reporter.errors().size() == 1);
  auto* e = dynamic_cast<kai::NotIndexableError*>(type_reporter.errors()[0].get());
  REQUIRE(e != nullptr);
  REQUIRE(e->kind == kai::Shape::Kind::Struct_Literal);
}

TEST_CASE("type_checker_reports_not_indexable_non_struct") {
  const auto errors = typecheck_source("(1 + 2)[0];");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::NotIndexable});
}

TEST_CASE("type_checker_not_indexable_non_struct_carries_non_struct_ctx") {
  kai::ErrorReporter parse_reporter;
  kai::Parser parser("(1 + 2)[0];", parse_reporter);
  auto program = parser.parse_program();
  REQUIRE(!parse_reporter.has_errors());

  kai::ErrorReporter type_reporter;
  kai::TypeChecker checker(type_reporter);
  checker.visit_program(*program);

  REQUIRE(type_reporter.errors().size() == 1);
  auto* e = dynamic_cast<kai::NotIndexableError*>(type_reporter.errors()[0].get());
  REQUIRE(e != nullptr);
  REQUIRE(e->kind == kai::Shape::Kind::Non_Struct);
}

TEST_CASE("type_checker_not_indexable_function_carries_function_ctx") {
  kai::ErrorReporter parse_reporter;
  kai::Parser parser("fn f(x) { return x; }\nf[0];\n", parse_reporter);
  auto program = parser.parse_program();
  REQUIRE(!parse_reporter.has_errors());

  kai::ErrorReporter type_reporter;
  kai::TypeChecker checker(type_reporter);
  checker.visit_program(*program);

  REQUIRE(type_reporter.errors().size() == 1);
  auto* e = dynamic_cast<kai::NotIndexableError*>(type_reporter.errors()[0].get());
  REQUIRE(e != nullptr);
  REQUIRE(e->kind == kai::Shape::Kind::Function);
}

TEST_CASE("type_checker_reports_not_a_struct_on_array") {
  const auto errors = typecheck_source(R"(
let arr = [1, 2, 3];
arr.x;
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::NotAStruct});
}

TEST_CASE("type_checker_reports_not_a_struct_on_function") {
  const auto errors = typecheck_source(R"(
fn add(a, b) { return a + b; }
add.x;
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::NotAStruct});
}

TEST_CASE("type_checker_accepts_valid_name_and_field_usage") {
  const auto errors = typecheck_source(R"(
fn add(a, b) { return a + b; }
let point = struct { x: 40, y: 2 };
add(point.x, point.y);
)");
  REQUIRE(errors.empty());
}

TEST_CASE("type_checker_accepts_address_of_and_dereference_read_flow") {
  const auto errors = typecheck_source(R"(
let x = 1;
let p = &x;
let y = *p;
y = 2;
)");
  REQUIRE(errors.empty());
}

TEST_CASE("type_checker_allows_dereference_of_unknown_pointer_shape") {
  const auto errors = typecheck_source(R"(
fn read_ptr(p) {
  return *p;
}
read_ptr(1);
)");
  REQUIRE(errors.empty());
}

TEST_CASE("type_checker_reports_mismatch_assigning_pointer_to_non_pointer_variable") {
  const auto errors = typecheck_source(R"(
fn make(v) {
  let gp = 0;
  let x = v;
  gp = &x;
  return gp;
}
make(1);
)");
  REQUIRE(errors == std::vector<kai::Error::Type>{kai::Error::Type::TypeMismatch});
}

TEST_CASE(
    "type_checker_allows_returning_argument_pointer_when_other_argument_is_local_pointer") {
  const auto errors = typecheck_source(R"(
fn pick(a, b) {
  return a;
}

fn wrap(a) {
  let x = 1;
  return pick(a, &x);
}

let y = 1;
let py = &y;
wrap(py);
)");
  REQUIRE(errors.empty());
}

TEST_CASE("type_checker_rejects_function_capture_of_outer_variable") {
  const auto errors = typecheck_source(R"(
let x = 1;
fn f() {
  return x;
}
f();
)");
  REQUIRE(std::find(errors.begin(), errors.end(),
                    kai::Error::Type::UndefinedVariable) != errors.end());
}

TEST_CASE("type_checker_rejects_returning_reference_to_local_variable") {
  const auto errors = typecheck_source(R"(
fn make(v) {
  let x = v;
  return &x;
}
make(1);
)");
  REQUIRE_FALSE(errors.empty());
}

TEST_CASE(
    "type_checker_rejects_returning_local_reference_via_address_of_dereference_composition") {
  const auto errors = typecheck_source(R"(
fn id(p) {
  return &*p;
}

fn make(v) {
  let x = v;
  return id(&x);
}

make(1);
)");
  REQUIRE(std::find(errors.begin(), errors.end(),
                    kai::Error::Type::DanglingReference) != errors.end());
}
