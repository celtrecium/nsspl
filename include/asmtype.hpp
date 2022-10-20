#pragma once

#include <string>
#include <vector>

class AssemblerType {
public:
  std::string asmname;
  std::vector<std::string> baseRegs;
  size_t size;

  AssemblerType(std::string name, std::vector<std::string> baseregs, size_t sz);
  AssemblerType();
};

