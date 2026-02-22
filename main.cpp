#include "ast.h"
#include "bytecode.h"

using namespace kai::ast;

std::unique_ptr<Ast> lit(int v) { return std::make_unique<Ast::Literal>(v); }

std::unique_ptr<Ast> var(const char *name) { return std::make_unique<Ast::Variable>(name); }

std::unique_ptr<Ast::Variable> var_ref(const char *name) { return std::make_unique<Ast::Variable>(name); }

std::unique_ptr<Ast::Add> add(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::Add>(std::move(left), std::move(right));
}

std::unique_ptr<Ast::Multiply> mul(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::Multiply>(std::move(left), std::move(right));
}

std::unique_ptr<Ast::LessThan> lt(std::unique_ptr<Ast> left, std::unique_ptr<Ast> right) {
  return std::make_unique<Ast::LessThan>(std::move(left), std::move(right));
}

std::unique_ptr<Ast::VariableDeclaration> decl(const char *name, std::unique_ptr<Ast> initializer) {
  return std::make_unique<Ast::VariableDeclaration>(name, std::move(initializer));
}

std::unique_ptr<Ast::Assignment> assign(const char *name, std::unique_ptr<Ast> value) {
  return std::make_unique<Ast::Assignment>(name, std::move(value));
}

std::unique_ptr<Ast::Increment> inc(const char *name) { return std::make_unique<Ast::Increment>(var_ref(name)); }

std::unique_ptr<Ast::Return> ret(std::unique_ptr<Ast> value) {
  return std::make_unique<Ast::Return>(std::move(value));
}

std::unique_ptr<Ast::While> while_loop(std::unique_ptr<Ast::LessThan> condition,
                                       std::unique_ptr<Ast::Block> body) {
  return std::make_unique<Ast::While>(std::move(condition), std::move(body));
}

Ast::Block fibonacci_example() {
  // begin function declaration
  auto decl_body = std::make_unique<Ast::Block>();

  // declare n, i, t1, t2, t3
  decl_body->append(decl("n", lit(10)));
  decl_body->append(decl("t1", lit(0)));
  decl_body->append(decl("t2", lit(1)));
  decl_body->append(decl("t3", lit(0)));

  // for (int i = 0; i < n; i++) {
  //   t3 = t1 + t2;
  //   t1 = t2;
  //   t2 = t3;
  // }
  decl_body->append(decl("i", lit(0)));
  auto block = std::make_unique<Ast::Block>();
  block->append(assign("t3", add(var("t1"), var("t2"))));
  block->append(assign("t1", var("t2")));
  block->append(assign("t2", var("t3")));
  block->append(inc("i"));
  decl_body->append(while_loop(lt(var("i"), var("n")), std::move(block)));
  // end for loop

  decl_body->append(ret(var("t1")));
  // end function declaration
  return std::move(*decl_body);
}

Ast::Block factorial_example() {
  auto decl_body = std::make_unique<Ast::Block>();
  decl_body->append(decl("n", lit(5)));
  decl_body->append(decl("result", lit(1)));
  decl_body->append(decl("i", lit(1)));

  auto block = std::make_unique<Ast::Block>();
  block->append(assign("result", mul(var("result"), var("i"))));
  block->append(inc("i"));
  decl_body->append(while_loop(lt(var("i"), add(var("n"), lit(1))), std::move(block)));
  decl_body->append(ret(var("result")));
  return std::move(*decl_body);
}

std::unique_ptr<Ast::FunctionDeclaration> function_example() {
  auto decl_body = std::make_unique<Ast::Block>();
  decl_body->append(decl("a", lit(4)));
  decl_body->append(decl("b", lit(2)));
  decl_body->append(ret(add(var("a"), var("b"))));
  return std::make_unique<Ast::FunctionDeclaration>("sum", std::move(decl_body));
}

void test_fibonacci() {
  auto fib = fibonacci_example();
  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(fib);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 55);
}

void test_factorial() {
  auto factorial = factorial_example();
  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(factorial);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 120);
}

void test_function_declaration() {
  auto factorial = function_example();
  kai::bytecode::BytecodeGenerator gen;
  gen.visit(*factorial);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  assert(interp.interpret(gen.blocks()) == 6);
}

int main() {
  test_fibonacci();
  test_factorial();
  test_function_declaration();
  return 0;
}
