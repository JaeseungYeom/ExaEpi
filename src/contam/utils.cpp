#include "utils.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iterator>

namespace Contam_gRPC {

bool checkFilename(const std::string& filename)
{
  std::filesystem::path p(filename);
  if (p.filename().empty() || p.filename() == "." || p.filename() == "..") {
    return false;
  }
  return true;
}

std::string addStrToFilenameBeforeExt(const std::string& filename, const std::string& str) {
  std::filesystem::path p(filename);

  // Get the filename without the extension
  std::string fnameWOExt = p.stem().string();

  // Get the extension (including the dot)
  std::string ext = p.extension().string();

  // Construct the new filename
  std::string newFilename = fnameWOExt + str + ext;

  // Create a new path with the modified filename
  std::filesystem::path newPath = p.parent_path() / newFilename;

  return newPath.string();
}

std::vector<char> loadFile(const std::string& fname)
{
  if (!checkFilename(fname)) {
    std::cerr << fname + " is not valid a filename!\n";
    return std::vector<char>{};
  }

  auto fsize = std::filesystem::file_size(fname);
  std::vector<char> content(fsize, 0u);
  std::ifstream ifs(fname, std::ios_base::in | std::ios_base::binary);
  if (!ifs) {
    std::cerr << "Cannot open " + fname + " for reading!\n";
    return std::vector<char>{};
  }
  ifs.read(&content[0], fsize);
  ifs.close();
  return content;
}

size_t writeFile(
  const std::string& fname,
  const std::string& content,
  std::ios_base::openmode mode)
{
  if (!checkFilename(fname)) {
    std::cerr << "Invalid filename: " + fname << std::endl;
    return static_cast<size_t>(0ul);
  }
  std::ofstream ofs(fname, mode | std::ios::binary);
  if (!ofs) {
    std::cerr << "Failed to open " << fname << " for writing." << std::endl;
    return static_cast<size_t>(0ul);
  }
  ofs.write(content.data(), content.size());
  ofs.close();
  return content.size();
}

void print(const std::vector<char>& data)
{
  std::copy(data.cbegin(), data.cend(), std::ostream_iterator<char>(std::cout, " "));
  std::cout << std::endl;
}

} // end of namespace
