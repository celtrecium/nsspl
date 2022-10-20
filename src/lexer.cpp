#include <iostream>
#include <sstream>
#include <unordered_map>
#include <stdexcept>
#include "codefile.hpp"
#include "lexer.hpp"
#include "lexer_token.hpp"

#define ERROR_C(e) "\033[1;31m" e "\033[m"
#define CUR_MOVE_RIGHT(x) "\033[" x "C"

inline bool isLetter(char ch) {
  return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

inline bool isDigit(char ch) { return ch >= '0' && ch <= '9'; }

inline std::string getUnderlineStr(const size_t len, const char ch = '^') {
  std::string underline;

  while(underline.length() != len)
    underline.push_back(ch);

  return underline;
}

namespace Lexer {
  static std::unordered_map<std::string, OperatorType> OPERATORS = {
    { "+", OperatorType::Plus },
    { "-", OperatorType::Minus },
    { "*", OperatorType::Multiply },
    { "/", OperatorType::Divide },
    { "=", OperatorType::Assign },
    { "|", OperatorType::BinOr },
    { "&", OperatorType::BinAnd },
    { "^", OperatorType::Pow },
    { "!", OperatorType::Not },
    { "(", OperatorType::LeftParen },
    { ")", OperatorType::RightParen },
    { "[", OperatorType::LeftSquareParen },
    { "]", OperatorType::RightSquareParen },
    { "{", OperatorType::LeftFigureParen },
    { "}", OperatorType::RightFigureParen },
    { ">", OperatorType::More },
    { "<", OperatorType::Less },
    { ":", OperatorType::Colon },
    { ";", OperatorType::Semicolon },
    { ",", OperatorType::Comma },
    { "==", OperatorType::Equals },
    { "!=", OperatorType::NotEquals },
    { ">=", OperatorType::MoreOrEquals },
    { "<=", OperatorType::LessOrEquals },
    { "||", OperatorType::Or },
    { "&&", OperatorType::And },
    { "@", OperatorType::At },
    { "=>", OperatorType::HardArrowRight },
    { "->", OperatorType::ArrowRight },
    { "++", OperatorType::Increment },
    { "--", OperatorType::Decrement },
    { "%", OperatorType::Percent },
  };

  Lexer::Lexer(const CodeFile::CodeFile &cfile)
    : position(Position(0, 0)), currentChar(cfile.fileData[0]), codeFile(cfile) {
    position.lineEnd = findLineEnd();
  }

  size_t Lexer::findLineEnd() {
    size_t i = position.lineBegin;

    while(codeFile.fileData[i] != 0 && codeFile.fileData[i++] != '\n');

    return codeFile.fileData[i] ? i - 1 : i;
  }
  
  char Lexer::nextChar() {
    if(currentChar == 0)
      return 0;

    currentChar = codeFile.fileData[++position.charNumber];

    if(currentChar == '\n') {
      ++position.lineNumber;
      position.lineBegin = position.charNumber + 1;
      position.lineEnd = findLineEnd();
    }

    return currentChar;
  }
  
  Token Lexer::getNumberToken() {
    Token token = Token(Type::Integer, "", position);

    do {
      if(currentChar == '.') {
        if(token.type == Type::Float)
          throw std::string("Unexpected second entry of char '.'");
        else
          token.type = Type::Float;
      }
      
      token.value.push_back(currentChar);
    } while(isDigit(nextChar()) || currentChar == '.');

    if(token.value[token.value.length() - 1] == '.')
      throw std::string("Unfinished float number entry");
    else if(isLetter(currentChar))
      throw std::string("Unexpected letter after number entry");
    
    return token;
  }


  char Lexer::replaceEscapedChar(char ch) {
    switch (ch) {
    case 'n':
      return '\n';
    case 't':
      return '\t';
    case 'e':
      return '\033';
    case 'b':
      return '\b';
    case 'r':
      return '\r';
    default:
      return ch;
    }
  }
  
  Token Lexer::getStringToken() {
    Token token = Token(Type::String, "", position);
    char prevChar = currentChar;
    bool escaped = false;

    while(nextChar() != '"' || prevChar == '\\') {
      if(currentChar == 0 || currentChar == '\n') {
        position = token.position;
        
        throw std::string("Missing terminating '\"' character");
      }

      if(prevChar == '\\' && !escaped) {
        escaped = true;
        token.value.push_back(replaceEscapedChar(currentChar));
      } else if(currentChar != '\\') {
        escaped = false;
        token.value.push_back(currentChar);
      } else {
        escaped = false;
      }

      prevChar = currentChar;
    }

    token.value.insert(0, "\"");
    token.value.push_back('\"');

    nextChar();
    
    return token;
  };
  
  Token Lexer::getCharToken() {
    Token token = Token(Type::Char, "", position);
    char prevChar = currentChar;

    while(nextChar() != '\'') {
      if(currentChar == 0 || currentChar == '\n') {
        position = token.position;
        
        throw std::string("Missing terminating ''' character");
      }
      
      if((prevChar != '\\' && token.value.length() == 1) ||
         (token.value.length() > 2)) {
        position = token.position;

        throw std::string("Multi-character constant");
      }

      if(prevChar == '\\')
        token.value.push_back(replaceEscapedChar(currentChar));
      else if(currentChar != '\\')
        token.value.push_back(currentChar);
      
      prevChar = currentChar;
    }

    token.value.insert(0, "'");
    token.value.push_back('\'');

    nextChar();

    return token;
  };

  Token Lexer::getIdentifierToken() {
    Token token = Token(Type::Identifier, "", position);

    do {
      token.value.push_back(currentChar);
    } while(isDigit(nextChar()) || isLetter(currentChar) || currentChar == '_');

    return token;
  }

  Token Lexer::getOperatorToken() {
    Token token = Token(Type::Operator, "", position);

    do {
      token.value.push_back(currentChar);
      
      if(!OPERATORS.count(token.value)) {
        token.value.pop_back();

        if(OPERATORS.count(token.value))
          token.operatorType = OPERATORS[token.value];
        else {
          token.value.push_back(currentChar);
          
          throw std::string("Unexpected token '" + token.value + "'");
        }

        break;
      }
    } while(nextChar() != 0);
    
    return token;
  }

  void Lexer::printError(const Position pos, const std::string &errMsg, const size_t underlineLen) {
    size_t errorCharNumInString = pos.charNumber - pos.lineBegin;
    size_t errorPtrPosition     = errorCharNumInString + std::to_string(pos.lineNumber+1).length() + 4;
    std::string errorCodeString = codeFile.fileData.substr(pos.lineBegin, pos.lineEnd - pos.lineBegin);
    
    std::cerr << std::endl
              << codeFile.fileName << ':' << pos.lineNumber + 1 << ':' << errorCharNumInString + 1 << ": " << ERROR_C("Error") ": " << errMsg << std::endl
              << ' ' << pos.lineNumber + 1 << " | " << errorCodeString << std::endl
              << CUR_MOVE_RIGHT(<< errorPtrPosition <<) ERROR_C(<< getUnderlineStr(underlineLen) <<) << std::endl;
  }

  TokenList Lexer::tokenize() {
    do {
      try {
      if(currentChar == ' ' || currentChar == '\n')
        nextChar();
      else if(isDigit(currentChar))
        tokenList.push_back(getNumberToken());
      else if(isLetter(currentChar) || currentChar == '_')
        tokenList.push_back(getIdentifierToken());
      else if(currentChar == '"')
        tokenList.push_back(getStringToken());
      else if(currentChar == '\'')
        tokenList.push_back(getCharToken());
      else
        tokenList.push_back(getOperatorToken());
      } catch(std::string e) {
        printError(position, e);
        
        tokenList.clear();

        return tokenList;
      }
    } while(currentChar != 0);
    
    return tokenList;
  }
}
