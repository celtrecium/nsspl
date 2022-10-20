#pragma once

#include <cstdlib>

namespace Lexer {
  class Position {
  public:
    size_t lineNumber;
    size_t lineBegin;
    size_t lineEnd;
    size_t charNumber;

    Position(const size_t line, const size_t pos);
  };
}
