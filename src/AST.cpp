#include "AST.hpp"
#include <vector>
#include <iostream>

using namespace std;

namespace AST {
  // Node
  Node::Node(const NodeType T, Lexer::Token &beg) : type(T), begin(beg) {}
  Node::~Node() {}
  void Node::printJSON(string spaces) { (void)spaces; }

  // Statementsnode
  StatementsNode::StatementsNode(Lexer::Token &beg)
    : Node(NodeType::Statements, beg) {}

  StatementsNode::~StatementsNode() {
    for(auto n : statements)
      delete n;
  }
  
  void StatementsNode::addNode(Node *node) {
    statements.push_back(node);
  }

  Type::Type() : type(""), pointerLevel(0), isPointer(false) {}
  Type::Type(std::string type_, size_t ptrlvl, bool isPtr) : type(type_), pointerLevel(ptrlvl), isPointer(isPtr) {}
  
  bool Type::operator ==(const Type &t) {
    return type == t.type && pointerLevel == t.pointerLevel;
  }

  bool Type::operator !=(const Type &t) {
    return type != t.type || pointerLevel != t.pointerLevel;
  }

  bool Type::isNull() {
    return type == "";
  }
  
  // VariableNode
  VariableNode::VariableNode(Lexer::Token &T, vector<Node*> &mods, Lexer::Token &var,
                             Node *val, bool isext, Lexer::Token &beg)
    : Node(NodeType::Variable, beg),
      varTypeToken(T),
      name(var),
      modifiers(mods),
      body(val),
      isExtern(isext),
      isDefined(val != nullptr)
  {
    exprType = Type(T.value, mods.size(), mods.size() != 0);
  }

  VariableNode::~VariableNode() {
    if(body != nullptr)
      delete body;

    for(auto i : modifiers) {
      if(i != nullptr)
        delete i;
    }

    modifiers.clear();
    body = nullptr;
  }

  // ValueNode
  ValueNode::ValueNode(Lexer::Token &val, Lexer::Token &beg)
    : Node(NodeType::Value, beg), value(val) {}

  // BinaryNode
  BinaryNode::BinaryNode(Lexer::Token &op_, Node *left_, Node *right_, Lexer::Token &beg)
    : Node(NodeType::BinaryOperator, beg),
      op(op_), left(left_), right(right_) {}

  BinaryNode::~BinaryNode() {
    if(left != nullptr)
      delete left;
    
    if(right != nullptr)
      delete right;

    left = nullptr;
    right = nullptr;
  };

  // UnaryNode
  UnaryNode::UnaryNode(Lexer::Token &op_, Node *node_, Lexer::Token &beg)
    : Node(NodeType::UnaryOperator, beg), op(op_), node(node_) {}

  UnaryNode::~UnaryNode() {
    if(node != nullptr)
      delete node;

    node = nullptr;
  }

  // FunctionNode
  FunctionNode::FunctionNode(Lexer::Token &T, vector<Node*> mods, Lexer::Token &name_,
                             ParametersNode *parameters_, Node *body_, bool isext, Lexer::Token &beg)
    : VariableNode(T, mods, name_, body_, isext, beg),
      parameters(parameters_) {
    type = NodeType::Function;
  }

  FunctionNode::~FunctionNode() {
    if(parameters != nullptr)
      delete parameters;

    if(body != nullptr)
      delete body;

    for(auto i : modifiers) {
      delete i;
    }

    modifiers.clear();
    parameters = nullptr;
    body = nullptr;
  }

  // ParameterNode
  void ParametersNode::addParameter(Node *parameter) {
    parameters.push_back(parameter);
  }
  
  ParametersNode::ParametersNode(Lexer::Token &beg) : Node(NodeType::Parameters, beg) {}

  ParametersNode::~ParametersNode() {
    for(auto i : parameters)
      delete i;
  }
 
  IfStatementNode::IfStatementNode(Node *cond, Node *ifstat, Node *elsestat, Lexer::Token &beg)
    : Node(NodeType::IfStatement, beg), condition(cond), ifstatement(ifstat), elsestatement(elsestat) {}

  IfStatementNode::~IfStatementNode() {
    if(condition != nullptr)
      delete condition;
    
    if(ifstatement != nullptr)
      delete ifstatement;

    if(elsestatement != nullptr)
      delete elsestatement;

    condition = nullptr;
    ifstatement = nullptr;
    elsestatement = nullptr;
  }

  CycleStatementNode::CycleStatementNode(Node *cond, Node *stat, Lexer::Token &beg)
    : Node(NodeType::WhileStatement, beg), condition(cond), statement(stat) {}

  CycleStatementNode::~CycleStatementNode() {
    if(condition != nullptr)
      delete condition;
    
    if(statement != nullptr)
      delete statement;

    condition = nullptr;
    statement = nullptr;
  }
}
