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
  os << "FunctionDeclaration(" << name << ", ";
  body->dump(os);
  os << ")";
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

void Ast::Add::dump(std::ostream &os) const {
  os << "Add(";
  left->dump(os);
  os << ", ";
  right->dump(os);
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
  os << indent_str(indent) << "fn " << name << "()";
  body->to_string(os, indent);
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

void Ast::Add::to_string(std::ostream &os, int indent) const {
  left->to_string(os, indent);
  os << " + ";
  right->to_string(os, indent);
}
} // namespace ast
} // namespace kai
