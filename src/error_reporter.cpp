#include "error_reporter.h"

namespace kai {
namespace {

void append_found_suffix(std::string& msg, const SourceLocation& location,
                         std::string_view prefix = ", found ") {
  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return;
  }
  msg += prefix;
  msg += "'";
  msg += std::string(found);
  msg += "'";
}

std::string_view context_text(const std::optional<SourceLocation>& context_location,
                              std::string_view fallback) {
  if (context_location.has_value() && !context_location->text().empty()) {
    return context_location->text();
  }
  return fallback;
}

}  // namespace

std::string UnexpectedCharError::format_error() const {
  std::string msg = "unexpected character '";
  msg += ch;
  msg += "'";
  return msg;
}

std::string ExpectedEndOfExpressionError::format_error() const {
  return "expected end of expression";
}

std::string ExpectedVariableError::format_error() const {
  std::string msg = "expected variable";
  switch (ctx) {
    case Ctx::AsFunctionCallTarget:
      msg += " as function call target";
      break;
    case Ctx::BeforePostfixIncrement:
      msg += " before postfix '++'";
      break;
  }
  append_found_suffix(msg, location);
  return msg;
}

std::string InvalidAssignmentTargetError::format_error() const {
  std::string msg =
      "invalid assignment target; expected variable or index expression before '='";
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedIdentifierError::format_error() const {
  std::string msg = "expected identifier";
  switch (ctx) {
    case Ctx::AfterDotInFieldAccess:
      msg += " after '.' in field access";
      break;
  }
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedFunctionIdentifierError::format_error() const {
  std::string msg = "expected ";
  switch (ctx) {
    case Ctx::AfterFnKeyword:
      msg += "function name after 'fn'";
      break;
    case Ctx::InParameterList:
      msg += "parameter name in function declaration";
      break;
  }
  append_found_suffix(msg, location);
  return msg;
}

std::string InvalidNumericLiteralError::format_error() const {
  std::string msg = "invalid numeric literal";
  append_found_suffix(msg, location, " ");
  return msg;
}

std::string ExpectedLetVariableNameError::format_error() const {
  std::string msg = "expected variable name after 'let'";
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedStructFieldNameError::format_error() const {
  std::string msg = "expected field name in struct literal";
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedStructFieldColonError::format_error() const {
  std::string msg = "expected ':'";
  const std::string_view field_name_text = context_text(field_name_location, "");
  if (!field_name_text.empty()) {
    msg += " after field name '";
    msg += std::string(field_name_text);
    msg += "'";
  } else {
    msg += " after struct field name";
  }
  msg += " in struct literal";
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedStructLiteralBraceError::format_error() const {
  std::string msg = boundary == Boundary::OpeningBrace
                        ? "expected '{' to start struct literal"
                        : "expected '}' to close struct literal";
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedLiteralStartError::format_error() const {
  std::string msg = "expected ";
  switch (ctx) {
    case Ctx::ArrayLiteral:
      msg += "'[' to start array literal";
      break;
    case Ctx::StructLiteral:
      msg += "'struct' to start struct literal";
      break;
  }
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedPrimaryExpressionError::format_error() const {
  std::string msg = "expected primary expression";
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedSemicolonError::format_error() const {
  std::string msg = "expected ';' after statement";
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedEqualsError::format_error() const {
  std::string msg = "expected '='";
  switch (ctx) {
    case Ctx::AfterLetVariableName: {
      const std::string_view variable_name_text = context_text(context_location, "name");
      msg += " after variable '";
      msg += std::string(variable_name_text);
      msg += "' in 'let' declaration";
      break;
    }
  }
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedOpeningParenthesisError::format_error() const {
  std::string msg = "expected '('";
  switch (ctx) {
    case Ctx::AfterWhile: {
      const std::string_view while_text = context_text(context_location, "while");
      msg += " after '";
      msg += std::string(while_text);
      msg += "'";
      break;
    }
    case Ctx::AfterIf: {
      const std::string_view if_text = context_text(context_location, "if");
      msg += " after '";
      msg += std::string(if_text);
      msg += "'";
      break;
    }
    case Ctx::AfterFunctionNameInDeclaration: {
      const std::string_view function_name_text =
          context_text(context_location, "function name");
      msg += " after function name '";
      msg += std::string(function_name_text);
      msg += "' in declaration";
      break;
    }
  }
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedClosingParenthesisError::format_error() const {
  std::string msg = "expected ')'";
  switch (ctx) {
    case Ctx::ToCloseWhileCondition: {
      const std::string_view while_text = context_text(context_location, "while");
      msg += " to close '";
      msg += std::string(while_text);
      msg += "' condition";
      break;
    }
    case Ctx::ToCloseIfCondition: {
      const std::string_view if_text = context_text(context_location, "if");
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
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedClosingSquareBracketError::format_error() const {
  std::string msg = "expected ']'";
  switch (ctx) {
    case Ctx::ToCloseIndexExpression:
      msg += " to close index expression";
      break;
    case Ctx::ToCloseArrayLiteral:
      msg += " to close array literal";
      break;
  }
  append_found_suffix(msg, location);
  return msg;
}

std::string ExpectedBlockError::format_error() const {
  std::string msg = (boundary == Boundary::OpeningBrace) ? "expected '{' to start "
                                                          : "expected '}' to close ";
  if (block_token.has_value()) {
    msg += (*block_token).sv();
    msg += " ";
  }
  msg += "block";
  append_found_suffix(msg, location);
  return msg;
}

// ---------------------------------------------------------------------------
// Type error formatters
// ---------------------------------------------------------------------------

std::string TypeMismatchError::format_error() const {
  switch (ctx) {
    case Ctx::Assignment:
      return "type mismatch in assignment: cannot assign '" + got +
             "' to variable declared as '" + expected + "'";
  }
}

std::string UndefinedVariableError::format_error() const {
  return "undefined variable '" + name + "'";
}

std::string UndefinedFunctionError::format_error() const {
  return "undefined function '" + name + "'";
}

std::string WrongArgCountError::format_error() const {
  std::string msg = "function '";
  msg += name;
  msg += "' expects ";
  msg += std::to_string(expected);
  msg += " argument";
  if (expected != 1) msg += "s";
  msg += ", got ";
  msg += std::to_string(got);
  return msg;
}

std::string NotAStructError::format_error() const {
  std::string msg = "field access on non-struct value of type '";
  msg += actual_type;
  msg += "'";
  return msg;
}

std::string UndefinedFieldError::format_error() const {
  return "struct has no field '" + field + "'";
}

std::string NotCallableError::format_error() const {
  std::string msg = "cannot call value of type '";
  msg += describe(kind);
  msg += "': ";
  switch (kind) {
    case Shape::Kind::Struct_Literal: msg += "struct literals are not callable"; break;
    case Shape::Kind::Array:          msg += "arrays are not callable"; break;
    default:                          msg += "only declared functions are callable"; break;
  }
  return msg;
}

std::string NotIndexableError::format_error() const {
  std::string msg = "cannot index value of type '";
  msg += describe(kind);
  msg += "': ";
  switch (kind) {
    case Shape::Kind::Struct_Literal: msg += "struct literals are not arrays"; break;
    case Shape::Kind::Function:       msg += "functions are not arrays"; break;
    default:                          msg += "only arrays support indexing"; break;
  }
  return msg;
}

}  // namespace kai
