#include "codefile.hpp"

namespace CodeFile {
  CodeFile::CodeFile(const std::string &filename)
    : fileName(filename) {
    std::ifstream file(filename);
    std::stringstream sstream;

    sstream << file.rdbuf();
    fileData = sstream.str();
    fileName = filename;
  }
}
