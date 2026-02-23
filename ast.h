#pragma once

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace kai {
namespace ast {

struct Ast {
  enum class Type {
    FunctionDeclaration,
    FunctionCall,
    Block,
    While,
    VariableDeclaration,
    LessThan,
    GreaterThan,
    Increment,
    Literal,
    Variable,
    Assignment,
    Return,
    IfElse,
    Equal,
    Add,
    Subtract,
    Multiply,
    Divide,
    Modulo,
    ArrayLiteral,
    Index,
    IndexAssignment,
  };

  struct FunctionDeclaration;
  struct FunctionCall;
  struct Block;
  struct While;
  struct VariableDeclaration;
  struct LessThan;
  struct GreaterThan;
  struct Increment;
  struct Literal;
  struct Variable;
  struct Assignment;
  struct Return;
  struct IfElse;
  struct Equal;
  struct Add;
  struct Subtract;
  struct Multiply;
  struct Divide;
  struct Modulo;
  struct ArrayLiteral;
  struct Index;
  struct IndexAssignment;

  Type type;

  Ast() = default;

  virtual ~Ast() = default;

  virtual void dump(std::ostream &os) const = 0;
  virtual void to_string(std::ostream &os, int indent = 0) const = 0;

 protected:
  explicit Ast(Type type) : type(type) {}
};

template <typename Derived_Reference, typename Base>
Derived_Reference derived_cast(Base &base) {
  using Derived = std::remove_reference_t<Derived_Reference>;
  static_assert(std::is_base_of_v<Base, Derived>);
  static_assert(!std::is_base_of_v<Derived, Base>);
  return static_cast<Derived_Reference>(base);
}

template <typename Derived>
Derived ast_cast(Ast &ast) {
  return derived_cast<Derived>(ast);
}

template <typename Derived>
Derived ast_cast(const Ast &ast) {
  return derived_cast<Derived>(ast);
}

struct Ast::Block final : public Ast {
  std::vector<std::unique_ptr<Ast>> children;

  Block() : Ast(Type::Block) {}

  void append(std::unique_ptr<Ast> node) { children.emplace_back(std::move(node)); }

  template <typename T, typename... Args>
  void append(Args &&...args) {
    children.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
  }

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

typedef uint64_t Value;

struct Ast::FunctionDeclaration final : public Ast {
  std::string name;
  std::unique_ptr<Block> body;

  FunctionDeclaration(std::string name, std::unique_ptr<Block> body)
      : Ast(Type::FunctionDeclaration),
        name(name),
        body(std::move(body)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::FunctionCall final : public Ast {
  std::string name;

  explicit FunctionCall(std::string name)
      : Ast(Type::FunctionCall), name(std::move(name)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::VariableDeclaration final : public Ast {
  std::string name;
  std::unique_ptr<Ast> initializer;

  VariableDeclaration(std::string name, std::unique_ptr<Ast> initializer)
      : Ast(Type::VariableDeclaration),
        name(name),
        initializer(std::move(initializer)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::LessThan final : public Ast {
  std::unique_ptr<Ast> left;
  std::unique_ptr<Ast> right;

  LessThan(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right)
      : Ast(Type::LessThan), left(std::move(left)), right(std::move(right)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::GreaterThan final : public Ast {
  std::unique_ptr<Ast> left;
  std::unique_ptr<Ast> right;

  GreaterThan(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right)
      : Ast(Type::GreaterThan), left(std::move(left)), right(std::move(right)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::Literal final : public Ast {
  Value value;

  Literal(Value value) : Ast(Type::Literal), value(value) {}

  void dump(std::ostream &os) const override { os << "Literal(" << value << ")"; }
  void to_string(std::ostream &os, int) const override { os << value; }
};

struct Ast::Variable final : public Ast {
  std::string name;

  Variable(std::string name) : Ast(Type::Variable), name(name) {}

  void dump(std::ostream &os) const override { os << "Variable(" << name << ")"; }
  void to_string(std::ostream &os, int) const override { os << name; }
};

struct Ast::Increment final : public Ast {
  std::unique_ptr<Variable> variable;

  Increment(std::unique_ptr<Variable> variable)
      : Ast(Type::Increment), variable(std::move(variable)) {}

  void dump(std::ostream &os) const override { os << "Increment(" << variable->name << ")"; }
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::While final : public Ast {
  std::unique_ptr<LessThan> condition;
  std::unique_ptr<Block> body;

  While(std::unique_ptr<LessThan> condition, std::unique_ptr<Block> body)
      : Ast(Type::While), condition(std::move(condition)), body(std::move(body)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::Assignment final : public Ast {
  std::string name;
  std::unique_ptr<Ast> value;

  Assignment(std::string name, std::unique_ptr<Ast> value)
      : Ast(Type::Assignment), name(name), value(std::move(value)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::Return final : public Ast {
  std::unique_ptr<Ast> value;

  Return(std::unique_ptr<Ast> value) : Ast(Type::Return), value(std::move(value)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::IfElse final : public Ast {
  std::unique_ptr<LessThan> condition;
  std::unique_ptr<Block> body;
  std::unique_ptr<Block> else_body;

  IfElse(std::unique_ptr<LessThan> condition, std::unique_ptr<Block> body,
         std::unique_ptr<Block> else_body)
      : Ast(Type::IfElse),
        condition(std::move(condition)),
        body(std::move(body)),
        else_body(std::move(else_body)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::Equal final : public Ast {
  std::unique_ptr<Ast> left;
  std::unique_ptr<Ast> right;

  Equal(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right)
      : Ast(Type::Equal), left(std::move(left)), right(std::move(right)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::Add final : public Ast {
  std::unique_ptr<Ast> left;
  std::unique_ptr<Ast> right;

  Add(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right)
      : Ast(Type::Add), left(std::move(left)), right(std::move(right)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::Subtract final : public Ast {
  std::unique_ptr<Ast> left;
  std::unique_ptr<Ast> right;

  Subtract(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right)
      : Ast(Type::Subtract), left(std::move(left)), right(std::move(right)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::Multiply final : public Ast {
  std::unique_ptr<Ast> left;
  std::unique_ptr<Ast> right;

  Multiply(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right)
      : Ast(Type::Multiply), left(std::move(left)), right(std::move(right)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::Divide final : public Ast {
  std::unique_ptr<Ast> left;
  std::unique_ptr<Ast> right;

  Divide(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right)
      : Ast(Type::Divide), left(std::move(left)), right(std::move(right)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::Modulo final : public Ast {
  std::unique_ptr<Ast> left;
  std::unique_ptr<Ast> right;

  Modulo(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right)
      : Ast(Type::Modulo), left(std::move(left)), right(std::move(right)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::ArrayLiteral final : public Ast {
  std::vector<std::unique_ptr<Ast>> elements;

  explicit ArrayLiteral(std::vector<std::unique_ptr<Ast>> elements)
      : Ast(Type::ArrayLiteral), elements(std::move(elements)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::Index final : public Ast {
  std::unique_ptr<Ast> array;
  std::unique_ptr<Ast> index;

  Index(std::unique_ptr<Ast> array, std::unique_ptr<Ast> index)
      : Ast(Type::Index), array(std::move(array)), index(std::move(index)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct Ast::IndexAssignment final : public Ast {
  std::unique_ptr<Ast> array;
  std::unique_ptr<Ast> index;
  std::unique_ptr<Ast> value;

  IndexAssignment(std::unique_ptr<Ast> array, std::unique_ptr<Ast> index,
                  std::unique_ptr<Ast> value)
      : Ast(Type::IndexAssignment),
        array(std::move(array)),
        index(std::move(index)),
        value(std::move(value)) {}

  void dump(std::ostream &os) const override;
  void to_string(std::ostream &os, int indent = 0) const override;
};

struct AstInterpreter {
  AstInterpreter() {
    push_scope();
  }

  ~AstInterpreter() {
    exit_scope();
  }

  Value interpret_variable(const Ast::Variable &variable) {
    auto it = find_variable_scope(variable.name);
    assert(it != scopes.rend());
    return it->at(variable.name);
  }

  Value interpret_literal(const Ast::Literal &literal) { return literal.value; }

  Value interpret_less_than(const Ast::LessThan &less_than) {
    return interpret(*less_than.left) < interpret(*less_than.right);
  }

  Value interpret_greater_than(const Ast::GreaterThan &greater_than) {
    return interpret(*greater_than.left) > interpret(*greater_than.right);
  }

  Value interpret_variable_declaration(const Ast::VariableDeclaration &variable_declaration) {
    assert(find_variable_scope(variable_declaration.name) == scopes.rend());
    return current_scope()[variable_declaration.name] = interpret(*variable_declaration.initializer);
  }

  Value interpret_increment(const Ast::Increment &increment) {
    auto it = find_variable_scope(increment.variable->name);
    assert(it != scopes.rend());
    return it->at(increment.variable->name)++;
  }

  Value interpret_block(const Ast::Block &block) {
    push_scope();
    Value result = 0;
    for (const auto &child : block.children) {
      result = interpret(*child);
    }
    exit_scope();
    return result;
  }

  Value interpret_while(const Ast::While &while_loop) {
    Value result = 0;
    while (interpret(*while_loop.condition)) {
      result = interpret_block(*while_loop.body);
    }
    return result;
  }

  Value interpret_function_declaration(const Ast::FunctionDeclaration &function_declaration) {
    functions[function_declaration.name] = &function_declaration;
    return 0;
  }

  Value interpret_function_call(const Ast::FunctionCall &function_call) {
    const auto it = functions.find(function_call.name);
    assert(it != functions.end());
    return interpret_block(*it->second->body);
  }

  Value interpret_assignment(const Ast::Assignment &assignment) {
    auto it = find_variable_scope(assignment.name);
    assert(it != scopes.rend());
    return it->at(assignment.name) = interpret(*assignment.value);
  }

  Value interpret_return(const Ast::Return &return_statement) {
    return interpret(*return_statement.value);
  }

  Value interpret_if_else(const Ast::IfElse &if_else) {
    if (interpret(*if_else.condition)) {
      return interpret_block(*if_else.body);
    } else {
      return interpret_block(*if_else.else_body);
    }
  }

  Value interpret_equal(const Ast::Equal &equal) {
    return interpret(*equal.left) == interpret(*equal.right);
  }

  Value interpret_add(const Ast::Add &add) { return interpret(*add.left) + interpret(*add.right); }

  Value interpret_subtract(const Ast::Subtract &subtract) {
    return interpret(*subtract.left) - interpret(*subtract.right);
  }

  Value interpret_multiply(const Ast::Multiply &multiply) {
    return interpret(*multiply.left) * interpret(*multiply.right);
  }

  Value interpret_divide(const Ast::Divide &divide) {
    return interpret(*divide.left) / interpret(*divide.right);
  }

  Value interpret_modulo(const Ast::Modulo &modulo) {
    return interpret(*modulo.left) % interpret(*modulo.right);
  }

  Value interpret_array_literal(const Ast::ArrayLiteral &array_literal) {
    const auto handle = next_array_handle++;
    auto &array = arrays[handle];
    array.reserve(array_literal.elements.size());
    for (const auto &element : array_literal.elements) {
      array.push_back(interpret(*element));
    }
    return handle;
  }

  Value interpret_index(const Ast::Index &index) {
    const auto handle = interpret(*index.array);
    const auto idx = interpret(*index.index);
    const auto it = arrays.find(handle);
    assert(it != arrays.end());
    assert(idx < it->second.size());
    return it->second[idx];
  }

  Value interpret_index_assignment(const Ast::IndexAssignment &index_assignment) {
    const auto handle = interpret(*index_assignment.array);
    const auto idx = interpret(*index_assignment.index);
    const auto value = interpret(*index_assignment.value);
    const auto it = arrays.find(handle);
    assert(it != arrays.end());
    assert(idx < it->second.size());
    it->second[idx] = value;
    return value;
  }

  Value interpret(const Ast &ast) {
    switch (ast.type) {
      case Ast::Type::Variable:
        return interpret_variable(ast_cast<Ast::Variable const &>(ast));
      case Ast::Type::Literal:
        return interpret_literal(ast_cast<Ast::Literal const &>(ast));
      case Ast::Type::LessThan:
        return interpret_less_than(ast_cast<Ast::LessThan const &>(ast));
      case Ast::Type::GreaterThan:
        return interpret_greater_than(ast_cast<Ast::GreaterThan const &>(ast));
      case Ast::Type::VariableDeclaration:
        return interpret_variable_declaration(ast_cast<Ast::VariableDeclaration const &>(ast));
      case Ast::Type::Increment:
        return interpret_increment(ast_cast<Ast::Increment const &>(ast));
      case Ast::Type::While:
        return interpret_while(ast_cast<Ast::While const &>(ast));
      case Ast::Type::Block:
        return interpret_block(ast_cast<Ast::Block const &>(ast));
      case Ast::Type::FunctionDeclaration:
        return interpret_function_declaration(ast_cast<Ast::FunctionDeclaration const &>(ast));
      case Ast::Type::FunctionCall:
        return interpret_function_call(ast_cast<Ast::FunctionCall const &>(ast));
      case Ast::Type::Assignment:
        return interpret_assignment(ast_cast<Ast::Assignment const &>(ast));
      case Ast::Type::Return:
        return interpret_return(ast_cast<Ast::Return const &>(ast));
      case Ast::Type::IfElse:
        return interpret_if_else(ast_cast<Ast::IfElse const &>(ast));
      case Ast::Type::Equal:
        return interpret_equal(ast_cast<Ast::Equal const &>(ast));
      case Ast::Type::Add:
        return interpret_add(ast_cast<Ast::Add const &>(ast));
      case Ast::Type::Subtract:
        return interpret_subtract(ast_cast<Ast::Subtract const &>(ast));
      case Ast::Type::Multiply:
        return interpret_multiply(ast_cast<Ast::Multiply const &>(ast));
      case Ast::Type::Divide:
        return interpret_divide(ast_cast<Ast::Divide const &>(ast));
      case Ast::Type::Modulo:
        return interpret_modulo(ast_cast<Ast::Modulo const &>(ast));
      case Ast::Type::ArrayLiteral:
        return interpret_array_literal(ast_cast<Ast::ArrayLiteral const &>(ast));
      case Ast::Type::Index:
        return interpret_index(ast_cast<Ast::Index const &>(ast));
      case Ast::Type::IndexAssignment:
        return interpret_index_assignment(ast_cast<Ast::IndexAssignment const &>(ast));
    }
    assert(false);
  }

  void push_scope() {
    scopes.emplace_back();
  }

  void exit_scope() {
    scopes.pop_back();
  }

  std::unordered_map<std::string, Value> &current_scope() {
    return scopes.back();
  }

  std::vector<std::unordered_map<std::string, Value>>::reverse_iterator find_variable_scope(const std::string &name) {
    auto it = scopes.rbegin();
    for (; it != scopes.rend(); ++it) {
      if (it->find(name) != it->end()) {
        return it;
      }
    }
    return it;
  }

  std::vector<std::unordered_map<std::string, Value>> scopes;
  std::unordered_map<std::string, const Ast::FunctionDeclaration *> functions;
  std::unordered_map<Value, std::vector<Value>> arrays;
  Value next_array_handle = 1;
};
} // namespace ast
} // namespace kai
