#pragma once

#include "asmtype.hpp"
#include <string>

#define NAT_TYPE "qword"
#define NAT_TYPE_SIZE 8
#define NAT_AX "rax"
#define NAT_BX "rbx"
#define NAT_DX "rdx"
#define NAT_BP "rbp"
#define NAT_SP "rsp"

static AssemblerType NAT_ASMTYPE("qword", { "rax", "rbx", "rcx", "rdx" }, 8);

static std::string parametersRegList[] = {
  "rdi", "rsi", "rdx", "r10", "r8", "r5"
};
