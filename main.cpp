#include "ast.h"
#include "bytecode.h"

using namespace kai::ast;

Ast::Block fibonacci_example() {
  // begin function declaration
  auto decl_body = std::make_unique<Ast::Block>();

  // declare n, i, t1, t2, t3
  decl_body->append<Ast::VariableDeclaration>("n", std::make_unique<Ast::Literal>(10));
  decl_body->append<Ast::VariableDeclaration>("t1", std::make_unique<Ast::Literal>(0));
  decl_body->append<Ast::VariableDeclaration>("t2", std::make_unique<Ast::Literal>(1));
  decl_body->append<Ast::VariableDeclaration>("t3", std::make_unique<Ast::Literal>(0));

  // for (int i = 0; i < n; i++) {
  //   t3 = t1 + t2;
  //   t1 = t2;
  //   t2 = t3;
  // }
  decl_body->append<Ast::VariableDeclaration>("i", std::make_unique<Ast::Literal>(0));
  auto less_than =
      std::make_unique<Ast::LessThan>(std::make_unique<Ast::Variable>("i"), std::make_unique<Ast::Variable>("n"));
  auto block = std::make_unique<Ast::Block>();
  block->append<Ast::Assignment>(
      "t3", std::make_unique<Ast::Add>(std::make_unique<Ast::Variable>("t1"), std::make_unique<Ast::Variable>("t2")));
  block->append<Ast::Assignment>("t1", std::make_unique<Ast::Variable>("t2"));
  block->append<Ast::Assignment>("t2", std::make_unique<Ast::Variable>("t3"));
  block->append<Ast::Increment>(std::make_unique<Ast::Variable>("i"));
  auto while_loop = std::make_unique<Ast::While>(std::move(less_than), std::move(block));
  decl_body->children.push_back(std::move(while_loop));
  // end for loop

  decl_body->append<Ast::Return>(std::make_unique<Ast::Variable>("t1"));
  // end function declaration
  return std::move(*decl_body);
}

int main() {
  auto fib = fibonacci_example();
  kai::bytecode::BytecodeGenerator gen;
  gen.visit_block(fib);
  gen.finalize();
  gen.dump();

  kai::bytecode::BytecodeInterpreter interp;
  std::cout << interp.interpret(gen.blocks()) << "\n";

  return 0;
}
