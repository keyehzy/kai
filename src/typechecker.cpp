#include "typechecker.h"

#include <cassert>
#include <utility>

namespace kai {

namespace {

bool shapes_compatible(const Shape* a, const Shape* b) {
  if (a->kind == Shape::Kind::Unknown || b->kind == Shape::Kind::Unknown) return true;
  if (a->kind != b->kind) return false;
  if (a->kind == Shape::Kind::Struct_Literal) {
    return derived_cast<const Shape::Struct_Literal&>(*a).fields_ ==
           derived_cast<const Shape::Struct_Literal&>(*b).fields_;
  }
  return true;
}

}  // namespace

TypeChecker::TypeChecker(ErrorReporter& reporter) : reporter_(reporter) {
  env_.push_scope();
}

void TypeChecker::visit_program(const Ast::Block& program) {
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

std::optional<Shape*> Env::lookup_variable(const std::string& name) {
  for (auto it = var_scopes_.rbegin(); it != var_scopes_.rend(); ++it) {
    auto found = it->find(name);
    if (found != it->end()) {
      return found->second;
    }
  }
  return std::nullopt;
}

void Env::declare_function(const std::string& name, size_t arity) {
  functions_[name] = arity;
}

std::optional<size_t> Env::lookup_function(const std::string& name) {
  auto it = functions_.find(name);
  return it != functions_.end() ? std::make_optional(it->second) : std::nullopt;
}

void TypeChecker::visit_block(const Ast::Block& block) {
  env_.push_scope();
  for (const auto& child : block.children) {
    visit_statement(child.get());
  }
  env_.pop_scope();
}

void TypeChecker::visit_statement(const Ast* node) {
  using T = Ast::Type;

  switch (node->type) {
    case T::VariableDeclaration: {
      const auto& decl = derived_cast<const Ast::VariableDeclaration&>(*node);
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
      const auto& fn = derived_cast<const Ast::FunctionDeclaration&>(*node);
      env_.declare_function(fn.name, fn.parameters.size());
      env_.bind_variable(fn.name, make_shape<Shape::Function>());

      env_.push_scope();
      for (const auto& param : fn.parameters) {
        env_.bind_variable(param, make_shape<Shape::Unknown>());
      }
      visit_block(*fn.body);
      env_.pop_scope();
      break;
    }
    case T::Block:
      visit_block(derived_cast<const Ast::Block&>(*node));
      break;
    default:
      static_cast<void>(visit_expression(node));
      break;
  }
}

Shape* TypeChecker::visit_expression(const Ast* node) {
  using T = Ast::Type;

  switch (node->type) {
    case T::Literal:
      return make_shape<Shape::Non_Struct>();

    case T::Variable: {
      const auto& var = derived_cast<const Ast::Variable&>(*node);
      auto value = env_.lookup_variable(var.name);
      if (!value) {
        reporter_.report<UndefinedVariableError>(no_loc(), var.name);
        return make_shape<Shape::Unknown>();
      }
      return *value;
    }

    case T::VariableDeclaration: {
      const auto& decl = derived_cast<const Ast::VariableDeclaration&>(*node);
      auto value = visit_expression(decl.initializer.get());
      env_.bind_variable(decl.name, value);
      return value;
    }

    case T::Assignment: {
      const auto& assignment = derived_cast<const Ast::Assignment&>(*node);
      auto value = visit_expression(assignment.value.get());
      auto target = env_.lookup_variable(assignment.name);
      if (!target) {
        reporter_.report<UndefinedVariableError>(no_loc(), assignment.name);
      } else if (!shapes_compatible(*target, value)) {
        reporter_.report<TypeMismatchError>(
            no_loc(), TypeMismatchError::Ctx::Assignment, describe((*target)->kind), describe(value->kind));
      }
      return value;
    }

    case T::FunctionCall: {
      const auto& call = derived_cast<const Ast::FunctionCall&>(*node);
      for (const auto& arg : call.arguments) {
        static_cast<void>(visit_expression(arg.get()));
      }

      auto arity = env_.lookup_function(call.name);
      if (!arity) {
        auto var = env_.lookup_variable(call.name);
        if (var && (*var)->kind != Shape::Kind::Unknown &&
            (*var)->kind != Shape::Kind::Function) {
          reporter_.report<NotCallableError>(no_loc(), (*var)->kind);
        } else {
          reporter_.report<UndefinedFunctionError>(no_loc(), call.name);
        }
        return make_shape<Shape::Unknown>();
      }

      if (call.arguments.size() != *arity) {
        reporter_.report<WrongArgCountError>(
            no_loc(), call.name, *arity, call.arguments.size());
      }
      return make_shape<Shape::Unknown>();
    }

    case T::Increment: {
      const auto& inc = derived_cast<const Ast::Increment&>(*node);
      if (!env_.lookup_variable(inc.variable->name)) {
        reporter_.report<UndefinedVariableError>(no_loc(), inc.variable->name);
      }
      return make_shape<Shape::Non_Struct>();
    }

    case T::ArrayLiteral: {
      const auto& array = derived_cast<const Ast::ArrayLiteral&>(*node);
      for (const auto& element : array.elements) {
        static_cast<void>(visit_expression(element.get()));
      }
      return make_shape<Shape::Array>();
    }

    case T::Index: {
      const auto& index = derived_cast<const Ast::Index&>(*node);
      auto array_shape = visit_expression(index.array.get());
      static_cast<void>(visit_expression(index.index.get()));
      if (array_shape->kind != Shape::Kind::Unknown &&
          array_shape->kind != Shape::Kind::Array) {
        reporter_.report<NotIndexableError>(no_loc(), array_shape->kind);
      }
      return make_shape<Shape::Unknown>();
    }

    case T::IndexAssignment: {
      const auto& assign = derived_cast<const Ast::IndexAssignment&>(*node);
      auto array_shape = visit_expression(assign.array.get());
      static_cast<void>(visit_expression(assign.index.get()));
      if (array_shape->kind != Shape::Kind::Unknown &&
          array_shape->kind != Shape::Kind::Array) {
        reporter_.report<NotIndexableError>(no_loc(), array_shape->kind);
      }
      return visit_expression(assign.value.get());
    }

    case T::StructLiteral: {
      const auto& literal = derived_cast<const Ast::StructLiteral&>(*node);
      std::unordered_set<std::string> fields;
      fields.reserve(literal.fields.size());
      for (const auto& [name, value] : literal.fields) {
        fields.insert(name);
        static_cast<void>(visit_expression(value.get()));
      }
      return make_shape<Shape::Struct_Literal>(std::move(fields));
    }

    case T::FieldAccess: {
      const auto& access = derived_cast<const Ast::FieldAccess&>(*node);
      auto object = visit_expression(access.object.get());
      if (object->kind != Shape::Kind::Struct_Literal) {
        reporter_.report<NotAStructError>(no_loc(), describe(object->kind));
        return make_shape<Shape::Unknown>();
      }
      auto& struct_shape = derived_cast<Shape::Struct_Literal&>(*object);
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
      visit_block(derived_cast<const Ast::Block&>(*node));
      return make_shape<Shape::Unknown>();
    }

    case T::Return: {
      const auto& ret = derived_cast<const Ast::Return&>(*node);
      return visit_expression(ret.value.get());
    }

    case T::IfElse: {
      const auto& if_else = derived_cast<const Ast::IfElse&>(*node);
      static_cast<void>(visit_expression(if_else.condition.get()));
      visit_block(*if_else.body);
      visit_block(*if_else.else_body);
      return make_shape<Shape::Unknown>();
    }

    case T::While: {
      const auto& while_loop = derived_cast<const Ast::While&>(*node);
      static_cast<void>(visit_expression(while_loop.condition.get()));
      visit_block(*while_loop.body);
      return make_shape<Shape::Unknown>();
    }

    case T::Add: {
      const auto& n = derived_cast<const Ast::Add&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::Subtract: {
      const auto& n = derived_cast<const Ast::Subtract&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::Multiply: {
      const auto& n = derived_cast<const Ast::Multiply&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::Divide: {
      const auto& n = derived_cast<const Ast::Divide&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::Modulo: {
      const auto& n = derived_cast<const Ast::Modulo&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::LessThan: {
      const auto& n = derived_cast<const Ast::LessThan&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::GreaterThan: {
      const auto& n = derived_cast<const Ast::GreaterThan&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::LessThanOrEqual: {
      const auto& n = derived_cast<const Ast::LessThanOrEqual&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::GreaterThanOrEqual: {
      const auto& n = derived_cast<const Ast::GreaterThanOrEqual&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::Equal: {
      const auto& n = derived_cast<const Ast::Equal&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::NotEqual: {
      const auto& n = derived_cast<const Ast::NotEqual&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::LogicalAnd: {
      const auto& n = derived_cast<const Ast::LogicalAnd&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::LogicalOr: {
      const auto& n = derived_cast<const Ast::LogicalOr&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return make_shape<Shape::Non_Struct>();
    }

    case T::Negate: {
      const auto& n = derived_cast<const Ast::Negate&>(*node);
      static_cast<void>(visit_expression(n.operand.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::UnaryPlus: {
      const auto& n = derived_cast<const Ast::UnaryPlus&>(*node);
      static_cast<void>(visit_expression(n.operand.get()));
      return make_shape<Shape::Non_Struct>();
    }
    case T::LogicalNot: {
      const auto& n = derived_cast<const Ast::LogicalNot&>(*node);
      static_cast<void>(visit_expression(n.operand.get()));
      return make_shape<Shape::Non_Struct>();
    }
  }

  return make_shape<Shape::Unknown>();
}
}  // namespace kai
