#ifndef CONTAM_GRPC_UTILS_H
#define CONTAM_GRPC_UTILS_H
#include <string>
#include <vector>
#include <iostream>

namespace Contam_gRPC {

bool checkFilename(const std::string& filename);

/*! \brief Add a string to a filename before extension */
std::string addStrToFilenameBeforeExt(const std::string& filename, const std::string& str);

std::vector<char> loadFile(const std::string& fname);

/*! Write a binary file */
size_t writeFile(
  const std::string& fname,
  const std::string& content,
  std::ios_base::openmode mode = std::ios_base::out);

/*! Print the content of a data */
void print(const std::vector<char>& data);

} // end of namespace

#endif // CONTAM_GRPC_UTILS_H
