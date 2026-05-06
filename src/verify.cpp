#include "verify.hpp"
#include "sha256.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <algorithm>

namespace fs = std::filesystem;

static inline std::string ltrim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    return (start == std::string::npos) ? "" : s.substr(start);
}

static inline std::string rtrim(const std::string& s) {
    size_t end = s.find_last_not_of(" \t\r\n");
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

static bool is_valid_sha256_hex(const std::string& s) {
    if (s.size() != 64) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
}

static void print_status(const std::string& status, std::string_view color, const std::string& filename) {
    const int INNER_WIDTH = 10;
    int len = static_cast<int>(utf8_length(status));
    int pad = INNER_WIDTH - len;
    if (pad < 0) pad = 0;
    int left_pad = (pad + 1) / 2;
    int right_pad = pad / 2;
    std::cout << "[" << std::string(left_pad, ' ')
              << color << status << COLOR_RESET
              << std::string(right_pad, ' ') << "] -- " << filename << '\n';
}

bool verify_checksums(const fs::path& checksum_file) {
    std::ifstream infile(checksum_file, std::ios::binary);
    if (!infile) {
        std::cerr << "Error: cannot open checksum file: " << checksum_file << '\n';
        return false;
    }

    char bom[3];
    infile.read(bom, 3);
    bool bom_detected = (infile.gcount() == 3 &&
                         static_cast<unsigned char>(bom[0]) == 0xEF &&
                         static_cast<unsigned char>(bom[1]) == 0xBB &&
                         static_cast<unsigned char>(bom[2]) == 0xBF);
    if (!bom_detected) {
        infile.seekg(0);
    }

    fs::path base_dir = checksum_file.parent_path();
    if (base_dir.empty()) base_dir = ".";
    fs::path canon_base_dir;
    std::error_code ec_base;
    canon_base_dir = fs::weakly_canonical(base_dir, ec_base);
    bool have_canon_base = !ec_base;
    if (!have_canon_base) {
        std::cerr << "Warning: cannot resolve base directory " << base_dir
                  << " (" << ec_base.message() << "), skipping containment check.\n";
    }

    std::string line;
    size_t total = 0, good = 0, bad = 0, missing = 0;

    while (std::getline(infile, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        line = ltrim(line);
        if (line.empty()) continue;

        size_t space_pos = line.find(' ');
        if (space_pos == std::string::npos) {
            std::cerr << "Warning: invalid line (no space): " << line << '\n';
            continue;
        }

        std::string expected_hash = line.substr(0, space_pos);
        if (!is_valid_sha256_hex(expected_hash)) {
            std::cerr << "Warning: invalid SHA-256 hash on line: " << line << '\n';
            continue;
        }

        size_t name_start = line.find_first_not_of(" \t", space_pos + 1);
        if (name_start == std::string::npos) {
            std::cerr << "Warning: invalid line (no filename): " << line << '\n';
            continue;
        }

        if (line[name_start] == '*') {
            ++name_start;
            name_start = line.find_first_not_of(" \t", name_start);
            if (name_start == std::string::npos) {
                std::cerr << "Warning: invalid line (no filename after '*'): " << line << '\n';
                continue;
            }
        }

        std::string filename = line.substr(name_start);
        filename = rtrim(filename);

        total++;

        std::u8string u8_filename(filename.begin(), filename.end());
        fs::path raw_path = base_dir / fs::path(u8_filename);
        fs::path resolved;
        std::error_code ec2;
        resolved = fs::weakly_canonical(raw_path, ec2);

        if (ec2) {
            if (ec2 == std::errc::no_such_file_or_directory)
                print_status("Missing!", COLOR_RED_BOLD, filename);
            else
                print_status("Error!", COLOR_RED_BOLD, filename);
            (ec2 == std::errc::no_such_file_or_directory ? missing : bad)++;
            continue;
        }

        if (have_canon_base && !is_inside(resolved, canon_base_dir, true)) {
            print_status("Missing!", COLOR_RED_BOLD, filename);
            missing++;
            continue;
        }

        std::error_code ec_status;
        auto status = fs::status(resolved, ec_status);
        if (ec_status) {
            if (ec_status == std::errc::no_such_file_or_directory)
                print_status("Missing!", COLOR_RED_BOLD, filename);
            else
                print_status("Error!", COLOR_RED_BOLD, filename);
            (ec_status == std::errc::no_such_file_or_directory ? missing : bad)++;
            continue;
        }
        if (!fs::is_regular_file(status)) {
            print_status("NotFile!", COLOR_RED_BOLD, filename);
            std::cerr << "Warning: " << filename << " is not a regular file.\n";
            bad++;
            continue;
        }

        std::string actual_hash;
        try {
            actual_hash = SHA256::from_file(resolved);
        } catch (const std::exception& e) {
            std::cerr << "Error reading " << filename << ": " << e.what() << '\n';
            print_status("Error!", COLOR_RED_BOLD, filename);
            bad++;
            continue;
        }

        for (char& c : expected_hash)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        for (char& c : actual_hash)
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (expected_hash == actual_hash) {
            print_status("OK!", COLOR_GREEN, filename);
            good++;
        } else {
            print_status("BAD!", COLOR_RED_BOLD, filename);
            bad++;
        }
    }

    std::cout << "\nSummary: " << total << " files checked, "
              << good << " OK, " << bad << " bad, " << missing << " missing.\n";

    return (bad == 0 && missing == 0);
}
