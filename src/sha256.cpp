#include "sha256.hpp"
#include "utils.hpp"
#include <cstring>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void SHA256::Impl::reset() noexcept {
    state[0] = 0x6a09e667;
    state[1] = 0xbb67ae85;
    state[2] = 0x3c6ef372;
    state[3] = 0xa54ff53a;
    state[4] = 0x510e527f;
    state[5] = 0x9b05688c;
    state[6] = 0x1f83d9ab;
    state[7] = 0x5be0cd19;
    bit_count_ = 0;
    buffer_len_ = 0;
    std::memset(buffer_, 0, sizeof(buffer_));
}

void SHA256::Impl::update(const void* data, std::size_t len) noexcept {
    if (len == 0) return;
    auto bytes = static_cast<const unsigned char*>(data);

    if (buffer_len_ > 0) {
        std::size_t take = std::min(len, 64 - buffer_len_);
        std::memcpy(buffer_ + buffer_len_, bytes, take);
        buffer_len_ += take;
        bytes += take;
        len -= take;

        if (buffer_len_ == 64) {
            transform(state, buffer_);
            bit_count_ += 512;
            buffer_len_ = 0;
        }
    }

    while (len >= 64) {
        transform(state, bytes);
        bit_count_ += 512;
        bytes += 64;
        len -= 64;
    }

    if (len > 0) {
        std::memcpy(buffer_, bytes, len);
        buffer_len_ = len;
    }
}

std::array<unsigned char, 32> SHA256::Impl::final() noexcept {
    uint64_t total_bits = bit_count_ + buffer_len_ * 8;

    std::size_t i = buffer_len_;
    buffer_[i++] = 0x80;

    if (i > 56) {
        std::memset(buffer_ + i, 0, 64 - i);
        transform(state, buffer_);
        i = 0;
    }

    std::memset(buffer_ + i, 0, 56 - i);

    for (int j = 7; j >= 0; --j) {
        buffer_[56 + j] = static_cast<unsigned char>(total_bits & 0xFF);
        total_bits >>= 8;
    }

    transform(state, buffer_);

    std::array<unsigned char, 32> digest;
    for (int j = 0; j < 8; ++j) {
        digest[j * 4]     = static_cast<unsigned char>(state[j] >> 24);
        digest[j * 4 + 1] = static_cast<unsigned char>(state[j] >> 16);
        digest[j * 4 + 2] = static_cast<unsigned char>(state[j] >> 8);
        digest[j * 4 + 3] = static_cast<unsigned char>(state[j]);
    }

    return digest;
}

void SHA256::Impl::transform(uint32_t state[8], const unsigned char block[64]) noexcept {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3],
             e = state[4], f = state[5], g = state[6], h = state[7];
    uint32_t w[64];

    for (int i = 0; i < 16; ++i) {
        w[i] = (static_cast<uint32_t>(block[i * 4])     << 24) |
               (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(block[i * 4 + 2]) << 8)  |
               (static_cast<uint32_t>(block[i * 4 + 3]));
    }
    for (int i = 16; i < 64; ++i) {
        uint32_t s0 = (w[i - 15] >> 7) | (w[i - 15] << 25);
        s0 ^= (w[i - 15] >> 18) | (w[i - 15] << 14);
        s0 ^= (w[i - 15] >> 3);

        uint32_t s1 = (w[i - 2] >> 17) | (w[i - 2] << 15);
        s1 ^= (w[i - 2] >> 19) | (w[i - 2] << 13);
        s1 ^= (w[i - 2] >> 10);

        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    for (int i = 0; i < 64; ++i) {
        uint32_t S1 = (e >> 6) | (e << 26);
        S1 ^= (e >> 11) | (e << 21);
        S1 ^= (e >> 25) | (e << 7);

        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t temp1 = h + S1 + ch + K[i] + w[i];

        uint32_t S0 = (a >> 2) | (a << 30);
        S0 ^= (a >> 13) | (a << 19);
        S0 ^= (a >> 22) | (a << 10);

        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;

        h = g; g = f; f = e; e = d + temp1;
        d = c; c = b; b = a; a = temp1 + temp2;
    }

    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

SHA256::SHA256() noexcept : impl() {}

void SHA256::update(const void* data, std::size_t len) noexcept {
    impl.update(data, len);
}

std::array<unsigned char, 32> SHA256::final() noexcept {
    return impl.final();
}

std::string SHA256::from_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::string u8path = to_string(path.u8string());
        throw std::runtime_error("Cannot open file: " + u8path);
    }

    SHA256 ctx;
    char buf[65536];

    while (file.read(buf, sizeof(buf))) {
        ctx.update(buf, sizeof(buf));
    }
    if (file.gcount() > 0) {
        ctx.update(buf, static_cast<std::size_t>(file.gcount()));
    }
    if (file.bad() || (!file.eof() && file.fail())) {
        std::string u8path = to_string(path.u8string());
        throw std::runtime_error("Read error on file: " + u8path);
    }

    auto digest = ctx.final();
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : digest)
        oss << std::setw(2) << static_cast<int>(c);
    return oss.str();
}
