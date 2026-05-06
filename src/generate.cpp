#include "generate.hpp"
#include "sha256.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <algorithm>
#include <string>
#include <optional>
#include <random>
#include <sstream>
#include <cstdio>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace fs = std::filesystem;

static std::optional<fs::path> resolve_symlink(const fs::path& link, int max_depth = 40) {
    fs::path current = link;
    for (int i = 0; i < max_depth; ++i) {
        std::error_code ec;
        if (!fs::is_symlink(current, ec) || ec)
            return current;
        fs::path target = fs::read_symlink(current, ec);
        if (ec)
            return std::nullopt;
        if (target.is_relative())
            target = current.parent_path() / target;
        current = target;
    }
    return std::nullopt;
}

static fs::path make_temp_file(const fs::path& dir) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<unsigned long long> dis(0, 999999999999999ULL);
#ifdef _WIN32
    DWORD pid = GetCurrentProcessId();
#else
    pid_t pid = getpid();
#endif
    for (int tries = 0; tries < 100; ++tries) {
        std::ostringstream name;
        name << "checksum." << pid << "." << dis(gen) << ".tmp";
        fs::path tmp = dir / name.str();

        // Open exclusively: fail if file already exists (no TOCTOU race)
#ifdef _WIN32
        std::wstring wtmp = tmp.wstring();
        FILE* fp = _wfopen(wtmp.c_str(), L"wxb");
#else
        std::string ntmp = tmp.string();
        FILE* fp = std::fopen(ntmp.c_str(), "wxb");
#endif
        if (fp) {
            std::fclose(fp);
            return tmp;
        }
    }
    throw std::runtime_error("Cannot generate unique temporary file name after 100 attempts");
}

bool generate_checksums(const fs::path& dir) {
    std::vector<std::pair<std::string, fs::path>> file_entries;
    std::set<fs::path> visited_dirs;
    std::error_code ec;

    fs::path out_dir = dir;
    fs::path iter_target = dir;

    if (fs::is_symlink(dir, ec)) {
        if (ec) {
            std::cerr << "Warning: cannot check symlink status for " << dir << ": " << ec.message() << '\n';
            ec.clear();
        } else {
            auto resolved = resolve_symlink(dir);
            if (resolved) {
                ec.clear();
                if (fs::is_directory(*resolved, ec)) {
                    iter_target = *resolved;
                } else {
                    iter_target = dir;
                }
                ec.clear();
            }
        }
    }

    // Make iter_target absolute to ensure correct display paths later
    iter_target = fs::absolute(iter_target, ec);
    if (ec) {
        std::cerr << "Error: cannot resolve path " << dir << ": " << ec.message() << '\n';
        return false;
    }

    fs::path canon_base = fs::weakly_canonical(iter_target, ec);
    if (ec) {
        std::cerr << "Warning: cannot resolve base directory " << dir << ": " << ec.message() << '\n';
        canon_base = iter_target;
    }

    auto opts = fs::directory_options::skip_permission_denied;
    fs::recursive_directory_iterator it(iter_target, opts, ec);
    if (ec) {
        std::cerr << "Error: cannot open directory " << dir << ": " << ec.message() << '\n';
        return false;
    }

    fs::path out_path = out_dir / "checksum.txt";
    fs::path tmp_path = make_temp_file(out_dir);

    fs::path out_path_real = iter_target / "checksum.txt";
    fs::path tmp_path_real = iter_target / tmp_path.filename();

    for (; it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) {
            std::cerr << "Warning: " << ec.message() << " - skipping\n";
            ec.clear();
            continue;
        }

        const auto& path = it->path();

        bool skip = false;
        if (fs::equivalent(path, out_path, ec) || fs::equivalent(path, tmp_path, ec)) skip = true;
        ec.clear();
        if (!skip && (out_dir != iter_target))
            if (fs::equivalent(path, out_path_real, ec) || fs::equivalent(path, tmp_path_real, ec)) skip = true;
        ec.clear();
        if (skip) continue;

        if (it->is_directory()) {
            fs::path resolved_dir = fs::weakly_canonical(path, ec);
            if (ec) {
                std::cerr << "Warning: cannot resolve " << to_string(path.u8string())
                          << " - skipping recursion\n";
                it.disable_recursion_pending();
                ec.clear();
                continue;
            }
            if (!visited_dirs.insert(resolved_dir).second) {
                it.disable_recursion_pending();
                continue;
            }
        } else if (it->is_regular_file() || it->is_symlink()) {
            fs::path target_path = path;
            if (it->is_symlink()) {
                auto resolved = resolve_symlink(path);
                if (!resolved) {
                    std::cerr << "Warning: skipping symlink " << to_string(path.u8string())
                              << " (symlink chain too long or broken)\n";
                    continue;
                }
                target_path = *resolved;
                ec.clear();
                if (!fs::is_regular_file(target_path, ec) || ec) {
                    std::cerr << "Warning: skipping symlink " << to_string(path.u8string())
                              << " (target not a regular file)\n";
                    continue;
                }
                if (!is_inside(target_path, canon_base)) {
                    std::cerr << "Warning: skipping symlink " << to_string(path.u8string())
                              << " (target outside base directory)\n";
                    continue;
                }
            }

            std::u8string display;
            ec.clear();
            auto rel = fs::relative(path, iter_target, ec);
            if (!ec) {
                display = rel.u8string();
            } else {
                ec.clear();
                display = path.u8string();
            }
            std::string display_str = to_string(display);
            std::replace(display_str.begin(), display_str.end(), '\\', '/');
            file_entries.emplace_back(std::move(display_str), target_path);
        }
    }

    std::sort(file_entries.begin(), file_entries.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    std::ofstream out(tmp_path, std::ios::binary);
    if (!out) {
        std::cerr << "Error: cannot create temporary file " << tmp_path << '\n';
        return false;
    }

    bool has_error = false;

    for (const auto& [display_str, file_path] : file_entries) {
        std::string hash;
        try {
            hash = SHA256::from_file(file_path);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << '\n';
            has_error = true;
            continue;
        }

        out << hash << " *" << display_str << '\n';
        if (!out) {
            std::cerr << "Error: failed to write to " << tmp_path << '\n';
            has_error = true;
            break;
        }
    }

    out.close();

    if (has_error) {
        std::cerr << "One or more errors occurred, removing incomplete temporary file.\n";
        fs::remove(tmp_path, ec);
        return false;
    }

    fs::remove(out_path, ec);
    if (ec) {
        std::cerr << "Warning: could not remove existing " << out_path << ": " << ec.message() << '\n';
        ec.clear();
    }
    fs::rename(tmp_path, out_path, ec);
    if (ec) {
        std::cerr << "Error: cannot rename " << tmp_path << " to " << out_path
                  << ": " << ec.message() << '\n';
        fs::remove(tmp_path, ec);
        return false;
    }

    std::cout << "Generated " << out_path << " with " << file_entries.size() << " entries.\n";
    return true;
}
