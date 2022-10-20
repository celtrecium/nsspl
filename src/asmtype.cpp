#include "asmtype.hpp"

using namespace std;

AssemblerType::AssemblerType(string name, vector<string> basereg, size_t sz)
  : asmname(name), baseRegs(basereg), size(sz) {}

AssemblerType::AssemblerType() : size(0) {}
