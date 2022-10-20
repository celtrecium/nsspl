#pragma once

#include <unordered_map>
#include <string>
#include "AST.hpp"
#include "variable.hpp"

class Scope {
public:
  std::unordered_map<std::string, Variable> variables;
  std::string text;
  virtual Variable &addVariable(AST::VariableNode *node_, AssemblerType asmtype);
};

class Function : public Scope {
public:
  AST::FunctionNode *node;
  size_t variablesOffset;
  
  Variable &addVariable(AST::VariableNode *node_, AssemblerType asmtype) override;
  
  Function(AST::FunctionNode *node_);
};

class GlobalScope : public Scope {
public:
  std::unordered_map<std::string, Function> functions;
  Variable &addVariable(AST::VariableNode *node_, AssemblerType asmtype) override;
  std::vector<std::string> stringLiterals;
};
