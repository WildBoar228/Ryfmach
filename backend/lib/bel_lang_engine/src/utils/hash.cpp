#include "utils/hash.hpp"

#include <bit>
#include <vector>

namespace ryfmach::utils {
namespace {

std::uint32_t LeftRotate(std::uint32_t value, int bits) noexcept {
    return std::rotl(value, bits);
}

std::uint64_t AddModulo(
    std::uint64_t left,
    std::uint64_t right,
    std::uint64_t modulo) noexcept {
    left %= modulo;
    right %= modulo;

    if (left >= modulo - right) {
        return left - (modulo - right);
    }
    return left + right;
}

std::uint64_t MultiplyModulo(
    std::uint64_t left,
    std::uint64_t right,
    std::uint64_t modulo) noexcept {
    std::uint64_t result = 0;
    left %= modulo;

    while (right > 0) {
        if (right & 1) {
            result = AddModulo(result, left, modulo);
        }
        right >>= 1;
        if (right > 0) {
            left = AddModulo(left, left, modulo);
        }
    }

    return result;
}

} // namespace

Sha1Digest Sha1(std::string_view input) {
    const std::uint64_t bit_count = static_cast<std::uint64_t>(input.size()) * 8;
    std::vector<std::uint8_t> message(input.begin(), input.end());
    message.push_back(0x80);
    while (message.size() % 64 != 56) {
        message.push_back(0);
    }
    for (int shift = 56; shift >= 0; shift -= 8) {
        message.push_back(static_cast<std::uint8_t>(bit_count >> shift));
    }

    std::uint32_t h0 = 0x67452301;
    std::uint32_t h1 = 0xEFCDAB89;
    std::uint32_t h2 = 0x98BADCFE;
    std::uint32_t h3 = 0x10325476;
    std::uint32_t h4 = 0xC3D2E1F0;

    for (std::size_t chunk = 0; chunk < message.size(); chunk += 64) {
        std::array<std::uint32_t, 80> words = {};
        for (std::size_t index = 0; index < 16; ++index) {
            const std::size_t offset = chunk + index * 4;
            words[index] =
                static_cast<std::uint32_t>(message[offset]) << 24 |
                static_cast<std::uint32_t>(message[offset + 1]) << 16 |
                static_cast<std::uint32_t>(message[offset + 2]) << 8 |
                static_cast<std::uint32_t>(message[offset + 3]);
        }
        for (std::size_t index = 16; index < words.size(); ++index) {
            words[index] = LeftRotate(
                words[index - 3] ^ words[index - 8] ^ words[index - 14] ^
                    words[index - 16],
                1);
        }

        std::uint32_t a = h0;
        std::uint32_t b = h1;
        std::uint32_t c = h2;
        std::uint32_t d = h3;
        std::uint32_t e = h4;

        for (std::size_t index = 0; index < words.size(); ++index) {
            std::uint32_t f = 0;
            std::uint32_t k = 0;
            if (index < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (index < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (index < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            const std::uint32_t temp = LeftRotate(a, 5) + f + e + k + words[index];
            e = d;
            d = c;
            c = LeftRotate(b, 30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    const std::array<std::uint32_t, 5> words = {h0, h1, h2, h3, h4};
    Sha1Digest digest = {};
    for (std::size_t index = 0; index < words.size(); ++index) {
        digest[index * 4] = static_cast<std::uint8_t>(words[index] >> 24);
        digest[index * 4 + 1] = static_cast<std::uint8_t>(words[index] >> 16);
        digest[index * 4 + 2] = static_cast<std::uint8_t>(words[index] >> 8);
        digest[index * 4 + 3] = static_cast<std::uint8_t>(words[index]);
    }
    return digest;
}

std::uint64_t DigestModulo(const Sha1Digest& digest, std::uint64_t modulo) noexcept {
    std::uint64_t result = 0;
    for (const std::uint8_t byte : digest) {
        result = AddModulo(MultiplyModulo(result, 256, modulo), byte, modulo);
    }
    return result;
}

} // namespace ryfmach::utils
