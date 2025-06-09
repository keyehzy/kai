#include "lexer.h"
#include <iostream>

// for (int i = 0; i < n; i++) {
//   t3 = t1 + t2;
//   t1 = t2;
//   t2 = t3;
// }

int main() {
  Lexer lex(
R"(
for (int i = 0; i < n; i++) {
  t3 = t1 + t2;
  t1 = t2;
  t2 = t3;
}
)");

  while(lex.peek().type != Token::Type::end_of_file) {
    std::cout << lex.peek().sv() << "\n";
    lex.skip();
  }
  return 0;
}
