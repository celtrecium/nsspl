#pragma once

#include <cstddef>
#include <stack>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>

#include "AST.hpp"
#include "variable.hpp"
#include "scope.hpp"

namespace Compiler {
  class NonsenseCompiler {
  private:
    AST::StatementsNode &tree;
    GlobalScope global;
    Scope *currentScope;

    std::unordered_map<std::string, AssemblerType> typesMap;

    std::string convertStringToNumbers(std::string str);
    AST::Type getValueType(AST::ValueNode *val);
    Variable &getVariable(AST::ValueNode *val);
    AST::Type getExpressionType(AST::Node *node);
    bool compareOperandsTypes(AST::Type &first, AST::Type &second);
    void compileStatement(AST::Node *stmt);
    void compileStatements(AST::StatementsNode *stmts);
    void compileAssign(AST::BinaryNode *bin);
    void compileFormula(AST::Node *val);
    void compileAssignLeftOperand(AST::Node *opd);
    void compileOptimizableBinaryOperator(AST::BinaryNode *bin);
    void compileNotOptimizableBinaryOperator(AST::BinaryNode *bin);
    void compileBinary(AST::BinaryNode *bin);
    void compileUnary(AST::UnaryNode *unr);
    void compileCall(AST::UnaryNode *fn);
    void compileVariableAddress(AST::ValueNode *varNode);
    void compileGlobalVariable(AST::ValueNode *varNode);
    void compileLocalVariable(AST::ValueNode *varNode);
    void compileVariable(AST::ValueNode *varNode);
    void compileVariableDeclaration(AST::VariableNode *var);
    void compileAsmIncluding(AST::ParametersNode *strings);
    void compileIfStatement(AST::IfStatementNode *ifstat);
    void compileWhileStatement(AST::CycleStatementNode *whilestat);
    void compileForStatement(AST::CycleStatementNode *forstat);
    void compileCycleStatement(AST::CycleStatementNode *whilestat);
    void compileValue(AST::ValueNode *val);
    void compileIndexToAssign(AST::BinaryNode *bin);
    void compileIndexInFormula(AST::BinaryNode *bin);
    void compileIndex(AST::BinaryNode *bin);
    void compileFunctionDeclaration(AST::FunctionNode *funcNode);
    void finalAssembly();
    
  public:
    std::string asmCode;
    NonsenseCompiler(AST::StatementsNode &tree_);

  };
}
