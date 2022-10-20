#pragma once

#include "lexer_token.hpp"
#include "AST.hpp"
#include <functional>
#include <stack>
#include <utility>

namespace Parser {
  class Error {
  public:
    Lexer::Token &token;
    std::string error;

    Error(Lexer::Token &tok, std::string err);
  };

  class Parser {
  private:
    Lexer::TokenList &tokens;
    Lexer::TokenList::iterator current;

    void match(std::string l);
    Lexer::Token &match(Lexer::Type t);
    Lexer::Token &match(Lexer::OperatorType ot);
    void next();
    void clearOperatorsStack(std::stack<Lexer::Token*> &ops, std::stack<AST::Node*> &opds,
                             std::function<bool()> expr = []() -> bool { return true; });
    AST::Node *parseList(std::function<AST::Node*()> parseElement);
    
  public:
    AST::StatementsNode stmts;
    Parser(Lexer::TokenList &toks);

    AST::StatementsNode     *parseStatements();
    AST::Node               *parseStatement();
    AST::FunctionNode       *parseFunction();
    AST::VariableNode       *parseVariableDeclaration();
    AST::ValueNode          *parseValue();
    AST::ValueNode          *parseVariable();
    AST::UnaryNode          *parseCall();
    AST::Node               *parseFormula();
    AST::VariableNode       *parseParameter();
    AST::ParametersNode     *parseParameters();
    AST::Node               *parseParenthesisFormula();
    AST::ParametersNode     *parseArguments();
    AST::Node               *parseOperand();
    AST::UnaryNode          *parseUnaryOperator();
    AST::BinaryNode         *parseIndex();
    AST::BinaryNode         *parseIndexes(AST::Node *left);
    AST::IfStatementNode    *parseIfStatement();
    AST::CycleStatementNode *parseWhileStatement();
    AST::CycleStatementNode *parseForStatement();
    AST::Node               *parseReturn();
    AST::Node               *parseTypeModifier();
    std::vector<AST::Node*> parseTypeModifiers();
    std::pair<std::vector<AST::Node*>, Lexer::Token&> parseType();
  };

}
