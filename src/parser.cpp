#include "AST.hpp"
#include "lexer_token.hpp"
#include <string>
#include <type_traits>
#include <unordered_map>
#include <stack>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>
#include "parser.hpp"

using namespace std;
using namespace AST;

static unordered_map<Lexer::OperatorType, size_t> OPERATION_PRIORITY = {
  { Lexer::OperatorType::Or,              1 },
  { Lexer::OperatorType::And,             1 },
  { Lexer::OperatorType::NotEquals,       2 },
  { Lexer::OperatorType::Equals,          2 },
  { Lexer::OperatorType::More,            2 },
  { Lexer::OperatorType::Less,            2 },
  { Lexer::OperatorType::Plus,            3 },
  { Lexer::OperatorType::Minus,           3 },
  { Lexer::OperatorType::Multiply,        4 },
  { Lexer::OperatorType::Divide,          4 },
  { Lexer::OperatorType::Percent,         4 },
  { Lexer::OperatorType::Assign,          0 }
};

static std::string LEXER_TYPENAMES[] = {
  "integer",
  "float",
  "string",
  "identifier",
  "char",
  "operator"
};

static std::string LEXER_OPERATORS[] = {
  "None", "+", "-",  "*",  "/",  "=",  "|",  "&",
  "^",    "!", "(",  ")",  "[",  "]",  "{",  "}",
  ">",    "<", "==", "!=", ">=", "<=", "||", "&&",
  ";",    ":", ",", "=>", "++", "--"
};

static unordered_map<std::string, bool> KEYWORDS = {
  { "var", true },
  { "if", true },
  { "while", true },
  { "for", true },
  { "fn", true },
};

namespace Parser {

Error::Error(Lexer::Token &tok, std::string err) : token(tok), error(err){};

Parser::Parser(Lexer::TokenList &toks)
    : tokens(toks), current(tokens.begin()), stmts(toks[0]) {
  while (current != tokens.end())
    stmts.addNode(parseStatement());
  }
  
  void Parser::match(string l) {
    if(current == tokens.end())
      throw Error(*(current - 1), "Unexpected end of file");

    if(current->value != l)
      throw Error(*current, "Expected '" + l + "' instead of '" + current->value + "'");

    next();
  }

  Lexer::Token &Parser::match(Lexer::Type t) {
    if(current == tokens.end())
      throw Error(*(current - 1), "Unexpected end of file");

    if(current->type != t)
      throw Error(*current, "Expected '" + LEXER_TYPENAMES[(size_t)t] + "' instead of '" + current->value + '\'');

    Lexer::Token &tok = *current;
    next();

    return tok;
  }

  Lexer::Token &Parser::match(Lexer::OperatorType ot) {
    if(current == tokens.end())
      throw Error(*(current - 1), "Unexpected end of file");
    
    if(current->operatorType != ot)
      throw Error(*current, "Expected '" + LEXER_OPERATORS[(size_t)ot] + "' instead of '" + current->value + '\'');

    Lexer::Token &tok = *current;
    next();

    return tok;
  }

  void Parser::next() {
    ++current;
  }

  FunctionNode *Parser::parseFunction() {
    auto begin = current;
    bool isExtern = true;
    
    if(current->value == "static") {
      isExtern = false;
      next();
    }
    
    if(current->value != "fn") {
      current = begin;
      return nullptr;
    }

    next();

    Lexer::Token &id = match(Lexer::Type::Identifier);
    ParametersNode *params = parseParameters();
    match(Lexer::OperatorType::Colon);
    pair<vector<Node*>, Lexer::Token&> type = parseType();

    if(current->operatorType != Lexer::OperatorType::Assign)
      return new FunctionNode(type.second, type.first, id, params, nullptr, isExtern, *begin);

    next();
    Node *body = parseStatements();

    if(body == nullptr)
      body = parseFormula();
    
    return new FunctionNode(type.second, type.first, id, params, body, isExtern, *begin);
  }
  
  StatementsNode *Parser::parseStatements() {
    auto begin = current;
    
    if(current->operatorType != Lexer::OperatorType::LeftFigureParen)
      return nullptr;

    next();
    
    StatementsNode *statements = new StatementsNode(*begin);

    while(current->operatorType != Lexer::OperatorType::RightFigureParen)
      statements->addNode(parseStatement());

    try {
      match(Lexer::OperatorType::RightFigureParen);
    } catch(Error &e) {
      delete statements;

      throw e;
    }
    
    return statements;
  }

  Node *Parser::parseReturn() {
    auto begin = current;
    
    if(current->operatorType != Lexer::OperatorType::HardArrowRight)
      return nullptr;

    Lexer::Token &op = *current;
    next();
    
    return new UnaryNode(op, parseFormula(), *begin);
  }
  
  Node *Parser::parseStatement() {
    Node *expr = nullptr;

    if((expr = parseVariableDeclaration()) != nullptr ||
       (expr = parseFunction())            != nullptr ||
       (expr = parseReturn())              != nullptr ||
       (expr = parseFormula())             != nullptr ||
       (expr = parseIfStatement())         != nullptr ||
       (expr = parseForStatement())        != nullptr ||
       (expr = parseWhileStatement())      != nullptr) {
      try {
        if(expr->type != NodeType::IfStatement &&
           expr->type != NodeType::WhileStatement)
          match(Lexer::OperatorType::Semicolon);
      } catch (Error &e) {
        if(expr != nullptr)
          delete expr;

        throw e;
      }

      return expr;
    }

    return nullptr;
  }

  Node *Parser::parseTypeModifier() {
    Node *modifier = nullptr;

    if(current->type != Lexer::Type::Identifier) {
      if(current->operatorType == Lexer::OperatorType::At) {
        modifier = new ValueNode(*current, *current);
        next();
      } else if(current->operatorType == Lexer::OperatorType::LeftSquareParen)
        modifier = parseIndex();
    }

    return modifier;
  }

  std::vector<AST::Node*> Parser::parseTypeModifiers() {
    Node *modifier = nullptr;
    vector<Node*> modifiers;

    while((modifier = parseTypeModifier()) != nullptr)
      modifiers.push_back(modifier);

    return modifiers;
  }

  pair<vector<Node*>, Lexer::Token&> Parser::parseType() {
    vector<Node*> modifiers = parseTypeModifiers();
    Lexer::Token *t = nullptr;

    try {
      t = &match(Lexer::Type::Identifier);
    } catch(Error &e) {
      for(auto i : modifiers)
        delete i;
      
      throw e;
    }

    return pair<vector<Node*>, Lexer::Token&>(modifiers, *t);
  }
  
  VariableNode *Parser::parseVariableDeclaration() {
    auto begin = current;
    bool isExtern = false;
    bool isDefined = false;

    if(current->value == "extern") {
      isExtern = true;
      next();
    }

    if(current->value == "def") {
      isDefined = true;
      next();
    }

    if(current->value != "var") {
      return nullptr;
    }

    next();
    Lexer::Token &id = match(Lexer::Type::Identifier);
    match(Lexer::OperatorType::Colon);

    pair<vector<Node *>, Lexer::Token&> type = parseType();

    if(current->operatorType != Lexer::OperatorType::Assign) {
      auto newvar = new VariableNode(type.second, type.first, id, nullptr, isExtern, *begin);
      newvar->isDefined = isDefined;

      return newvar;
    }

    match(Lexer::OperatorType::Assign);

    return new VariableNode(type.second, type.first, id, parseFormula(), isExtern, *begin);
  }
  
  static inline bool isValue(Lexer::Token &lex) {
    return
      lex.type == Lexer::Type::Char    ||
      lex.type == Lexer::Type::Float   ||
      lex.type == Lexer::Type::Integer ||
      lex.type == Lexer::Type::String;
  }
  
  ValueNode *Parser::parseValue() {
    Lexer::Token &val = *current;
    
    if(isValue(*current)) {
      next();
      
      return new ValueNode(val, val);
    }

    return nullptr;
  }

  ValueNode *Parser::parseVariable() {
    if(KEYWORDS.find(current->value) != KEYWORDS.end())
      return nullptr;
    
    ValueNode *val;
    
    if(current->type == Lexer::Type::Identifier) {
      val = new ValueNode(*current, *current);
      next();

      return val;
    }

    throw Error(*current, "Expected identifier instead of '" + current->value + '\'');
  }

  VariableNode *Parser::parseParameter() {
    
    Lexer::Token &id = match(Lexer::Type::Identifier);
    match(Lexer::OperatorType::Colon);
    pair<vector<Node*>, Lexer::Token&> type = parseType();

    return new VariableNode(type.second, type.first, id, nullptr, false, id);
  }

  Node *Parser::parseList(function<Node*()> parseElement) {
    auto begin = current;
    
    match(Lexer::OperatorType::LeftParen);
    
    ParametersNode *parameters = new ParametersNode(*begin);
    
    while(current->operatorType != Lexer::OperatorType::RightParen) {
      parameters->addParameter(parseElement());
      
      if(current->operatorType == Lexer::OperatorType::Comma) {
        next();
        continue;
      }

      if(current->operatorType == Lexer::OperatorType::RightParen) {
        next();
        return parameters;
      }

      throw Error(*current, "Unexpected token '" + current->value + '\'');
    }
    
    next();
    return parameters;
  }
  
  ParametersNode *Parser::parseParameters() {
    return static_cast<ParametersNode*>(parseList([this]() -> Node* {
      return static_cast<Node*>(parseParameter());
    }));
  }
  
  ParametersNode *Parser::parseArguments() {
    return static_cast<ParametersNode*>(parseList([this]() -> Node* {
      return static_cast<Node*>(parseFormula());
    }));
  }

  void Parser::clearOperatorsStack(stack<Lexer::Token*> &ops, stack<Node*> &opds, function<bool()> expr) {
    while (ops.size() != 0 && expr()) {
      Node *second = opds.top();
      opds.pop();
      Node *first = opds.top();
      opds.pop();

      opds.push(new BinaryNode(*ops.top(), first, second, *ops.top()));
      ops.pop();
    }
  }
  
  Node *Parser::parseParenthesisFormula() {
    if(current->operatorType != Lexer::OperatorType::LeftParen)
      return nullptr;

    auto begin = current;
    next();

    Node *f = parseFormula();

    if(f == nullptr)
      throw Error(*begin, "Expected formula");

    try {
      match(Lexer::OperatorType::RightParen);

      return f;
    } catch (Error &e) {
      delete f;

      throw e;
    }
  }
  
  Node *Parser::parseFormula() {
    stack<Node*> operands;
    stack<Lexer::Token*> operators;
    Node *operand = nullptr;

    while (true) {
      if((operand = parseOperand()) == nullptr) {
        while(operands.size() != 0) {
          delete operands.top();
          operands.pop();
        }
          
        return nullptr;
      }

      operands.push(operand);
      
      if(OPERATION_PRIORITY.find(current->operatorType) != OPERATION_PRIORITY.end()) {
        clearOperatorsStack(operators, operands, [this, operators]() -> bool {
          return OPERATION_PRIORITY[current->operatorType] <= OPERATION_PRIORITY[operators.top()->operatorType];
        });

        operators.push(&*current);
      } else {
        clearOperatorsStack(operators, operands);

        return operands.top();
      }

      next();
    }
    
    return nullptr;
  }
  
  UnaryNode *Parser::parseCall() {
    if(KEYWORDS.find(current->value) != KEYWORDS.end())
      return nullptr;
    
    auto begin = current;
    ParametersNode *args = nullptr;

    try {
      Lexer::Token &id = match(Lexer::Type::Identifier);
      args = parseArguments();
      
      return new UnaryNode(id, args, *begin);
    } catch(Error &e) {
      current = begin;
      
      return nullptr;
    }
  }

  BinaryNode *Parser::parseIndex() {
    if(current->operatorType != Lexer::OperatorType::LeftSquareParen)
      return nullptr;

    Lexer::Token &op = match(Lexer::OperatorType::LeftSquareParen);
    Node *f = parseFormula();

    try {
      match(Lexer::OperatorType::RightSquareParen);
    } catch(Error &e) {
      delete f;

      throw e;
    }

    return new BinaryNode(op, nullptr, f, op);
  }
  
  BinaryNode *Parser::parseIndexes(Node *left) {
    BinaryNode *node = nullptr;

    if((node = parseIndex()) != nullptr)
      node->left = left;
    else
      return nullptr;
      
    while((node = parseIndex()) != nullptr) {
    }

    return node;
  }
  
  UnaryNode *Parser::parseUnaryOperator() {
    if(current->operatorType != Lexer::OperatorType::Minus &&
       current->operatorType != Lexer::OperatorType::BinAnd &&
       current->operatorType != Lexer::OperatorType::At)
      return nullptr;

    Lexer::Token &op = match(Lexer::Type::Operator);
    Node *opd = parseOperand();
    
    return new UnaryNode(op, opd, op);
  }
  
  Node *Parser::parseOperand() {
    Node *operand = nullptr;
    auto begin = current;
    
    if((operand = parseValue())              != nullptr ||
       (operand = parseParenthesisFormula()) != nullptr ||
       (operand = parseCall())               != nullptr ||
       (operand = parseUnaryOperator())      != nullptr ||
       (operand = parseVariable())           != nullptr) {
      Node* opd = operand;
      Node* additNode = nullptr; // additional node

      if(current->value == "as") {
        next();
        pair<vector<Node*>, Lexer::Token&> t = parseType();
        
        opd->exprType = Type(t.second.value, t.first.size(), t.first.size() != 0);

        return opd;
      }

      if((additNode = parseIndex()) == nullptr)
        return operand;

      static_cast<BinaryNode*>(additNode)->left = opd;

      return additNode;
    }

    current = begin;
    return nullptr;
  }

  IfStatementNode *Parser::parseIfStatement() {
    auto begin = current;
    
    if(current->value != "if")
      return nullptr;

    next();
    Node *cond = nullptr;
    Node *ifstat = nullptr;
    Node *elsestat = nullptr;
    
    if((cond = parseParenthesisFormula()) == nullptr)
      throw Error(*current, "Expected if-condition");

    try {
      if((ifstat = parseStatements()) == nullptr &&
         (ifstat = parseStatement())  == nullptr) {
        delete cond;

        throw Error(*current, "Expected statement or block of statements");
      }

      if(current->value != "else")
        return new IfStatementNode(cond, ifstat, nullptr, *begin);

      next();
    
      if((elsestat = parseStatements()) == nullptr &&
         (elsestat = parseStatement())  == nullptr) {
        delete cond;
        delete ifstat;

        throw Error(*current, "Expected statement or block of statements");
      }

      return new IfStatementNode(cond, ifstat, elsestat, *begin);
    } catch (Error &e) {
      if(cond != nullptr)
        delete cond;

      if(ifstat != nullptr)
        delete ifstat;

      if(elsestat != nullptr)
        delete elsestat;

      throw e;
    }
  }

  CycleStatementNode *Parser::parseWhileStatement() {
    auto begin = current;
    
    if(current->value != "while")
      return nullptr;

    next();
    Node *cond = nullptr;
    Node *stat = nullptr;
    
    if((cond = parseParenthesisFormula()) == nullptr)
      throw Error(*current, "Expected while-condition");

    try {
      if((stat = parseStatements()) == nullptr &&
         (stat = parseStatement())  == nullptr) {
        delete cond;

        throw Error(*current, "Expected statement or block of statements");
      }

      return new CycleStatementNode(cond, stat, *begin);
    } catch (Error &e) {
      if(cond != nullptr)
        delete cond;

      if(stat != nullptr)
        delete stat;

      throw e;
    }
  }

  CycleStatementNode *Parser::parseForStatement() {
    auto begin = current;
    
    if(current->value != "for")
      return nullptr;

    next();
    Node *cond = nullptr;
    Node *stat = nullptr;
    
    if((cond = parseArguments()) == nullptr)
      throw Error(*current, "Expected for-condition");

    try {
      if((stat = parseStatements()) == nullptr &&
         (stat = parseStatement())  == nullptr) {
        delete cond;

        throw Error(*current, "Expected statement or block of statements");
      }

      return new CycleStatementNode(cond, stat, *begin);
    } catch (Error &e) {
      if(cond != nullptr)
        delete cond;

      if(stat != nullptr)
        delete stat;

      throw e;
    }
  }
}
