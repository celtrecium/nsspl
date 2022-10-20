#include "lexer_token.hpp"
#include <cstddef>
#include <iostream>

#define KEY(K)          "\033[1;38;5;2m" K "\033[m"
#define VALUE_NUMBER(V) "\033[38;5;4m" V "\033[m"
#define VALUE_STRING(V) "\033[1;38;5;9m" V "\033[m"
  
static std::string DBG_TYPENAMES[] = {
  "Integer",
  "Float",
  "String",
  "Identifier",
  "Char",
  "Operator"
};
    
  static std::string DBG_LEXER_OPTYPENAMES[] = {
    "None",
    "Plus",
    "Minus",
    "Multiply",
    "Divide",
    "Assign",
    "BinOr",
    "BinAnd",
    "Pow",
    "Not",
    "LeftParen",
    "RightParen",
    "LeftSquareParen",
    "RightSquareParen",
    "LeftFigureParen",
    "RightFigureParen",
    "More",
    "Less",
    "Equals",
    "NotEquals",
    "MoreOrEquals",
    "LessOrEquals",
    "Or",
    "And",
    "Semicolon",
    "Colon",
    "Comma",
    "At",
    "HardArrowRight",
    "ArrowRight"
  };
  
namespace Lexer {
  Token::Token(const Type T, const std::string V, const Position pos)
    : type(T), operatorType(OperatorType::None), value(V), position(pos) {}

  void Token::printTokenJSON() {
    std::cout <<
      "{ "
      KEY("\"type\"") ": "  VALUE_STRING("\"" << DBG_TYPENAMES[static_cast<size_t>(type)] << "\"") ", "
      KEY("\"optype\"") ": "  VALUE_STRING("\"" << DBG_LEXER_OPTYPENAMES[static_cast<size_t>(operatorType)] << "\"") ", "
      KEY("\"value\"") ": " VALUE_STRING("\"" << value << "\"") ", "
      "}, \n";
  }
}
