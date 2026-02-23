#include "ast.h"

namespace kai {
namespace ast {
void Ast::Block::dump(std::ostream &os) const {
  os << "Block(";
  for (const auto &child : children) {
    child->dump(os);
  }
  os << ")";
}

void Ast::FunctionDeclaration::dump(std::ostream &os) const {
  os << "FunctionDeclaration(" << name << ", [";
  for (size_t i = 0; i < parameters.size(); ++i) {
    if (i != 0) {
      os << ", ";
    }
    os << parameters[i];
  }
  os << "], ";
  body->dump(os);
  os << ")";
}

void Ast::FunctionCall::dump(std::ostream &os) const {
  os << "FunctionCall(" << name << ", [";
  for (size_t i = 0; i < arguments.size(); ++i) {
    if (i != 0) {
      os << ", ";
    }
    arguments[i]->dump(os);
  }
  os << "])";
}

void Ast::VariableDeclaration::dump(std::ostream &os) const {
  os << "VariableDeclaration(" << name << ", ";
  initializer->dump(os);
  os << ")";
}

void Ast::LessThan::dump(std::ostream &os) const {
  os << "LessThan(";
  left->dump(os);
  os << ", ";
  right->dump(os);
  os << ")";
}

void Ast::GreaterThan::dump(std::ostream &os) const {
  os << "GreaterThan(";
  left->dump(os);
  os << ", ";
  right->dump(os);
  os << ")";
}

void Ast::LessThanOrEqual::dump(std::ostream &os) const {
  os << "LessThanOrEqual(";
  left->dump(os);
  os << ", ";
  right->dump(os);
  os << ")";
}

void Ast::GreaterThanOrEqual::dump(std::ostream &os) const {
  os << "GreaterThanOrEqual(";
  left->dump(os);
  os << ", ";
  right->dump(os);
  os << ")";
}

void Ast::While::dump(std::ostream &os) const {
  os << "While(";
  condition->dump(os);
  os << ", ";
  body->dump(os);
  os << ")";
}

void Ast::Assignment::dump(std::ostream &os) const {
  os << "Assignment(" << name << ", ";
  value->dump(os);
  os << ")";
}

void Ast::Return::dump(std::ostream &os) const {
  os << "Return(";
  value->dump(os);
  os << ")";
}

void Ast::IfElse::dump(std::ostream &os) const {
  os << "IfElse(";
  condition->dump(os);
  os << ", ";
  body->dump(os);
  os << ", ";
  else_body->dump(os);
  os << ")";
}

void Ast::Equal::dump(std::ostream &os) const {
  os << "Equal(";
  left->dump(os);
  os << ", ";
  right->dump(os);
  os << ")";
}

void Ast::NotEqual::dump(std::ostream &os) const {
  os << "NotEqual(";
  left->dump(os);
  os << ", ";
  right->dump(os);
  os << ")";
}

void Ast::Add::dump(std::ostream &os) const {
  os << "Add(";
  left->dump(os);
  os << ", ";
  right->dump(os);
  os << ")";
}

void Ast::Subtract::dump(std::ostream &os) const {
  os << "Subtract(";
  left->dump(os);
  os << ", ";
  right->dump(os);
  os << ")";
}

void Ast::Multiply::dump(std::ostream &os) const {
  os << "Multiply(";
  left->dump(os);
  os << ", ";
  right->dump(os);
  os << ")";
}

void Ast::Divide::dump(std::ostream &os) const {
  os << "Divide(";
  left->dump(os);
  os << ", ";
  right->dump(os);
  os << ")";
}

void Ast::Modulo::dump(std::ostream &os) const {
  os << "Modulo(";
  left->dump(os);
  os << ", ";
  right->dump(os);
  os << ")";
}

void Ast::ArrayLiteral::dump(std::ostream &os) const {
  os << "ArrayLiteral(";
  for (size_t i = 0; i < elements.size(); ++i) {
    if (i != 0) {
      os << ", ";
    }
    elements[i]->dump(os);
  }
  os << ")";
}

void Ast::Index::dump(std::ostream &os) const {
  os << "Index(";
  array->dump(os);
  os << ", ";
  index->dump(os);
  os << ")";
}

void Ast::IndexAssignment::dump(std::ostream &os) const {
  os << "IndexAssignment(";
  array->dump(os);
  os << ", ";
  index->dump(os);
  os << ", ";
  value->dump(os);
  os << ")";
}

std::string indent_str(int indent) {
  return std::string(2 * indent, ' ');
}

void Ast::Block::to_string(std::ostream &os, int indent) const {
  os << " {\n";
  for (const auto &child : children) {
    child->to_string(os, indent + 1);
  }
  os << indent_str(indent) << "}\n";
}

void Ast::FunctionDeclaration::to_string(std::ostream &os, int indent) const {
  os << indent_str(indent) << "fn " << name << "(";
  for (size_t i = 0; i < parameters.size(); ++i) {
    if (i != 0) {
      os << ", ";
    }
    os << parameters[i];
  }
  os << ")";
  body->to_string(os, indent);
}

void Ast::FunctionCall::to_string(std::ostream &os, int) const {
  os << name << "(";
  for (size_t i = 0; i < arguments.size(); ++i) {
    if (i != 0) {
      os << ", ";
    }
    arguments[i]->to_string(os, 0);
  }
  os << ")";
}

void Ast::VariableDeclaration::to_string(std::ostream &os, int indent) const {
  os << indent_str(indent) << "let " << name << " = ";
  initializer->to_string(os, indent);
  os << "\n";
}

void Ast::LessThan::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " < ";
  right->to_string(os, indent);
}

void Ast::GreaterThan::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " > ";
  right->to_string(os, indent);
}

void Ast::LessThanOrEqual::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " <= ";
  right->to_string(os, indent);
}

void Ast::GreaterThanOrEqual::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " >= ";
  right->to_string(os, indent);
}

void Ast::Increment::to_string(std::ostream &os, int indent) const {
  os << indent_str(indent);
  variable->to_string(os, indent);
  os << "++";
  os << "\n";
}

void Ast::While::to_string(std::ostream &os, int indent) const {
  os << indent_str(indent) << "while (";
  condition->to_string(os, indent);
  os << ")";
  body->to_string(os, indent);
}

void Ast::Assignment::to_string(std::ostream &os, int indent) const {
  os << indent_str(indent) << name << " = ";
  value->to_string(os, indent);
  os << "\n";
}

void Ast::Return::to_string(std::ostream &os, int indent) const {
  os << indent_str(indent) << "return ";
  value->to_string(os, indent);
  os << "\n";
}

void Ast::IfElse::to_string(std::ostream &os, int indent) const {
  os << indent_str(indent) << "if (";
  condition->to_string(os, indent);
  os << ")";
  body->to_string(os, indent);
  os << " else ";
  else_body->to_string(os, indent);
}

void Ast::Equal::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " == ";
  right->to_string(os, indent);
}

void Ast::NotEqual::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " != ";
  right->to_string(os, indent);
}

void Ast::Add::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " + ";
  right->to_string(os, indent);
}

void Ast::Subtract::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " - ";
  right->to_string(os, indent);
}

void Ast::Multiply::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " * ";
  right->to_string(os, indent);
}

void Ast::Divide::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " / ";
  right->to_string(os, indent);
}

void Ast::Modulo::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " % ";
  right->to_string(os, indent);
}

void Ast::ArrayLiteral::to_string(std::ostream &os, int indent) const {
  (void)indent;
  os << "[";
  for (size_t i = 0; i < elements.size(); ++i) {
    if (i != 0) {
      os << ", ";
    }
    elements[i]->to_string(os, indent);
  }
  os << "]";
}

void Ast::Index::to_string(std::ostream &os, int indent) const {
  array->to_string(os, indent);
  os << "[";
  index->to_string(os, indent);
  os << "]";
}

void Ast::IndexAssignment::to_string(std::ostream &os, int indent) const {
  os << indent_str(indent);
  array->to_string(os, indent);
  os << "[";
  index->to_string(os, indent);
  os << "] = ";
  value->to_string(os, indent);
  os << "\n";
}
} // namespace ast
} // namespace kai
