#ifndef UNIT_TESTS_H
#define UNIT_TESTS_H

#include <string>
#include <vector>
#include <random>
#include <functional>
#include "lzss.h"

class UnitTests {
public:
    static bool LZSSFileDecompressTest(const std::string& compressedFilename, const std::string& etalonFilename);
    static bool LZSSFileDecompressTest();
    static bool LZSSTests();

private:
    struct TestCase {
        size_t size;
        std::string description;
        std::function<std::vector<uint8_t>(std::mt19937&, size_t)> generator;
    };

    static std::vector<TestCase> GetTestCases();
    static void PrintBuffer(const std::string& label, const std::vector<uint8_t>& buffer);
    static bool CompareBuffers(const std::vector<uint8_t>& original, const std::vector<uint8_t>& decompressed);
};

#endif // UNIT_TESTS_H 