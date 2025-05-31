#include <string>
#include <vector>
#include <memory>
#include <iostream>

class Ast {
public:
  Ast() = default;
  virtual ~Ast() = default;

  enum class Type {
    Literal,
    Identifier,
    VariableDeclaration,
    Assignment,
    For,
    LessThan,
    Increment,
    Return,
    Block,
    FunctionDeclaration,
  };

  class Literal;
  class Identifier;
  class VariableDeclaration;
  class Assignment;
  class For;
  class LessThan;
  class Increment;
  class Return;
  class Block;
  class FunctionDeclaration;
    
  Type type() const { return type_; }

  static const std::string indent_str(int indent) { return std::string(4 * indent, ' '); }
  virtual std::string to_string(int indent = 0) const = 0;

protected:
  Ast(Type type) : type_(type) {}

private:
  Type type_;
};

enum class ValueType {
  Int,
  Bool,
  String,
};

std::string value_type_to_string(ValueType type) {
  switch (type) {
    case ValueType::Int:
      return "int";
    default:
      throw std::runtime_error("Invalid value type");
  }
}


class Literal final : public Ast {
public:
  Literal(int value) : Ast(Type::Literal), value_(value) {}

  std::string to_string(int) const override {
    return std::to_string(value_);
  }

private:
  int value_;
};

class Identifier final : public Ast {
public:
  Identifier(std::string name) : Ast(Type::Identifier), name_(std::move(name)) {}

  std::string to_string(int) const override {
    return name_;
  }

private:
  std::string name_;
};

class VariableDeclaration final : public Ast {
public:
  VariableDeclaration(ValueType type, std::string name, std::unique_ptr<Ast> value) : Ast(Type::VariableDeclaration), type_(type), name_(std::move(name)), value_(std::move(value)) {}

  std::string to_string(int indent) const override {
    return indent_str(indent) + value_type_to_string(type_) + " " + name_ + " = " + value_->to_string(0);
  }

private:
  ValueType type_;
  std::string name_;
  std::unique_ptr<Ast> value_;
};

class Assignment final : public Ast {
public:
  Assignment(std::unique_ptr<Ast> lhs, std::unique_ptr<Ast> rhs) : Ast(Type::Assignment), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  std::string to_string(int indent) const override {
    return indent_str(indent) + lhs_->to_string(0) + " = " + rhs_->to_string(0);
  }

private:
  std::unique_ptr<Ast> lhs_;
  std::unique_ptr<Ast> rhs_;
};

class For final : public Ast {
public:
  For(std::unique_ptr<Ast> init, std::unique_ptr<Ast> condition, std::unique_ptr<Ast> update, std::unique_ptr<Ast> body) : Ast(Type::For), init_(std::move(init)), condition_(std::move(condition)), update_(std::move(update)), body_(std::move(body)) {}

  std::string to_string(int indent) const override {
    std::string result = indent_str(indent);
    result += "for (" + init_->to_string(0) + "; " + condition_->to_string(0) + "; " + update_->to_string(0) + ") {\n";
    result += body_->to_string(indent + 1);
    result += "\n" + indent_str(indent) + "}";
    return result;
  }

private:
  std::unique_ptr<Ast> init_;
  std::unique_ptr<Ast> condition_;
  std::unique_ptr<Ast> update_;
  std::unique_ptr<Ast> body_;
};

class LessThan final : public Ast {
public:
  LessThan(std::unique_ptr<Ast> lhs, std::unique_ptr<Ast> rhs) : Ast(Type::LessThan), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

  std::string to_string(int indent) const override {
    return indent_str(indent) + lhs_->to_string(0) + " < " + rhs_->to_string(0);
  }

private:
  std::unique_ptr<Ast> lhs_;
  std::unique_ptr<Ast> rhs_;
};

class Increment final : public Ast {
public:
  Increment(std::unique_ptr<Ast> lhs) : Ast(Type::Increment), lhs_(std::move(lhs)) {}

  std::string to_string(int indent) const override {
    return indent_str(indent) + lhs_->to_string(0) + "++";
  }

private:
  std::unique_ptr<Ast> lhs_;
};

class Return final : public Ast {
public:
  Return(std::unique_ptr<Ast> value) : Ast(Type::Return), value_(std::move(value)) {}

  std::string to_string(int indent) const override {
    return indent_str(indent) + "return " + value_->to_string(0);
  }

private:
  std::unique_ptr<Ast> value_;
};

class Block final : public Ast {
public:
  Block() : Ast(Type::Block) {}

  Block(std::vector<std::unique_ptr<Ast>> statements) : Ast(Type::Block), statements_(std::move(statements)) {}

  void add_statement(std::unique_ptr<Ast> statement) {
    statements_.push_back(std::move(statement));
  }

  std::string to_string(int indent) const override {
    std::string result;
    for (size_t i = 0; i < statements_.size(); ++i) {
      if (i > 0) result += "\n";
      result += statements_[i]->to_string(indent);
    }
    return result;
  }

private:
  std::vector<std::unique_ptr<Ast>> statements_;
};

class FunctionDeclaration final : public Ast {
public:
  FunctionDeclaration(ValueType return_type, std::string name, std::vector<std::unique_ptr<Ast>> args, std::unique_ptr<Ast> body)
      : Ast(Type::FunctionDeclaration), return_type_(return_type), name_(std::move(name)), args_(std::move(args)), body_(std::move(body)) {}

  std::string to_string(int indent) const override {
    std::string result = indent_str(indent) + "fn " + value_type_to_string(return_type_) + " " + name_ + "(";
    for (size_t i = 0; i < args_.size(); ++i) {
      if (i > 0)  result += ", ";
      result += args_[i]->to_string(0);
    }
    result += ") {\n";
    result += body_->to_string(indent + 1);
    result += "\n" + indent_str(indent) + "}";
    return result;
  }

private:
  ValueType return_type_;
  std::string name_;
  std::vector<std::unique_ptr<Ast>> args_;
  std::unique_ptr<Ast> body_;
};

/* 
  Example of code (iterative fibonacci)
  fn int fib(int n) {
    int result = 0;
    int a = 0;
    int b = 1;
    for (int i = 0; i < n; i++) {
      result = a + b;
      a = b;
      b = result;
    }
    return result;
  }
*/

int main() { 
  auto block = std::make_unique<Block>();
  block->add_statement(std::make_unique<VariableDeclaration>(ValueType::Int, "result", std::make_unique<Literal>(0)));
  block->add_statement(std::make_unique<VariableDeclaration>(ValueType::Int, "a", std::make_unique<Literal>(0)));
  block->add_statement(std::make_unique<VariableDeclaration>(ValueType::Int, "b", std::make_unique<Literal>(1)));
  
  std::vector<std::unique_ptr<Ast>> loop_statements;
  loop_statements.push_back(std::make_unique<Assignment>(std::make_unique<Identifier>("result"), std::make_unique<Literal>(0)));
  loop_statements.push_back(std::make_unique<Assignment>(std::make_unique<Identifier>("a"), std::make_unique<Literal>(0)));
  loop_statements.push_back(std::make_unique<Assignment>(std::make_unique<Identifier>("b"), std::make_unique<Literal>(1)));
  
  block->add_statement(
    std::make_unique<For>(
      std::make_unique<VariableDeclaration>(ValueType::Int, "i", std::make_unique<Literal>(0)),
      std::make_unique<LessThan>(std::make_unique<Identifier>("i"), std::make_unique<Identifier>("n")),
      std::make_unique<Increment>(std::make_unique<Identifier>("i")),
      std::make_unique<Block>(std::move(loop_statements))
    )
  );
  block->add_statement(std::make_unique<Return>(std::make_unique<Identifier>("result")));

  auto function_declaration = std::make_unique<FunctionDeclaration>(ValueType::Int, "fib", std::vector<std::unique_ptr<Ast>>(), std::move(block));

  std::cout << function_declaration->to_string(0) << std::endl;
  return 0;
}