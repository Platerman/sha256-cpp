#ifndef VERIFY_HPP
#define VERIFY_HPP

#include <filesystem>

bool verify_checksums(const std::filesystem::path& checksum_file);

#endif
