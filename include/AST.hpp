#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include "lexer_token.hpp"

namespace AST {
  enum class NodeType {
    Statements,
    Variable,
    Value,
    BinaryOperator,
    UnaryOperator,
    Function,
    Parameters,
    IfStatement,
    WhileStatement,
    Type
  };

  class Type {
  public:
    std::string type;
    size_t pointerLevel;
    bool isPointer;

    bool operator ==(const Type &t);
    bool operator !=(const Type &t);
    bool isNull();

    Type();
    Type(std::string type_, size_t ptrlvl, bool isPtr);
  };


  class Node {
  public:
    NodeType type;
    Lexer::Token &begin;
    Type exprType;

    virtual void printJSON(std::string spaces = " ");

    Node(const NodeType T, Lexer::Token &beg);
    virtual ~Node();
  };

  class StatementsNode : public Node {
  public:
    std::vector<Node*> statements;
    Type exprType;

    void printJSON(std::string spaces) override;

    StatementsNode(Lexer::Token &beg);
    ~StatementsNode();
    void addNode(Node *node);
  };

  class VariableNode : public Node {
  public:
    Lexer::Token &varTypeToken;
    Lexer::Token &name;
    std::vector<Node*> modifiers;
    Node *body;
    bool isExtern;
    bool isDefined;

    void printJSON(std::string spaces) override;
    VariableNode(Lexer::Token &T, std::vector<Node*> &mods, Lexer::Token &var, Node *val, bool isext, Lexer::Token &beg);
    VariableNode(Lexer::Token &var, Lexer::Token &beg);
    ~VariableNode() override;
  };

  class ValueNode : public Node {
  public:
    Lexer::Token &value;

    void printJSON(std::string spaces) override;

    ValueNode(Lexer::Token &val, Lexer::Token &beg);
  };

  class BinaryNode : public Node {
  public:
    Lexer::Token &op;
    Node *left;
    Node *right;

    void printJSON(std::string spaces) override;

    BinaryNode(Lexer::Token &op_, Node *left_, Node *right_, Lexer::Token &beg);
    ~BinaryNode() override;
  };

  class UnaryNode : public Node {
  public:
    Lexer::Token &op;
    Node *node;

    void printJSON(std::string spaces) override;

    UnaryNode(Lexer::Token &op_, Node *node_, Lexer::Token &beg);
    ~UnaryNode() override;
  };

  class ParametersNode : public Node {
  public:
    std::vector<Node*> parameters;

    void printJSON(std::string spaces) override;
    void addParameter(Node *parameter);

    ParametersNode(Lexer::Token &beg);
    ~ParametersNode() override;
  };

  class FunctionNode : public VariableNode {
  public:
    ParametersNode *parameters;

    void printJSON(std::string spaces) override;

    FunctionNode(Lexer::Token &T, std::vector<Node*> mods, Lexer::Token &name_,
                 ParametersNode *parameters_, Node *body_, bool isext, Lexer::Token &beg);
    ~FunctionNode() override;
  };

  class IfStatementNode : public Node {
  public:
    Node *condition;
    Node *ifstatement;
    Node *elsestatement;

    void printJSON(std::string spaces) override;

    IfStatementNode(Node *cond, Node *ifstat, Node *elsestat, Lexer::Token &beg);
    ~IfStatementNode() override;
  };

  class CycleStatementNode : public Node {
  public:
    Node *condition;
    Node *statement;

    void printJSON(std::string spaces) override;

    CycleStatementNode(Node *cond, Node *stat, Lexer::Token &beg);
    ~CycleStatementNode() override;
  };

}
