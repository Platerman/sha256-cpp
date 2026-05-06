#include "utils.hpp"
#include <iostream>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

void enable_ansi() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    SetConsoleOutputCP(CP_UTF8);
#endif
}

bool is_terminal(FILE* stream) {
#ifdef _WIN32
    HANDLE h = (HANDLE)_get_osfhandle(_fileno(stream));
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD mode;
    return GetConsoleMode(h, &mode) != 0;
#else
    return isatty(fileno(stream)) != 0;
#endif
}

bool is_inside(const fs::path& path, const fs::path& base, bool quiet) {
    std::error_code ec;
    auto resolve = [&](const fs::path& p, const char* subject) -> std::optional<fs::path> {
        fs::path canon = fs::weakly_canonical(p, ec);
        if (!ec) return canon;
        std::string err_msg = ec.message();
        ec.clear();
        fs::path abs = fs::absolute(p, ec);
        if (!ec) return abs.lexically_normal();
        err_msg = ec.message();
        ec.clear();
        if (!quiet) std::cerr << "Warning: cannot resolve " << subject << " " << p << ": " << err_msg << '\n';
        return std::nullopt;
    };

    auto resolved_path = resolve(path, "path");
    auto resolved_base = resolve(base, "base");

    if (!resolved_path || !resolved_base) {
        return false;
    }

    auto it = resolved_path->begin(), end = resolved_path->end();
    auto it_base = resolved_base->begin(), end_base = resolved_base->end();
    for (; it_base != end_base; ++it, ++it_base) {
        if (it == end || *it != *it_base)
            return false;
    }
    return true;
}

size_t utf8_length(const std::string& s) {
    size_t len = 0;
    for (unsigned char c : s)
        if ((c & 0xC0) != 0x80) ++len;
    return len;
}
