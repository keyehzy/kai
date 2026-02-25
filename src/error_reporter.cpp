#include "error_reporter.h"

namespace kai {

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

  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string InvalidAssignmentTargetError::format_error() const {
  std::string msg =
      "invalid assignment target; expected variable or index expression before '='";
  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string ExpectedIdentifierError::format_error() const {
  std::string msg = "expected identifier";
  switch (ctx) {
    case Ctx::AfterDotInFieldAccess:
      msg += " after '.' in field access";
      break;
  }

  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
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

  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string InvalidNumericLiteralError::format_error() const {
  std::string msg = "invalid numeric literal";
  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }

  msg += " '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string ExpectedLetVariableNameError::format_error() const {
  std::string msg = "expected variable name after 'let'";
  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string ExpectedStructFieldNameError::format_error() const {
  std::string msg = "expected field name in struct literal";
  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string ExpectedStructFieldColonError::format_error() const {
  std::string msg = "expected ':'";
  if (field_name_location.has_value() && !field_name_location->text().empty()) {
    msg += " after field name '";
    msg += std::string(field_name_location->text());
    msg += "'";
  } else {
    msg += " after struct field name";
  }
  msg += " in struct literal";

  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string ExpectedStructLiteralBraceError::format_error() const {
  std::string msg = boundary == Boundary::OpeningBrace
                        ? "expected '{' to start struct literal"
                        : "expected '}' to close struct literal";
  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
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

  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string ExpectedPrimaryExpressionError::format_error() const {
  std::string msg = "expected primary expression";
  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string ExpectedSemicolonError::format_error() const {
  std::string msg = "expected ';' after statement";
  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string ExpectedEqualsError::format_error() const {
  std::string msg = "expected '='";
  switch (ctx) {
    case Ctx::AfterLetVariableName: {
      const std::string_view variable_name_text =
          context_location.has_value() && !context_location->text().empty()
              ? context_location->text()
              : std::string_view("name");
      msg += " after variable '";
      msg += std::string(variable_name_text);
      msg += "' in 'let' declaration";
      break;
    }
  }

  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string ExpectedOpeningParenthesisError::format_error() const {
  std::string msg = "expected '('";
  switch (ctx) {
    case Ctx::AfterWhile: {
      const std::string_view while_text =
          context_location.has_value() && !context_location->text().empty()
              ? context_location->text()
              : std::string_view("while");
      msg += " after '";
      msg += std::string(while_text);
      msg += "'";
      break;
    }
    case Ctx::AfterIf: {
      const std::string_view if_text =
          context_location.has_value() && !context_location->text().empty()
              ? context_location->text()
              : std::string_view("if");
      msg += " after '";
      msg += std::string(if_text);
      msg += "'";
      break;
    }
    case Ctx::AfterFunctionNameInDeclaration: {
      const std::string_view function_name_text =
          context_location.has_value() && !context_location->text().empty()
              ? context_location->text()
              : std::string_view("function name");
      msg += " after function name '";
      msg += std::string(function_name_text);
      msg += "' in declaration";
      break;
    }
  }

  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

std::string ExpectedClosingParenthesisError::format_error() const {
  std::string msg = "expected ')'";
  switch (ctx) {
    case Ctx::ToCloseWhileCondition: {
      const std::string_view while_text =
          context_location.has_value() && !context_location->text().empty()
              ? context_location->text()
              : std::string_view("while");
      msg += " to close '";
      msg += std::string(while_text);
      msg += "' condition";
      break;
    }
    case Ctx::ToCloseIfCondition: {
      const std::string_view if_text =
          context_location.has_value() && !context_location->text().empty()
              ? context_location->text()
              : std::string_view("if");
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

  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
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

  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
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
  const std::string_view found = location.text();
  if (found.empty()) {
    msg += ", found end of input";
    return msg;
  }
  msg += ", found '";
  msg += std::string(found);
  msg += "'";
  return msg;
}

}  // namespace kai
