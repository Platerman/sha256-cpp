#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <string_view>
#include <filesystem>
#include <cstdio>
#include <algorithm>
#include <iterator>

inline constexpr std::string_view COLOR_GREEN   = "\033[32m";
inline constexpr std::string_view COLOR_RED_BOLD = "\033[1;31m";
inline constexpr std::string_view COLOR_RESET    = "\033[0m";
inline constexpr std::string_view CLEAR_SCREEN   = "\033[2J\033[1;1H";

void enable_ansi();

inline std::string to_string(const std::u8string& u8s) {
    std::string result;
    result.reserve(u8s.size());
    std::transform(u8s.begin(), u8s.end(), std::back_inserter(result),
                   [](char8_t c) { return static_cast<char>(static_cast<unsigned char>(c)); });
    return result;
}

bool is_terminal(FILE* stream);

bool is_inside(const std::filesystem::path& path, const std::filesystem::path& base, bool quiet = false);

size_t utf8_length(const std::string& s);

#endif
