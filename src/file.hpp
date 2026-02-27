#pragma once

#include "assert.hpp"
#include <fstream>
#include <vector>

namespace zephyr {
static std::vector<char> read_file(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  ZEPH_ENSURE(!file.is_open(), "Couldn't open file");

  size_t file_size = (size_t)file.tellg();

  std::vector<char> buffer(file_size);

  file.seekg(0);
  file.read(buffer.data(), file_size);

  file.close();

  return buffer;
}

} // namespace zephyr
