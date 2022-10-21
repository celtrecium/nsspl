#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <iostream>
#include <functional>

#include "compiler.hpp"
#include "AST.hpp"
#include "lexer_token.hpp"
#include "parser.hpp"
#include "asmtype.hpp"
#include "variable.hpp"
#include "scope.hpp"
#include "arch.hpp"

using namespace Compiler;
using namespace Parser;
using namespace AST;
using namespace std;

inline bool isVariable(Node *node) {
  return node->type == NodeType::Value &&
    static_cast<ValueNode*>(node)->value.type == Lexer::Type::Identifier;
}

bool NonsenseCompiler::compareOperandsTypes(Type &first, Type &second) {
  if(first.type == "#ctint" || second.type == "#ctint")
    return true;

  AssemblerType &firstAsmType = first.pointerLevel != 0 ? NAT_ASMTYPE : typesMap.find(first.type)->second;
  AssemblerType &secondAsmType = second.pointerLevel != 0 ? NAT_ASMTYPE : typesMap.find(second.type)->second;

  return firstAsmType.asmname == secondAsmType.asmname;
}

void NonsenseCompiler::compileVariableAddress(AST::ValueNode *varNode) {
  Variable &var = getVariable(varNode);

  if(varNode->exprType.isNull())
    varNode->exprType = var.node->exprType;

  if(currentScope->variables.find(varNode->value.value) != currentScope->variables.end())
    currentScope->text += "mov " NAT_AX ", " NAT_BP "\n" "sub " NAT_AX ", " + to_string(var.stackOffset) + '\n';
  else if(global.variables.find(varNode->value.value) != global.variables.end())
    currentScope->text += "mov " NAT_AX ", " + var.node->name.value + '\n';
}

void NonsenseCompiler::compileGlobalVariable(AST::ValueNode *varNode) {
  Variable &var = global.variables.find(varNode->value.value)->second;

  if(varNode->exprType.isNull())
    varNode->exprType = var.node->exprType;

  if(var.asmtype.asmname == NAT_TYPE) {
    currentScope->text +=
      "mov " NAT_AX ", " + var.asmtype.asmname + "[" + var.node->name.value + "]\n";
  } else {
    currentScope->text +=
      "movzx " NAT_AX ", " + var.asmtype.asmname + "[" + var.node->name.value + "]\n";
  }
}

void NonsenseCompiler::compileLocalVariable(AST::ValueNode *varNode) {
  Variable &var = currentScope->variables.find(varNode->value.value)->second;

  if(varNode->exprType.isNull())
    varNode->exprType = var.node->exprType;

  if(var.asmtype.asmname == NAT_TYPE) {
    currentScope->text +=
      "mov " NAT_AX ", " + var.asmtype.asmname + "[" NAT_BP "-" + to_string(var.stackOffset) + "]\n";
  } else {
    currentScope->text +=
      "movzx " NAT_AX ", " + var.asmtype.asmname + "[" NAT_BP "-" + to_string(var.stackOffset) + "]\n";
  }
}

void NonsenseCompiler::compileVariable(AST::ValueNode *varNode) {
  if(currentScope->variables.find(varNode->value.value) != currentScope->variables.end()) {
    compileLocalVariable(varNode);
  } else if(global.variables.find(varNode->value.value) != global.variables.end()) {
    compileGlobalVariable(varNode);
  } else if(global.functions.find(varNode->value.value) != global.functions.end()) {
    Variable &var = currentScope->variables.find(varNode->value.value)->second;

    if(varNode->exprType.isNull())
      varNode->exprType = var.node->exprType;

    currentScope->text +=
      "mov " NAT_AX ", " + varNode->value.value + '\n';
  } else {
    throw Error(varNode->begin, "Undefined variable '" + varNode->value.value + "'");
  }
}

void NonsenseCompiler::compileValue(AST::ValueNode *val) {
  if(val->exprType.isNull())
    val->exprType = getValueType(val);

  switch(val->value.type) {
  case Lexer::Type::Integer:
    currentScope->text += "mov " NAT_AX ", " + val->value.value + '\n';
    break;

  case Lexer::Type::Char:
    currentScope->text += "mov " NAT_AX ", " + to_string((int)val->value.value[1]) + "\n";
    break;

  case Lexer::Type::String:
    global.stringLiterals.push_back(val->value.value);
    currentScope->text += "mov " NAT_AX ", __string_literal_" + to_string(global.stringLiterals.size()) + '\n';
    break;

  case Lexer::Type::Identifier: {
    auto var = getVariable(val);

    if(var.variableType == VariableType::StaticArray)
      compileVariableAddress(val);
    else
      compileVariable(val);

    if(val->exprType.isNull())
      val->exprType = getValueType(val);

    break;
  }

  default:
    throw Error(val->begin, "Not implemented #1");
  }

  if(val->exprType.isNull())
    val->exprType = getValueType(val);
}

static unordered_map<Lexer::OperatorType, string> BIN_OPERATORS_N_O = {
  { Lexer::OperatorType::Plus, "add " NAT_AX ", " NAT_BX "\n" },
  { Lexer::OperatorType::Minus, "sub " NAT_AX ", " NAT_BX "\n" },
  { Lexer::OperatorType::Multiply, "imul " NAT_BX "\n" },
  { Lexer::OperatorType::Divide, "mov " NAT_DX ", 0\n" "idiv " NAT_BX "\n" },
  { Lexer::OperatorType::Percent,   "mov " NAT_DX ", 0\n" "idiv " NAT_BX "\n" "mov " NAT_AX ", " NAT_DX "\n" },
  { Lexer::OperatorType::More, "cmp " NAT_AX ", " NAT_BX "\n" "setg al\n" "and al, 0x01\n" "movzx " NAT_AX ", al\n" },
  { Lexer::OperatorType::Less, "cmp " NAT_AX ", " NAT_BX "\n" "setl al\n" "and al, 0x01\n" "movzx " NAT_AX ", al\n" },
  { Lexer::OperatorType::Equals, "cmp " NAT_AX ", " NAT_BX "\n" "sete al\n" "and al, 0x01\n" "movzx " NAT_AX ", al\n" },
  { Lexer::OperatorType::NotEquals, "cmp " NAT_AX ", " NAT_BX "\n" "setne al\n" "and al, 0x01\n" "movzx " NAT_AX ", al\n" },
  { Lexer::OperatorType::And, "and " NAT_AX ", " NAT_BX "\n" },
  { Lexer::OperatorType::Or, "or " NAT_AX ", " NAT_BX "\n" },
};

static unordered_map<Lexer::OperatorType, function<string(string right)>> BIN_OPERATORS_O = {
  { Lexer::OperatorType::Plus, [](string right) -> string { return "add " NAT_AX ", " + right + "\n"; } },
  { Lexer::OperatorType::Minus, [](string right) -> string { return "sub " NAT_AX ", " + right + "\n"; } },
  { Lexer::OperatorType::More, [](string right) -> string {
      return "cmp " NAT_AX ", " + right + "\n" "setg al\n" "and al, 0x01\n" "movzx " NAT_AX ", al\n"; } },
  { Lexer::OperatorType::Less, [](string right) -> string {
      return "cmp " NAT_AX ", " + right + "\n" "setl al\n" "and al, 0x01\n" "movzx " NAT_AX ", al\n"; } },
  { Lexer::OperatorType::Equals, [](string right) -> string {
      return "cmp " NAT_AX ", " + right + "\n" "sete al\n" "and al, 0x01\n" "movzx " NAT_AX ", al\n"; } },
  { Lexer::OperatorType::NotEquals, [](string right) -> string {
      return "cmp " NAT_AX ", " + right + "\n" "setne al\n" "and al, 0x01\n" "movzx " NAT_AX ", al\n"; } },
  { Lexer::OperatorType::And, [](string right) -> string { return "and " NAT_AX ", " + right + "\n"; } },
  { Lexer::OperatorType::Or, [](string right) -> string { return "or " NAT_AX ", " + right + "\n"; } },
};

static unordered_map<string, pair<string, string>> TYPES_RES_LABELS = {
  { "qword", { "dq", "resq" } },
  { "byte", { "db", "resb" } }
};

void NonsenseCompiler::compileNotOptimizableBinaryOperator(BinaryNode *bin) {
  compileFormula(bin->left);
  currentScope->text += "push " NAT_AX "\n";
  compileFormula(bin->right);

  if(!compareOperandsTypes(bin->left->exprType, bin->right->exprType))
    throw Error(bin->begin, "Incompatible types of operands");

  currentScope->text +=
    "mov " NAT_BX ", " NAT_AX "\n"
    "pop " NAT_AX "\n" +
    BIN_OPERATORS_N_O[bin->op.operatorType];
}

void NonsenseCompiler::compileOptimizableBinaryOperator(BinaryNode *bin) {
  compileFormula(bin->left);

  if(bin->right->type == NodeType::Value) {
    auto vn = static_cast<ValueNode*>(bin->right);
    string right = vn->value.type == Lexer::Type::Char ? to_string((int)vn->value.value[1]) : vn->value.value;
    currentScope->text += BIN_OPERATORS_O[bin->op.operatorType](right);

    if(bin->right->exprType.isNull())
      bin->right->exprType = getValueType(static_cast<ValueNode*>(bin->right));
  } else {
    currentScope->text += "push " NAT_AX "\n";
    compileFormula(bin->right);

    if(!compareOperandsTypes(bin->left->exprType, bin->right->exprType))
      throw Error(bin->begin, "Incompatible types of operands");

    currentScope->text +=
      "mov " NAT_BX ", " NAT_AX "\n"
      "pop " NAT_AX "\n" +
      BIN_OPERATORS_N_O[bin->op.operatorType];
  }
}

Variable &NonsenseCompiler::getVariable(ValueNode *var) {
  if(currentScope->variables.find(var->value.value) != currentScope->variables.end())
    return currentScope->variables.find(var->value.value)->second;

  if(global.variables.find(var->value.value) != global.variables.end())
    return global.variables.find(var->value.value)->second;

  throw Error(var->begin, "Undefined variable '" + var->value.value + "'");
}

Type NonsenseCompiler::getValueType(ValueNode *val) {
  switch (val->value.type) {
  case Lexer::Type::String:
    return Type("byte", 1, false);

  case Lexer::Type::Integer:
    return Type("#ctint", 0, false);

  case Lexer::Type::Char:
    return Type("#ctint", 0, false);

  case Lexer::Type::Identifier:
    return getVariable(val).node->exprType;

  default:
    throw Error(val->begin, "Not implemented #4");
  }
}

Type NonsenseCompiler::getExpressionType(Node *node) {
  switch (node->type) {
  case NodeType::BinaryOperator:
    return static_cast<BinaryNode*>(node)->exprType;

  case NodeType::UnaryOperator:
    return static_cast<UnaryNode*>(node)->exprType;

  case NodeType::Value:
    return getValueType(static_cast<ValueNode*>(node));

  default:
    throw Error(node->begin, "Can't get type of this statement");
  }
}

void NonsenseCompiler::compileAssignLeftOperand(AST::Node *opd) {
  switch (opd->type) {
  case NodeType::BinaryOperator: {
    auto bin = static_cast<BinaryNode*>(opd);

    if(bin->op.operatorType == Lexer::OperatorType::LeftSquareParen)
      compileIndexToAssign(bin);
    else
      compileBinary(bin);

    break;
  }

  case NodeType::UnaryOperator: {
    auto unr = static_cast<UnaryNode*>(opd);

    if(unr->op.operatorType == Lexer::OperatorType::At && isVariable(unr->node)) {
      if(getVariable(static_cast<ValueNode*>(unr->node)).variableType == VariableType::StaticArray) {
        compileVariableAddress(static_cast<ValueNode*>(unr->node));
        unr->exprType = getValueType(static_cast<ValueNode*>(unr->node));
        --unr->exprType.pointerLevel;
      } else {
        compileVariable(static_cast<ValueNode*>(unr->node));
        unr->exprType = getValueType(static_cast<ValueNode*>(unr->node));
        --unr->exprType.pointerLevel;
      }

      break;
    }

    compileFormula(unr->node);

    if(unr->exprType.isNull()) {
      unr->exprType = unr->node->exprType;
      --unr->exprType.pointerLevel;
    }

    break;
  }

  case NodeType::Value:
    if(isVariable(opd))
      compileVariableAddress(static_cast<ValueNode*>(opd));
    else
      compileValue(static_cast<ValueNode*>(opd));

    break;

  default:
    throw Error(opd->begin, "Not implemented #2");
    break;
  }
}

void NonsenseCompiler::compileIndex(AST::BinaryNode *bin) {
  compileFormula(bin->left);
  currentScope->text += "push " NAT_AX "\n";
  compileFormula(bin->right);

  bin->exprType = bin->left->exprType;

  if(bin->exprType.pointerLevel == 0)
    throw Error(bin->op, "The indexing operation requires a pointer");

  --bin->exprType.pointerLevel;

  AssemblerType &asmtype = bin->exprType.pointerLevel != 0
    ? NAT_ASMTYPE
    : typesMap.find(bin->exprType.type)->second;

  currentScope->text +=
    "mov " NAT_BX ", " + to_string(asmtype.size) + "\n"
    "mul " NAT_BX "\n";

  currentScope->text +=
    "mov " NAT_BX ", " NAT_AX "\n"
    "pop " NAT_AX "\n" +
    BIN_OPERATORS_N_O[Lexer::OperatorType::Plus];
}

void NonsenseCompiler::compileIndexToAssign(AST::BinaryNode *bin) {
  compileIndex(bin);
}
void NonsenseCompiler::compileIndexInFormula(AST::BinaryNode *bin) {
  compileIndex(bin);

  AssemblerType &asmtype = bin->exprType.pointerLevel != 0
    ? NAT_ASMTYPE
    : typesMap.find(bin->exprType.type)->second;

  if(asmtype.asmname == NAT_ASMTYPE.asmname)
    currentScope->text += "mov " NAT_AX ", qword[" NAT_AX "]\n";
  else
    currentScope->text += "movzx " NAT_AX ", " + asmtype.asmname + "[" NAT_AX "]\n";
}

void NonsenseCompiler::compileAssign(AST::BinaryNode *bin) {
  auto presetType = bin->exprType;

  if(currentScope == &global)
    throw Error(bin->begin, "Unexpected assign in global");

  compileAssignLeftOperand(bin->left);
  currentScope->text += "push " NAT_AX "\n";
  compileFormula(bin->right);

  if(!compareOperandsTypes(bin->left->exprType, bin->right->exprType))
    throw Error(bin->begin, "Incompatible types of operands 1");

  bin->exprType = bin->left->exprType;
  AssemblerType &asmtype = bin->exprType.pointerLevel != 0
    ? NAT_ASMTYPE
    : typesMap.find(bin->exprType.type)->second;

  currentScope->text +=
    "mov " NAT_BX ", " NAT_AX "\n"
    "pop " NAT_AX "\n"
    "mov " + asmtype.asmname + "[" NAT_AX "], " + asmtype.baseRegs[1] + '\n';

  if(asmtype.asmname == NAT_TYPE)
    currentScope->text += "mov " NAT_AX ", " + asmtype.asmname + "[" NAT_AX "]\n";
  else
    currentScope->text += "movzx " NAT_AX ", " + asmtype.asmname + "[" NAT_AX "]\n";

  if(presetType.isNull())
    bin->exprType = bin->left->exprType;
}

void NonsenseCompiler::compileBinary(AST::BinaryNode *bin) {
  switch(bin->op.operatorType) {
  case Lexer::OperatorType::Assign:
    compileAssign(bin);
    return;
  case Lexer::OperatorType::LeftSquareParen:
    compileIndexInFormula(bin);
    return;
  default:
    break;
  }

  if(BIN_OPERATORS_O.find(bin->op.operatorType) != BIN_OPERATORS_O.end() && !isVariable(bin->right))
    compileOptimizableBinaryOperator(bin);
  else
    compileNotOptimizableBinaryOperator(bin);

  if(!compareOperandsTypes(bin->left->exprType, bin->right->exprType)) {
    throw Error(bin->begin, "Incompatible types of operands");
  }

  if(bin->exprType.isNull())
    bin->exprType = bin->left->exprType.type == "#ctint" ? bin->right->exprType : bin->left->exprType;
}

void NonsenseCompiler::compileAsmIncluding(ParametersNode *strings) {
  for(auto i : strings->parameters) {
    if(i->type == NodeType::Value && static_cast<ValueNode*>(i)->value.type == Lexer::Type::String) {
      auto &str = static_cast<ValueNode*>(i)->value.value;

      currentScope->text += str.substr(1, str.size() - 2) + '\n';
    } else {
      throw Error(i->begin, "Expected string literal");
    }
  }
}

void NonsenseCompiler::compileCall(AST::UnaryNode *fnNode) {
  ParametersNode *args = static_cast<ParametersNode*>(fnNode->node);
  auto func = global.functions.find(fnNode->op.value);

  if(func == global.functions.end())
    throw Error(fnNode->begin, "Undefined function");

  if(func->second.node->parameters->parameters.size() != args->parameters.size())
    throw Error(fnNode->begin, "Not enough arguments");

  if(func->second.node->parameters->parameters.size() < args->parameters.size())
    throw Error(args->begin, "Too many arguments");

  if(fnNode->exprType.isNull())
    fnNode->exprType = global.functions.find(fnNode->op.value)->second.node->exprType;

  for(size_t i = 0; i < args->parameters.size(); ++i) {
    Node *arg = args->parameters[i];

    if(arg->type == NodeType::Value && !isVariable(arg) &&
       static_cast<ValueNode*>(arg)->value.type != Lexer::Type::String) {
      auto val = static_cast<ValueNode*>(arg);

      if(val->value.type == Lexer::Type::Char)
        currentScope->text += "push " + to_string(static_cast<int>(val->value.value[1])) + '\n';
      else
        currentScope->text += "push " + val->value.value + '\n';
    } else {
      compileFormula(arg);

      if(func->second.node->parameters->parameters[i]->exprType != args->parameters[i]->exprType)
        throw Error(args->parameters[i]->begin, "Unexpected argument type");

      currentScope->text += "push " NAT_AX "\n";
    }
  }

  for(long int i = static_cast<long int>(args->parameters.size()) - 1; i >= 0 ; --i)
    currentScope->text += "pop " + parametersRegList[i] + '\n';

  currentScope->text += "call " + fnNode->op.value + '\n';
}

void NonsenseCompiler::compileIfStatement(AST::IfStatementNode *ifstat) {
  string labelnum = to_string((intptr_t)ifstat);

  compileFormula(ifstat->condition);
  currentScope->text += "cmp " NAT_AX ", 0x00\n" "je .endif_" + labelnum + '\n';

  if(ifstat->ifstatement->type == NodeType::Statements)
    compileStatements(static_cast<StatementsNode*>(ifstat->ifstatement));
  else
    compileStatement(ifstat->ifstatement);

  if(ifstat->elsestatement == nullptr) {
    currentScope->text += ".endif_" + labelnum + ":\n";
    return;
  }

  currentScope->text += "jmp .endelse_" + labelnum + '\n';
  currentScope->text += ".endif_" + labelnum + ":\n";

  if(ifstat->ifstatement->type == NodeType::Statements)
    compileStatements(static_cast<StatementsNode*>(ifstat->elsestatement));
  else
    compileStatement(ifstat->elsestatement);

  currentScope->text += ".endelse_" + labelnum + ":\n";

}

void NonsenseCompiler::compileWhileStatement(AST::CycleStatementNode *whilestat) {
  string labelnum = to_string((intptr_t)whilestat);

  currentScope->text += ".beginwhile_" + labelnum + ":\n";
  compileFormula(whilestat->condition);
  currentScope->text += "cmp " NAT_AX ", 0x00\n" "je .endwhile_" + labelnum + '\n';

  if(whilestat->statement->type == NodeType::Statements)
    compileStatements(static_cast<StatementsNode*>(whilestat->statement));
  else
    compileStatement(whilestat->statement);

  currentScope->text +=
    "jmp .beginwhile_" + labelnum + "\n "
    ".endwhile_" + labelnum +  ":\n";
}

void NonsenseCompiler::compileForStatement(AST::CycleStatementNode *forstat) {
  string labelnum = to_string((intptr_t)forstat);
  ParametersNode *args = static_cast<ParametersNode*>(forstat->condition);

  compileFormula(args->parameters[0]);
  currentScope->text += ".beginfor_" + labelnum + ":\n";
  compileFormula(args->parameters[1]);
  currentScope->text += "cmp " NAT_AX ", 0x00\n" "je .endfor_" + labelnum + '\n';

  if(forstat->statement->type == NodeType::Statements)
    compileStatements(static_cast<StatementsNode*>(forstat->statement));
  else
    compileStatement(forstat->statement);

  compileFormula(args->parameters[2]);
  currentScope->text +=
    "jmp .beginfor_" + labelnum + "\n "
    ".endfor_" + labelnum +  ":\n";
}

void NonsenseCompiler::compileUnary(AST::UnaryNode *unr) {
  if(unr->node->type == NodeType::Parameters) {
    compileCall(unr);
    return;
  }

  Type presetType = unr->exprType;

  switch (unr->op.operatorType) {
  case Lexer::OperatorType::HardArrowRight:
    compileFormula(unr->node);
    currentScope->text += "mov " NAT_SP ", " NAT_BP "\n" "pop " NAT_BP "\n" "ret\n";
    break;

  case Lexer::OperatorType::BinAnd:
    if(!isVariable(unr->node))
      throw Error(unr->begin, "Can't take adress of expression");

    unr->exprType = unr->node->exprType;
    compileVariableAddress(static_cast<ValueNode*>(unr->node));
    unr->exprType.isPointer = true;
    ++unr->exprType.pointerLevel;

    if(!presetType.isNull())
      unr->exprType = presetType;

    break;

  case Lexer::OperatorType::At: {
    compileFormula(unr->node);

    unr->exprType = unr->node->exprType;
    AssemblerType exprasmtype;

    if(unr->exprType.pointerLevel == 0) {
      throw Error(unr->begin, "Indirection requires pointer operand");
    }

    if(unr->exprType.pointerLevel != 1)
      exprasmtype = NAT_ASMTYPE;
    else
      exprasmtype = typesMap.find(unr->node->exprType.type)->second;

    if(exprasmtype.asmname == NAT_TYPE)
      currentScope->text += "mov " NAT_AX ", " + exprasmtype.asmname + "[" NAT_AX "]\n";
    else
      currentScope->text += "movzx " NAT_AX ", " + exprasmtype.asmname + "[" NAT_AX "]\n";

    --unr->exprType.pointerLevel;

    if(!presetType.isNull())
      unr->exprType = presetType;

    break;
  }

  default:
      throw Error(unr->op, "Unknown unary operator");
  }
}

void NonsenseCompiler::compileFormula(AST::Node *val) {
  switch (val->type) {
  case NodeType::BinaryOperator:
    compileBinary(static_cast<BinaryNode*>(val));
    break;

  case NodeType::UnaryOperator:
    compileUnary(static_cast<UnaryNode*>(val));
    break;

  case NodeType::Value:
    compileValue(static_cast<ValueNode*>(val));
    break;

  default:
    throw Error(val->begin, "Not implemented #5");
  }
}

void NonsenseCompiler::compileVariableDeclaration(AST::VariableNode *varNode) {
  if(typesMap.find(varNode->varTypeToken.value) == typesMap.end())
    throw Error(varNode->varTypeToken, "Unknown variable type");

  Variable &var = currentScope->addVariable(varNode, typesMap[varNode->varTypeToken.value]);

  if(currentScope != &global) {
    if(var.variableType == VariableType::StaticArray && var.node->body != nullptr) {
      if(varNode->body != nullptr)
        throw Error(varNode->body->begin, "Can't initialize array [Not implemented]");

    } else if(varNode->body != nullptr) {
      compileFormula(varNode->body);

      currentScope->text +=
        "mov " + var.asmtype.asmname + "[" NAT_BP "-" + to_string(var.stackOffset) + "], " +
        var.asmtype.baseRegs[0] + '\n';
    }

    return;
  }

  if(varNode->body != nullptr && varNode->body->type != NodeType::Value)
    throw Error(varNode->body->begin, "Global variable initializer isn't constant");

  if(varNode->body != nullptr) {
    auto &val = static_cast<ValueNode*>(varNode->body)->value;

    if(val.type == Lexer::Type::String) {
      global.stringLiterals.push_back(val.value);

      var.initializer = "__string_literal_" + to_string(global.stringLiterals.size());
    } else {
      var.initializer = static_cast<ValueNode*>(varNode->body)->value.value;
    }
  }
}

void NonsenseCompiler::compileCycleStatement(AST::CycleStatementNode *node) {
  if(node->condition->type == NodeType::Parameters)
    compileForStatement(node);
  else
    compileWhileStatement(node);
}

void NonsenseCompiler::compileStatement(AST::Node *stmt) {
    switch (stmt->type) {
    case NodeType::UnaryOperator:
      if(static_cast<UnaryNode*>(stmt)->op.value == "asm")
        compileAsmIncluding(static_cast<ParametersNode*>(static_cast<UnaryNode*>(stmt)->node));
      else
        compileFormula(stmt);

      break;
    case NodeType::BinaryOperator:
    case NodeType::Value:
      compileFormula(stmt);
      break;
    case NodeType::Variable:
      compileVariableDeclaration(static_cast<VariableNode*>(stmt));
      break;
    case NodeType::IfStatement:
      compileIfStatement(static_cast<IfStatementNode*>(stmt));
      break;
    case NodeType::WhileStatement:
      compileCycleStatement(static_cast<CycleStatementNode*>(stmt));
      break;
    default:
      throw Error(stmt->begin, "Not implemented #3");
    }
}

void NonsenseCompiler::compileStatements(AST::StatementsNode *stmts) {
  for(auto i : stmts->statements)
    compileStatement(i);
}

void NonsenseCompiler::compileFunctionDeclaration(FunctionNode *funcNode) {
  Function func(funcNode);
  currentScope = &func;
  auto &parameters = static_cast<ParametersNode*>(func.node->parameters)->parameters;

  Variable *var = nullptr;

  if(parameters.size() > sizeof(parametersRegList) / sizeof(string))
    throw Error(func.node->begin, "Too many function parameters(more than 6)");

  for(size_t i = 0; i < parameters.size(); ++i) {
    auto parameter = static_cast<VariableNode*>(parameters[i]);

    if(typesMap.find(parameter->varTypeToken.value) != typesMap.end())
      var = &func.addVariable(parameter, typesMap[parameter->varTypeToken.value]);
    else
      throw Error(parameter->varTypeToken, "Unknown variable type");

    if(var->asmtype.asmname == NAT_TYPE)
      func.text +=
        "mov " + var->asmtype.asmname + "[" NAT_BP "-" + to_string(var->stackOffset) + "], " +
        parametersRegList[i] + '\n';
    else
      func.text +=
        "mov " NAT_AX ", " + parametersRegList[i] + "\n"
        "mov " + var->asmtype.asmname + "[" NAT_BP "-" + to_string(var->stackOffset) + "], " +
        var->asmtype.baseRegs[0] + '\n';
  }

  if(func.node->body == nullptr) {
    func.text.clear();
    global.functions.insert({ func.node->name.value, func });

    return;
  }

  if(func.node->body->type == NodeType::Statements)
    compileStatements(static_cast<StatementsNode*>(func.node->body));
  else
    compileFormula(func.node->body);

  func.text +=
    "mov " NAT_SP ", " NAT_BP "\n"
    "pop " NAT_BP "\n"
    "ret\n";

  currentScope->text.insert(
    0,
    func.node->name.value + ":\n"
    "push " NAT_BP "\n"
    "mov " NAT_BP ", " NAT_SP "\n" +
    (func.variablesOffset != 0 ? "sub " NAT_SP ", " + to_string(func.variablesOffset) + '\n' : ""));

  global.functions.insert({funcNode->name.value, func});
}

string NonsenseCompiler::convertStringToNumbers(string str) {
  string nums;

  for(char i : str) {
    nums += to_string(static_cast<int>(i)) + ", ";
  }

  nums.pop_back();
  nums.pop_back();

  return nums;
}

void NonsenseCompiler::finalAssembly() {
  asmCode += "section .text\n";
  asmCode += global.functions.find("_start") != global.functions.end() ? "global _start\n" : "";
  asmCode += global.text;

  for(auto &i : global.functions) {
    if(i.second.node->isExtern)
      asmCode += "extern " + i.second.node->name.value + '\n';
  }

  for(auto i : global.variables) {
    if(i.second.node->isExtern)
      asmCode += "extern " + i.second.node->name.value + '\n';
  }

  for(auto &i : global.functions) {
    asmCode += i.second.text;
  }

  asmCode += "section .data\n";

  for(size_t i = 0; i < global.stringLiterals.size(); ++i)
    asmCode +=
      "__string_literal_" + to_string(i + 1) + " db " +
      convertStringToNumbers(global.stringLiterals[i].substr(1, global.stringLiterals[i].size() - 2)) + ", 0x00\n";

  for(auto &i : global.variables) {
    if(!i.second.initializer.empty()) {
      if(i.second.initializer[0] == '"') {
        asmCode += i.second.node->name.value + ' ' + TYPES_RES_LABELS[i.second.asmtype.asmname].first +
          ' ' + convertStringToNumbers(i.second.initializer.substr(1, i.second.initializer.size() - 2)) + ", 0x00\n";
      } else if(i.second.initializer[0] == '\'') {
        asmCode += i.second.node->name.value + ' ' + TYPES_RES_LABELS[i.second.asmtype.asmname].first +
          ' ' + to_string(static_cast<int>(i.second.initializer[1])) + ", 0x00\n";
      } else {
        asmCode += i.second.node->name.value + ' ' + TYPES_RES_LABELS[i.second.asmtype.asmname].first +
          ' ' + i.second.initializer + '\n';
      }
    }
  }

  asmCode += "section .bss\n";

  for(auto &i : global.variables)
    if(i.second.initializer.empty() && (!i.second.node->isExtern || i.second.node->isDefined))
      asmCode += i.second.node->name.value + " resb " +
        to_string(i.second.variableType == VariableType::StaticArray
                  ? i.second.arraySizeInBytes
                  : i.second.asmtype.size) + '\n';
}

NonsenseCompiler::NonsenseCompiler(StatementsNode &tree_)
    : tree(tree_), currentScope(static_cast<Scope *>(&global)),
      typesMap({ { "i64",   AssemblerType("qword", { "rax", "rbx", "rcx", "rbx"}, 8)},
                 { "i32",   AssemblerType("dword", { "eax", "ebx", "ecx", "edx" }, 4) },
                 { "byte",  AssemblerType("byte",  { "al", "bl" }, 1) },
                 { "void",  AssemblerType("",      { "rax", "rbx", "rcx", "rdx" }, 0) }}) {
  for(auto i : tree.statements) {
    currentScope = static_cast<Scope*>(&global);

    switch (i->type) {
    case NodeType::Function:
      compileFunctionDeclaration(static_cast<FunctionNode*>(i));
      break;

    case NodeType::Variable:
      compileVariableDeclaration(static_cast<VariableNode*>(i));
      break;

    default:
      throw Error(i->begin, "Expected function or variable declaration");
    }
  }

  finalAssembly();
}
