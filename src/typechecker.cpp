#include "typechecker.h"

#include <algorithm>
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
  if (a->kind == Shape::Kind::Pointer) {
    return shapes_compatible(derived_cast<const Shape::Pointer&>(*a).pointee_,
                             derived_cast<const Shape::Pointer&>(*b).pointee_);
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
  if (!function_scope_starts_.empty()) {
    assert(function_scope_starts_.back() != var_scopes_.size() - 1 &&
           "cannot pop the base function scope before exit_function_scope");
  }
  var_scopes_.pop_back();
}

void Env::enter_function_scope() {
  assert(!var_scopes_.empty());
  function_scope_starts_.push_back(var_scopes_.size() - 1);
}

void Env::exit_function_scope() {
  assert(!function_scope_starts_.empty());
  function_scope_starts_.pop_back();
}

bool Env::inside_function() const {
  return !function_scope_starts_.empty();
}

size_t Env::variable_lookup_floor() const {
  return function_scope_starts_.empty() ? 0 : function_scope_starts_.back();
}

void Env::bind_variable(const std::string& name, Shape* shape,
                        bool may_reference_local,
                        bool may_reference_argument,
                        std::unordered_set<size_t> referenced_argument_indices) {
  assert(!var_scopes_.empty());
  var_scopes_.back()[name] = {
      .shape = shape,
      .may_reference_local = may_reference_local,
      .may_reference_argument = may_reference_argument,
      .referenced_argument_indices = std::move(referenced_argument_indices),
  };
}

std::optional<Env::VariableBinding> Env::lookup_variable(const std::string& name) const {
  if (var_scopes_.empty()) {
    return std::nullopt;
  }

  const size_t floor = variable_lookup_floor();
  for (size_t i = var_scopes_.size(); i-- > floor;) {
    auto found = var_scopes_[i].find(name);
    if (found != var_scopes_[i].end()) {
      return found->second;
    }
  }
  return std::nullopt;
}

Env::VariableBinding* Env::lookup_variable_mut(const std::string& name) {
  if (var_scopes_.empty()) {
    return nullptr;
  }

  const size_t floor = variable_lookup_floor();
  for (size_t i = var_scopes_.size(); i-- > floor;) {
    auto found = var_scopes_[i].find(name);
    if (found != var_scopes_[i].end()) {
      return &found->second;
    }
  }
  return nullptr;
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
      const auto value = visit_expression(decl.initializer.get());
      env_.bind_variable(
          decl.name, value.shape, value.may_reference_local,
          value.may_reference_argument, value.referenced_argument_indices);
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

      auto& summary = function_summaries_[fn.name];
      summary.arity = fn.parameters.size();
      summary.returns_local_reference = false;
      summary.returned_argument_indices.clear();

      env_.push_scope();
      env_.enter_function_scope();
      function_stack_.push_back(fn.name);
      for (size_t i = 0; i < fn.parameters.size(); ++i) {
        env_.bind_variable(fn.parameters[i], make_shape<Shape::Unknown>(), false,
                           true, {i});
      }
      visit_block(*fn.body);
      function_stack_.pop_back();
      env_.exit_function_scope();
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

TypeChecker::ExprInfo TypeChecker::visit_expression(const Ast* node) {
  using T = Ast::Type;

  const auto unknown = [&]() -> ExprInfo {
    return {
        .shape = make_shape<Shape::Unknown>(),
        .may_reference_local = false,
        .may_reference_argument = false,
        .referenced_argument_indices = {},
    };
  };

  switch (node->type) {
    case T::Literal:
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
          .referenced_argument_indices = {},
      };

    case T::Variable: {
      const auto& var = derived_cast<const Ast::Variable&>(*node);
      auto value = env_.lookup_variable(var.name);
      if (!value) {
        reporter_.report<UndefinedVariableError>(no_loc(), var.name);
        return unknown();
      }
      return {
          .shape = value->shape,
          .may_reference_local = value->may_reference_local,
          .may_reference_argument = value->may_reference_argument,
          .referenced_argument_indices = value->referenced_argument_indices,
      };
    }

    case T::VariableDeclaration: {
      const auto& decl = derived_cast<const Ast::VariableDeclaration&>(*node);
      const auto value = visit_expression(decl.initializer.get());
      env_.bind_variable(
          decl.name, value.shape, value.may_reference_local,
          value.may_reference_argument, value.referenced_argument_indices);
      return value;
    }

    case T::Assignment: {
      const auto& assignment = derived_cast<const Ast::Assignment&>(*node);
      const auto value = visit_expression(assignment.value.get());
      auto* target = env_.lookup_variable_mut(assignment.name);
      if (target == nullptr) {
        reporter_.report<UndefinedVariableError>(no_loc(), assignment.name);
      } else if (!shapes_compatible(target->shape, value.shape)) {
        reporter_.report<TypeMismatchError>(
            no_loc(), TypeMismatchError::Ctx::Assignment,
            describe(target->shape->kind), describe(value.shape->kind));
      } else {
        target->may_reference_local = value.may_reference_local;
        target->may_reference_argument = value.may_reference_argument;
        target->referenced_argument_indices = value.referenced_argument_indices;
      }
      // TODO(pointer): add dereference-assignment (`*p = v`) typing once the
      // parser/AST support dereference assignment targets.
      return value;
    }

    case T::FunctionCall: {
      const auto& call = derived_cast<const Ast::FunctionCall&>(*node);
      std::vector<ExprInfo> args;
      args.reserve(call.arguments.size());
      for (const auto& arg : call.arguments) {
        args.push_back(visit_expression(arg.get()));
      }

      auto arity = env_.lookup_function(call.name);
      if (!arity) {
        auto var = env_.lookup_variable(call.name);
        if (var && var->shape->kind != Shape::Kind::Unknown &&
            var->shape->kind != Shape::Kind::Function) {
          reporter_.report<NotCallableError>(no_loc(), var->shape->kind);
        } else {
          reporter_.report<UndefinedFunctionError>(no_loc(), call.name);
        }
        return unknown();
      }

      if (call.arguments.size() != *arity) {
        reporter_.report<WrongArgCountError>(
            no_loc(), call.name, *arity, call.arguments.size());
      }

      bool returns_local_reference = false;
      std::unordered_set<size_t> returned_argument_indices;
      auto summary_it = function_summaries_.find(call.name);
      if (summary_it != function_summaries_.end()) {
        returns_local_reference = summary_it->second.returns_local_reference;
        returned_argument_indices = summary_it->second.returned_argument_indices;
      }

      bool references_local_from_argument = false;
      bool references_argument_from_argument = false;
      std::unordered_set<size_t> referenced_argument_indices;
      for (size_t returned_index : returned_argument_indices) {
        if (returned_index >= args.size()) {
          continue;
        }
        const auto& arg = args[returned_index];
        references_local_from_argument =
            references_local_from_argument || arg.may_reference_local;
        references_argument_from_argument =
            references_argument_from_argument || arg.may_reference_argument;
        referenced_argument_indices.insert(arg.referenced_argument_indices.begin(),
                                           arg.referenced_argument_indices.end());
      }

      return {
          .shape = make_shape<Shape::Unknown>(),
          .may_reference_local =
              returns_local_reference || references_local_from_argument,
          .may_reference_argument = references_argument_from_argument,
          .referenced_argument_indices = std::move(referenced_argument_indices),
      };
    }

    case T::Increment: {
      const auto& inc = derived_cast<const Ast::Increment&>(*node);
      if (!env_.lookup_variable(inc.variable->name)) {
        reporter_.report<UndefinedVariableError>(no_loc(), inc.variable->name);
      }
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }

    case T::ArrayLiteral: {
      const auto& array = derived_cast<const Ast::ArrayLiteral&>(*node);
      for (const auto& element : array.elements) {
        static_cast<void>(visit_expression(element.get()));
      }
      return {
          .shape = make_shape<Shape::Array>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }

    case T::Index: {
      const auto& index = derived_cast<const Ast::Index&>(*node);
      const auto array = visit_expression(index.array.get());
      static_cast<void>(visit_expression(index.index.get()));
      if (array.shape->kind != Shape::Kind::Unknown &&
          array.shape->kind != Shape::Kind::Array) {
        reporter_.report<NotIndexableError>(no_loc(), array.shape->kind);
      }
      return unknown();
    }

    case T::IndexAssignment: {
      const auto& assign = derived_cast<const Ast::IndexAssignment&>(*node);
      const auto array = visit_expression(assign.array.get());
      static_cast<void>(visit_expression(assign.index.get()));
      if (array.shape->kind != Shape::Kind::Unknown &&
          array.shape->kind != Shape::Kind::Array) {
        reporter_.report<NotIndexableError>(no_loc(), array.shape->kind);
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
      return {
          .shape = make_shape<Shape::Struct_Literal>(std::move(fields)),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }

    case T::FieldAccess: {
      const auto& access = derived_cast<const Ast::FieldAccess&>(*node);
      const auto object = visit_expression(access.object.get());
      if (object.shape->kind != Shape::Kind::Struct_Literal) {
        reporter_.report<NotAStructError>(no_loc(), describe(object.shape->kind));
        return unknown();
      }
      auto& struct_shape = derived_cast<Shape::Struct_Literal&>(*object.shape);
      if (!struct_shape.fields_.contains(access.field)) {
        reporter_.report<UndefinedFieldError>(no_loc(), access.field);
        return unknown();
      }
      return unknown();
    }

    case T::FunctionDeclaration: {
      visit_statement(node);
      return unknown();
    }

    case T::Block: {
      visit_block(derived_cast<const Ast::Block&>(*node));
      return unknown();
    }

    case T::Return: {
      const auto& ret = derived_cast<const Ast::Return&>(*node);
      const auto value = visit_expression(ret.value.get());
      if (!function_stack_.empty()) {
        auto& summary = function_summaries_[function_stack_.back()];
        if (value.may_reference_local && !summary.returns_local_reference) {
          reporter_.report<DanglingReferenceError>(no_loc());
        }
        summary.returns_local_reference =
            summary.returns_local_reference || value.may_reference_local;
        for (size_t argument_index : value.referenced_argument_indices) {
          if (argument_index < summary.arity) {
            summary.returned_argument_indices.insert(argument_index);
          }
        }
      }
      return value;
    }

    case T::IfElse: {
      const auto& if_else = derived_cast<const Ast::IfElse&>(*node);
      static_cast<void>(visit_expression(if_else.condition.get()));
      visit_block(*if_else.body);
      visit_block(*if_else.else_body);
      return unknown();
    }

    case T::While: {
      const auto& while_loop = derived_cast<const Ast::While&>(*node);
      static_cast<void>(visit_expression(while_loop.condition.get()));
      visit_block(*while_loop.body);
      return unknown();
    }

    case T::Add: {
      const auto& n = derived_cast<const Ast::Add&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::Subtract: {
      const auto& n = derived_cast<const Ast::Subtract&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::Multiply: {
      const auto& n = derived_cast<const Ast::Multiply&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::Divide: {
      const auto& n = derived_cast<const Ast::Divide&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::Modulo: {
      const auto& n = derived_cast<const Ast::Modulo&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::LessThan: {
      const auto& n = derived_cast<const Ast::LessThan&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::GreaterThan: {
      const auto& n = derived_cast<const Ast::GreaterThan&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::LessThanOrEqual: {
      const auto& n = derived_cast<const Ast::LessThanOrEqual&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::GreaterThanOrEqual: {
      const auto& n = derived_cast<const Ast::GreaterThanOrEqual&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::Equal: {
      const auto& n = derived_cast<const Ast::Equal&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::NotEqual: {
      const auto& n = derived_cast<const Ast::NotEqual&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::LogicalAnd: {
      const auto& n = derived_cast<const Ast::LogicalAnd&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::LogicalOr: {
      const auto& n = derived_cast<const Ast::LogicalOr&>(*node);
      static_cast<void>(visit_expression(n.left.get()));
      static_cast<void>(visit_expression(n.right.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }

    case T::Negate: {
      const auto& n = derived_cast<const Ast::Negate&>(*node);
      static_cast<void>(visit_expression(n.operand.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::UnaryPlus: {
      const auto& n = derived_cast<const Ast::UnaryPlus&>(*node);
      static_cast<void>(visit_expression(n.operand.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::LogicalNot: {
      const auto& n = derived_cast<const Ast::LogicalNot&>(*node);
      static_cast<void>(visit_expression(n.operand.get()));
      return {
          .shape = make_shape<Shape::Non_Struct>(),
          .may_reference_local = false,
          .may_reference_argument = false,
      };
    }
    case T::AddressOf: {
      const auto& n = derived_cast<const Ast::AddressOf&>(*node);
      const auto operand = visit_expression(n.operand.get());
      const bool is_direct_variable = n.operand->type == T::Variable;
      bool local_reference = false;
      if (env_.inside_function() && is_direct_variable) {
        const auto& var = derived_cast<const Ast::Variable&>(*n.operand);
        local_reference = env_.lookup_variable(var.name).has_value();
      }
      std::unordered_set<size_t> referenced_argument_indices;
      if (!is_direct_variable) {
        referenced_argument_indices = operand.referenced_argument_indices;
      }
      return {
          .shape = make_shape<Shape::Pointer>(operand.shape),
          .may_reference_local =
              local_reference || (!is_direct_variable && operand.may_reference_local),
          .may_reference_argument =
              !is_direct_variable && operand.may_reference_argument,
          .referenced_argument_indices = std::move(referenced_argument_indices),
      };
    }
    case T::Dereference: {
      const auto& n = derived_cast<const Ast::Dereference&>(*node);
      const auto operand = visit_expression(n.operand.get());
      if (operand.shape->kind == Shape::Kind::Pointer) {
        return {
            .shape = derived_cast<const Shape::Pointer&>(*operand.shape).pointee_,
            .may_reference_local = operand.may_reference_local,
            .may_reference_argument = operand.may_reference_argument,
            .referenced_argument_indices = operand.referenced_argument_indices,
        };
      }
      if (operand.shape->kind == Shape::Kind::Unknown) {
        return {
            .shape = make_shape<Shape::Unknown>(),
            .may_reference_local = operand.may_reference_local,
            .may_reference_argument = operand.may_reference_argument,
            .referenced_argument_indices = operand.referenced_argument_indices,
        };
      }
      return unknown();
    }
  }

  return unknown();
}
}  // namespace kai
