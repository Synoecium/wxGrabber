#ifndef LZSS_H
#define LZSS_H

#include <vector>
#include <memory>
#include <iostream>

class LZSS {
public:
    // Constants for LZSS algorithm
    static constexpr int32_t RING_BUFFER_SIZE = 0x1000;  // N: size of ring buffer
    static constexpr int32_t MIN_MATCH_LENGTH = 3;      // Minimum match length
    static constexpr int32_t MATCH_LENGTH_LIMIT = 0xF + MIN_MATCH_LENGTH;  // 0xF + N: upper limit for match length
    static constexpr int32_t INITIAL_POSITION = RING_BUFFER_SIZE - MATCH_LENGTH_LIMIT;
    static constexpr uint8_t INITIAL_VALUE = 0;
    static constexpr uint8_t FLAG_BYTE_BITS = 8;

    LZSS() = default;
    ~LZSS() = default;

    // Main compression and decompression functions
    static std::vector<uint8_t> Compress(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> Decompress(const std::vector<uint8_t>& inputBuffer);

private:

    // Helper functions for compression
    static int32_t CalculateHash(const std::vector<uint8_t>& data, int32_t pos);
};

#endif // LZSS_H