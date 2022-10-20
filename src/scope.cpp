#include "scope.hpp"
#include "arch.hpp"
#include "AST.hpp"

using namespace AST;

Function::Function(FunctionNode *node_)
  : node(node_), variablesOffset(0) {}

Variable &Scope::addVariable(AST::VariableNode *node_, AssemblerType asmtype) {
  return variables.insert({ node_->name.value, Variable(node_, 0, asmtype) }).first->second;
}

Variable &Function::addVariable(AST::VariableNode *node_, AssemblerType asmtype) {
  if(node_->modifiers.size() != 0 && node_->modifiers[0]->type == NodeType::BinaryOperator) {
    auto sa = Variable(node_, variablesOffset, asmtype);
    variables.insert({node_->name.value, sa});

    variablesOffset += sa.arraySizeInBytes;

    return variables.insert({ node_->name.value, sa }).first->second;
  }

  if(node_->exprType.isPointer)
    asmtype = NAT_ASMTYPE;

  Variable &var = variables.insert({ node_->name.value, Variable(node_, variablesOffset, asmtype) }).first->second;
  variablesOffset += asmtype.size;

  return var;
}

Variable &GlobalScope::addVariable(AST::VariableNode *node_, AssemblerType asmtype) {
  if(node_->modifiers.size() != 0 && node_->modifiers[0]->type == NodeType::BinaryOperator) {
    auto sa = Variable(node_, 0, asmtype);
    variables.insert({ node_->name.value, sa });

    return variables.insert({ node_->name.value, sa }).first->second;
  }

  if(node_->exprType.isPointer)
    asmtype = NAT_ASMTYPE;

  if(node_->modifiers.size() != 0 && node_->modifiers[0]->type == NodeType::BinaryOperator)
    variables.insert({ node_->name.value, Variable(node_, 0, asmtype)});

  Variable var = Variable(node_, 0, asmtype);

  return variables.insert({ node_->name.value, var }).first->second;
}
