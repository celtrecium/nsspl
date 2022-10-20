#include "codefile.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include "compiler.hpp"
#include <iostream>

int main(int argc, char **argv) {
  (void) argc;

  CodeFile::CodeFile file(argv[1]);

  Lexer::Lexer lexer(file);
  Lexer::TokenList tlist = lexer.tokenize();

  try {
    Parser::Parser prs(tlist);

    Compiler::NonsenseCompiler comp(prs.stmts);
    
    std::cout << comp.asmCode;
  } catch(Parser::Error &e) {
    lexer.printError(e.token.position, e.error);
    return 1;
  }
  
  return 0;
}
