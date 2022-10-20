#pragma once

#include <sstream>
#include <string>
#include <fstream>

namespace CodeFile {
  class CodeFile {
  public:
    std::string fileName;
    std::string fileData;

    CodeFile(const std::string &filename);
  };
}
