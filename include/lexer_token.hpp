#pragma once

#include <vector>
#include <string>
#include "lexer_position.hpp"

namespace Lexer {
  enum class Type {
    Integer,
    Float,
    String,
    Identifier,
    Char,
    Operator,
  };

  enum class OperatorType {
    None,
    Plus,
    Minus,
    Multiply,
    Divide,
    Assign,
    BinOr,
    BinAnd,
    Pow,
    Not,
    LeftParen,
    RightParen,
    LeftSquareParen,
    RightSquareParen,
    LeftFigureParen,
    RightFigureParen,
    More,
    Less,
    Equals,
    NotEquals,
    MoreOrEquals,
    LessOrEquals,
    Or,
    And,
    Semicolon,
    Colon,
    Comma,
    At,
    HardArrowRight,
    ArrowRight,
    Increment,
    Decrement,
    Percent
  };
  
  class Token {
  public:
    Type type;
    OperatorType operatorType;
    std::string value;
    Position position; 

    void printTokenJSON();
    Token(const Type T, const std::string V, const Position pos);
  };

  typedef std::vector<Token> TokenList; 
}
