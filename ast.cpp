#include "ast.h"

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