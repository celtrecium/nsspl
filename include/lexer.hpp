#pragma once

#include "codefile.hpp"
#include "lexer_position.hpp"
#include "lexer_token.hpp"

namespace Lexer {
  class Lexer {
  private:
    Position position;
    char currentChar;
    const CodeFile::CodeFile codeFile;

    char replaceEscapedChar(char ch);
    size_t findLineEnd();
    char nextChar();
    Token getNumberToken();
    Token getStringToken();
    Token getCharToken();
    Token getIdentifierToken();
    Token getOperatorToken();
    
  public:
    TokenList tokenList;

    Lexer(const CodeFile::CodeFile &cfile);

    void printError(const Position pos, const std::string &errMsg, const size_t underlineLen = 1);
    TokenList tokenize(); 
  };
}
