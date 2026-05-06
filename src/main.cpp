#include "verify.hpp"
#include "generate.hpp"
#include "utils.hpp"
#include "sha256.hpp"
#include <iostream>
#include <chrono>
#include <string>
#include <cstdio>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    enable_ansi();

    auto start = std::chrono::steady_clock::now();

    if (argc < 2) {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " <file>                          (compute hash of a single file)\n"
                  << "  " << argv[0] << " <directory>                     (generate checksums)\n"
                  << "  " << argv[0] << " -c <checksumfile>               (verify checksums)\n";
        return 1;
    }

    bool success = false;

    if (std::string(argv[1]) == "-c") {
        if (argc < 3) {
            std::cerr << "Error: missing checksum file after -c\n";
            return 1;
        }
        fs::path checksum_file = argv[2];

        if (is_terminal(stdout)) {
            std::cout << CLEAR_SCREEN;
        }

        success = verify_checksums(checksum_file);
    } else if (argc == 2) {
        fs::path p = argv[1];
        std::error_code ec;

        if (fs::is_regular_file(p, ec)) {
            try {
                std::string hash = SHA256::from_file(p);
                std::cout << hash << " *" << to_string(p.u8string()) << '\n';
                success = true;
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << '\n';
                success = false;
            }
        } else if (fs::is_directory(p, ec)) {
            success = generate_checksums(p);
        } else {
            std::cerr << "Error: " << p << " is not a regular file or directory.\n";
            return 1;
        }
    } else {
        std::cerr << "Error: too many arguments. Use <directory> or -c <checksumfile>\n";
        return 1;
    }

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "Time taken: " << (duration / 1000.0) << " seconds\n";

    return success ? 0 : 1;
}
