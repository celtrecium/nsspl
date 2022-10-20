#include "lexer_position.hpp"

namespace Lexer {
  Position::Position(const size_t line, const size_t pos)
    : lineNumber(line), lineBegin(0), lineEnd(0), charNumber(pos) {}
}
  
