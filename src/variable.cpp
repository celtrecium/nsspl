#include "variable.hpp"
#include "parser.hpp"
#include "arch.hpp"

using namespace std;
using namespace AST;
using namespace Parser;

Variable::Variable(VariableNode *node_, size_t stoffset, AssemblerType atype)
  : node(node_), asmtype(atype), stackOffset(stoffset + asmtype.size), arraySizeInBytes(0) {
  if(node_->modifiers.size() == 0) {
    variableType = VariableType::Variable;
    return;
  }

  if(node_->modifiers.size() != 0 &&
     node_->modifiers[0]->type == NodeType::Value &&
     static_cast<ValueNode*>(node_->modifiers[0])->value.type != Lexer::Type::Integer) {
    asmtype = NAT_ASMTYPE;
    variableType = VariableType::Variable;
    return;
  }

  variableType = VariableType::StaticArray;

  for(auto i : node_->modifiers) {
    if(i->type != NodeType::BinaryOperator) {
      if(static_cast<ValueNode*>(i)->value.operatorType == Lexer::OperatorType::At)
        asmtype = NAT_ASMTYPE;

      break;
    }

    auto val = static_cast<BinaryNode*>(i)->right;

    if(val->type != NodeType::Value)
      throw Error(i->begin, "Array dimension isn't compile-time constant");

    dimensionsSize.push_back(stoul(static_cast<ValueNode*>(val)->value.value));
  }
  
  size_t arraySize = 1;

  for(auto i : dimensionsSize)
    arraySize *= i;

  arraySizeInBytes = arraySize * asmtype.size;
  stackOffset = arraySizeInBytes + stoffset;
}
