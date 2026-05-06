#ifndef SHA256_HPP
#define SHA256_HPP

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>
#include <filesystem>

class SHA256 {
public:
    SHA256() noexcept;
    void update(const void* data, std::size_t len) noexcept;
    std::array<unsigned char, 32> final() noexcept;
    static std::string from_file(const std::filesystem::path& path);

private:
    struct Impl {
        uint32_t state[8];
        uint64_t bit_count_;
        unsigned char buffer_[64];
        std::size_t buffer_len_;

        Impl() noexcept { reset(); }
        void reset() noexcept;
        void update(const void* data, std::size_t len) noexcept;
        std::array<unsigned char, 32> final() noexcept;
        static void transform(uint32_t state[8], const unsigned char block[64]) noexcept;
    };

    Impl impl;
};

#endif
