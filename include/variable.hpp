#pragma once

#include "AST.hpp"
#include "asmtype.hpp"

enum class VariableType {
  Variable,
  StaticArray
};

class Variable {
public:
  AST::VariableNode *node;
  std::string initializer;
  AssemblerType asmtype;
  size_t stackOffset;

  std::vector<size_t> dimensionsSize;
  size_t arraySizeInBytes;

  VariableType variableType;

  Variable(VariableType vtype, AST::VariableNode *node_, size_t stoffset, AssemblerType atype);
  Variable(AST::VariableNode *node_, size_t stoffset, AssemblerType atype);
};
