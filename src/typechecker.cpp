#include "typechecker.h"

#include <cassert>
#include <utility>

namespace kai {

TypeChecker::TypeChecker(ErrorReporter& reporter) : reporter_(reporter) {
  env_.push_scope();
}

void TypeChecker::visit_program(const ast::Ast::Block& program) {
  for (const auto& child : program.children) {
    visit_statement(child.get());
  }
}

SourceLocation TypeChecker::no_loc() {
  return {nullptr, nullptr};
}

void Env::push_scope() {
  var_scopes_.emplace_back();
}

void Env::pop_scope() {
  assert(!var_scopes_.empty());
  var_scopes_.pop_back();
}

void Env::bind_variable(const std::string& name, Shape* shape) {
  assert(!var_scopes_.empty());
  var_scopes_.back()[name] = shape;
}

Shape** Env::lookup_variable(const std::string& name) {
  for (auto it = var_scopes_.rbegin(); it != var_scopes_.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return &found->second;
    }
  }
  return nullptr;
}

void Env::declare_function(const std::string& name, size_t arity) {
  functions_[name] = arity;
}

size_t* Env::lookup_function(const std::string& name) {
  auto it = functions_.find(name);
  return it != functions_.end() ? &it->second : nullptr;
}

void TypeChecker::visit_block(const ast::Ast::Block& block) {
  env_.push_scope();
  for (const auto& child : block.children) {
    visit_statement(child.get());
  }
  env_.pop_scope();
}

void TypeChecker::visit_statement(const ast::Ast* node) {
  using T = ast::Ast::Type;

  switch (node->type) {
    case T::VariableDeclaration: {
      const auto& decl = ast_cast<const ast::Ast::VariableDeclaration&>(*node);
      env_.bind_variable(decl.name, visit_expression(decl.initializer.get()));
      break;
    }
    case T::Assignment:
    case T::IndexAssignment:
    case T::Return:
    case T::IfElse:
    case T::While:
    case T::Increment:
    case T::FunctionCall:
      static_cast<void>(visit_expression(node));
      break;
    case T::FunctionDeclaration: {
      const auto& fn = ast_cast<const ast::Ast::FunctionDeclaration&>(*node);
      env_.declare_function(fn.name, fn.parameters.size());

      env_.push_scope();
      for (const auto& param : fn.parameters) {
        env_.bind_variable(param, make_shape<Shape::Unknown>());
      }
      visit_block(*fn.body);
      env_.pop_scope();
      break;
    }
    case T::Block:
      visit_block(ast_cast<const ast::Ast::Block&>(*node));
      break;
    default:
      static_cast<void>(visit_expression(node));
      break;
  }
}

Shape* TypeChecker::visit_expression(const ast::Ast* node) {
  using T = ast::Ast::Type;

  switch (node->type) {
    case T::Literal:
      return make_shape<Shape::Non_Struct>();

    case T::Variable: {
      const auto& var = ast_cast<const ast::Ast::Variable&>(*node);
      Shape** value = env_.lookup_variable(var.name);
      if (!value) {
        reporter_.report<UndefinedVariableError>(no_loc(), var.name);
        return make_shape<Shape::Unknown>();
      }
      return *value;
    }

    case T::VariableDeclaration: {
      const auto& decl = ast_cast<const ast::Ast::VariableDeclaration&>(*node);
      Shape* value = visit_expression(decl.initializer.get());
      env_.bind_variable(decl.name, value);
      return value;
    }

    case T::Assignment: {
      const auto& assignment = ast_cast<const ast::Ast::Assignment&>(*node);
      Shape* value = visit_expression(assignment.value.get());
      Shape** target = env_.lookup_variable(assignment.name);
      if (!target) {
        reporter_.report<UndefinedVariableError>(no_loc(), assignment.name);
      } else {
        *target = value;
      }
      return value;
    }

    case T::FunctionCall: {
      const auto& call = ast_cast<const ast::Ast::FunctionCall&>(*node);
      for (const auto& arg : call.arguments) {
        static_cast<void>(visit_expression(arg.get()));
      }

      size_t* arity = env_.lookup_function(call.name);
      if (!arity) {
        reporter_.report<UndefinedFunctionError>(no_loc(), call.name);
        return make_shape<Shape::Unknown>();
      }

      if (call.arguments.size() != *arity) {
        reporter_.report<WrongArgCountError>(
            no_loc(), call.name, *arity, call.arguments.size());
      }
      return make_shape<Shape::Unknown>();
    }

    case T::Increment: {
      const auto& inc = ast_cast<const ast::Ast::Increment&>(*node);
      if (!env_.lookup_variable(inc.variable->name)) {
        reporter_.report<UndefinedVariableError>(no_loc(), inc.variable->name);
      }
      return make_shape<Shape::Non_Struct>();
    }

    case T::ArrayLiteral: {
      const auto& array = ast_cast<const ast::Ast::ArrayLiteral&>(*node);
      for (const auto& element : array.elements) {
        static_cast<void>(visit_expression(element.get()));
      }
      return make_shape<Shape::Non_Struct>();
    }

    case T::Index: {
      const auto& index = ast_cast<const ast::Ast::Index&>(*node);
      static_cast<void>(visit_expression(index.array.get()));
      static_cast<void>(visit_expression(index.index.get()));
      return make_shape<Shape::Unknown>();
    }

    case T::IndexAssignment: {
      const auto& assign = ast_cast<const ast::Ast::IndexAssignment&>(*node);
      static_cast<void>(visit_expression(assign.array.get()));
      static_cast<void>(visit_expression(assign.index.get()));
      return visit_expression(assign.value.get());
    }

    case T::StructLiteral: {
      const auto& literal = ast_cast<const ast::Ast::StructLiteral&>(*node);
      std::unordered_set<std::string> fields;
      fields.reserve(literal.fields.size());
      for (const auto& [name, value] : literal.fields) {
        fields.insert(name);
        static_cast<void>(visit_expression(value.get()));
      }
      return make_shape<Shape::Struct_Literal>(std::move(fields));
    }

    case T::FieldAccess: {
      const auto& access = ast_cast<const ast::Ast::FieldAccess&>(*node);
      Shape* object = visit_expression(access.object.get());
      if (object->kind != Shape::Kind::Struct_Literal) {
        reporter_.report<NotAStructError>(no_loc(), object->describe());
        return make_shape<Shape::Unknown>();
      }
      auto& struct_shape = derived_cast<Shape::Struct_Literal>(*object);
      if (!struct_shape.fields_.contains(access.field)) {
        reporter_.report<UndefinedFieldError>(no_loc(), access.field);
        return make_shape<Shape::Unknown>();
      }
      return make_shape<Shape::Unknown>();
    }

    case T::FunctionDeclaration: {
      visit_statement(node);
      return make_shape<Shape::Unknown>();
    }

    case T::Block: {
      visit_block(ast_cast<const ast::Ast::Block&>(*node));
      return make_shape<Shape::Unknown>();
    }

    case T::Return: {
      const auto& ret = ast_cast<const ast::Ast::Return&>(*node);
      return visit_expression(ret.value.get());
    }

    case T::IfElse: {
      const auto& if_else = ast_cast<const ast::Ast::IfElse&>(*node);
      static_cast<void>(visit_expression(if_else.condition.get()));
      visit_block(*if_else.body);
      visit_block(*if_else.else_body);
      return make_shape<Shape::Unknown>();
    }

    case T::While: {
      const auto& while_loop = ast_cast<const ast::Ast::While&>(*node);
      static_cast<void>(visit_expression(while_loop.condition.get()));
      visit_block(*while_loop.body);
      return make_shape<Shape::Unknown>();
    }

    case T::Add: {
      const auto& n = ast_cast<const ast::Ast::Add&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::Subtract: {
      const auto& n = ast_cast<const ast::Ast::Subtract&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::Multiply: {
      const auto& n = ast_cast<const ast::Ast::Multiply&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::Divide: {
      const auto& n = ast_cast<const ast::Ast::Divide&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::Modulo: {
      const auto& n = ast_cast<const ast::Ast::Modulo&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::LessThan: {
      const auto& n = ast_cast<const ast::Ast::LessThan&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::GreaterThan: {
      const auto& n = ast_cast<const ast::Ast::GreaterThan&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::LessThanOrEqual: {
      const auto& n = ast_cast<const ast::Ast::LessThanOrEqual&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::GreaterThanOrEqual: {
      const auto& n = ast_cast<const ast::Ast::GreaterThanOrEqual&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::Equal: {
      const auto& n = ast_cast<const ast::Ast::Equal&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::NotEqual: {
      const auto& n = ast_cast<const ast::Ast::NotEqual&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }

    case T::Negate: {
      const auto& n = ast_cast<const ast::Ast::Negate&>(*node);
      static_cast<void>(visit_expression(n.operand.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::UnaryPlus: {
      const auto& n = ast_cast<const ast::Ast::UnaryPlus&>(*node);
      static_cast<void>(visit_expression(n.operand.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::LogicalNot: {
      const auto& n = ast_cast<const ast::Ast::LogicalNot&>(*node);
      static_cast<void>(visit_expression(n.operand.get()));
      return make_shape<Shape::Non_Struct>();
    }
  }

  return make_shape<Shape::Unknown>();
}
}  // namespace kai
