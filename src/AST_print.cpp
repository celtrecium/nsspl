#include "AST.hpp"
#include <iostream>

using namespace std;

namespace AST {
  void StatementsNode::printJSON(string spaces) {
    cout << "{\n" << spaces << "  statements: ";

    for(auto i : statements) {
      if(i != nullptr) i->printJSON(spaces + "  ");

      cout << ", ";
    }

    cout << "\b\b\n";
    cout << spaces << "}\n";
  }
  
  void VariableNode::printJSON(string spaces) {
    cout << "{\n" << spaces << "  varType: ";
    cout << varTypeToken.value << ",\n";
    cout << spaces << "  modifiers: ";

    for(auto i : modifiers)
      i->printJSON(spaces + "  ");

    cout << '\n';
    cout << spaces << "  variable: ";
    cout << name.value << ",\n";

    if(body != nullptr) {
      cout << spaces << "  body: ";
      body->printJSON(spaces + "  ");
      cout << '\n';
    }
    
    cout << spaces << "}";
  }

  void ValueNode::printJSON(string spaces) {
    (void)spaces;
    cout << "{ value: ";
    cout << value.value;
    cout << " }";
  }

  void BinaryNode::printJSON(string spaces) {
    cout << "{\n" << spaces << "  operator: ";
    cout << op.value << ",\n";
    cout << spaces << "  left: ";
    left->printJSON(spaces + "  ");
    cout << ",\n" << spaces << "  right: ";
    right->printJSON(spaces + "  ");
    cout << '\n' << spaces << "}";
  }

  void UnaryNode::printJSON(string spaces) {
    cout << "{\n" << spaces << "  operator: ";
    cout << op.value << ",\n";
    cout << spaces << "  operand: ";
    node->printJSON(spaces + "  ");
    cout << '\n' << spaces << "}";
  }
  
  void FunctionNode::printJSON(string spaces) {
    cout << "{\n" << spaces << "  name: ";
    cout << name.value << ",\n";
    cout << spaces << "  type: ";
    cout << varTypeToken.value << ",\n";
    cout << spaces << "  parameters: ";
    parameters->printJSON(spaces + "  ");

    if(body != nullptr) {
      cout << ",\n" << spaces << "  body: ";
      body->printJSON(spaces + "  ");
      cout << '\n';
    }
    
    cout << spaces << "}";
  }

  void ParametersNode::printJSON(string spaces) {
    cout << "{ ";

    for(auto i : parameters) {
      i->printJSON(spaces + "  ");
      cout << ", ";
    }

    cout << "\b\b\n" << spaces << "}";
  }

  void IfStatementNode::printJSON(std::string spaces) {
    cout << "{\n" << spaces << "  condition: ";
    condition->printJSON(spaces + "  ");
    cout << ",\n" << spaces << "  if: ";
    ifstatement->printJSON(spaces + "  ");

    if(elsestatement != nullptr) {
      cout << ",\n" << spaces << "  else: ";
      elsestatement->printJSON();
      cout << '\n';
    }

    cout << spaces << "}";
  }

  void CycleStatementNode::printJSON(std::string spaces) {
    cout << "{\n" << spaces << "  condition: ";
    condition->printJSON(spaces + "  ");
    cout << ",\n" << spaces << "  statement: ";
    statement->printJSON(spaces + "  ");
    cout << '\n' << spaces << "}";
  }
}
