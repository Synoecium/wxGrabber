#include "../include/UnitTests.h"
#include "../include/log.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>

std::vector<UnitTests::TestCase> UnitTests::GetTestCases() {
    return {
        // Constant data test
        {
            10,
            "Constant data test",
            [](std::mt19937&, size_t) {
                return std::vector<uint8_t>{
                    0x41, 0x4C, 0x4C, 0x2E, 0x00, 0x00, 0x00, 0x3E, 0x70, 0x72, 0x6F, 0x70, 0x44, 0x41, 0x54, 0x45, 0x00, 0x00, 0x00, 0x10, 0x36, 0x2D, 0x30, 0x35, 0x2D, 0x32, 0x30, 0x30, 0x38, 0x2C, 0x20, 0x31, 0x30, 0x3A, 0x30, 0x32,
                    0x70, 0x72, 0x6F, 0x70, 0x4E, 0x41, 0x4D, 0x45, 0x00, 0x00, 0x00, 0x09, 0x30, 0x30, 0x57, 0x61, 0x72, 0x72, 0x69, 0x6F, 0x72, 0x70, 0x72, 0x6F, 0x70, 0x4F, 0x52, 0x49, 0x47
                };
            }
        },
        // Small random buffer
        {
            64,
            "Small random buffer",
            [](std::mt19937& g, size_t size) {
                std::uniform_int_distribution<> dis(0, 255);
                std::vector<uint8_t> buffer(size);
                std::generate(buffer.begin(), buffer.end(), [&]() { return dis(g); });
                return buffer;
            }
        },
        // Medium random buffer
        {
            1024,
            "Medium random buffer",
            [](std::mt19937& g, size_t size) {
                std::uniform_int_distribution<> dis(0, 255);
                std::vector<uint8_t> buffer(size);
                std::generate(buffer.begin(), buffer.end(), [&]() { return dis(g); });
                return buffer;
            }
        },
        // Buffer with repeating pattern
        {
            1024,
            "Repeating pattern buffer",
            [](std::mt19937& g, size_t size) {
                std::uniform_int_distribution<> dis(0, 255);
                std::vector<uint8_t> pattern;
                // Generate random pattern of 16 bytes
                for (int i = 0; i < 16; i++) {
                    pattern.push_back(dis(g));
                }
                // Repeat pattern to fill buffer
                std::vector<uint8_t> buffer;
                while (buffer.size() < size) {
                    buffer.insert(buffer.end(), pattern.begin(), pattern.end());
                }
                buffer.resize(size);
                return buffer;
            }
        },
        // Buffer with long sequences
        {
            2048,
            "Long sequences buffer",
            [](std::mt19937& g, size_t size) {
                std::uniform_int_distribution<> dis(0, 255);
                std::vector<uint8_t> buffer;
                while (buffer.size() < size) {
                    // Generate random byte and repeat it 20-50 times
                    uint8_t value = dis(g);
                    std::uniform_int_distribution<> len(20, 50);
                    size_t sequence_length = len(g);
                    buffer.insert(buffer.end(), sequence_length, value);
                }
                buffer.resize(size);
                return buffer;
            }
        },
        // Buffer with alternating patterns
        {
            2048,
            "Alternating patterns buffer",
            [](std::mt19937& g, size_t size) {
                std::uniform_int_distribution<> dis(0, 255);
                std::vector<uint8_t> buffer;
                while (buffer.size() < size) {
                    // Random sequence
                    for (int i = 0; i < 32; i++) {
                        buffer.push_back(dis(g));
                    }
                    // Repeating sequence
                    uint8_t value = dis(g);
                    buffer.insert(buffer.end(), 32, value);
                }
                buffer.resize(size);
                return buffer;
            }
        },
        // Very large buffer with mixed content
        {
            1024 * 1024, // 1MB
            "Large mixed content buffer",
            [](std::mt19937& g, size_t size) {
                std::uniform_int_distribution<> dis(0, 255);
                std::vector<uint8_t> buffer;
                buffer.reserve(size);
                
                while (buffer.size() < size) {
                    // Randomly choose between different patterns
                    std::uniform_int_distribution<> patternType(0, 3);
                    switch (patternType(g)) {
                        case 0: {
                            // Random bytes
                            for (int i = 0; i < 256; i++) {
                                buffer.push_back(dis(g));
                            }
                            break;
                        }
                        case 1: {
                            // Repeating sequence
                            uint8_t value = dis(g);
                            std::uniform_int_distribution<> len(50, 200);
                            buffer.insert(buffer.end(), len(g), value);
                            break;
                        }
                        case 2: {
                            // Alternating values
                            uint8_t val1 = dis(g);
                            uint8_t val2 = dis(g);
                            for (int i = 0; i < 100; i++) {
                                buffer.push_back(val1);
                                buffer.push_back(val2);
                            }
                            break;
                        }
                        case 3: {
                            // Incrementing sequence
                            uint8_t start = dis(g);
                            for (int i = 0; i < 128; i++) {
                                buffer.push_back(start + i);
                            }
                            break;
                        }
                    }
                }
                buffer.resize(size);
                return buffer;
            }
        },
        // Very large buffer (10MB) with diverse patterns
        {
            10 * 1024 * 1024, // 10MB
            "Huge mixed content buffer (10MB)",
            [](std::mt19937& g, size_t size) {
                std::uniform_int_distribution<> dis(0, 255);
                std::vector<uint8_t> buffer;
                buffer.reserve(size);
                
                while (buffer.size() < size) {
                    // Randomly choose between different patterns
                    std::uniform_int_distribution<> patternType(0, 5);
                    switch (patternType(g)) {
                        case 0: {
                            // Random bytes
                            for (int i = 0; i < 1024; i++) {
                                buffer.push_back(dis(g));
                            }
                            break;
                        }
                        case 1: {
                            // Long repeating sequence
                            uint8_t value = dis(g);
                            std::uniform_int_distribution<> len(1000, 5000);
                            buffer.insert(buffer.end(), len(g), value);
                            break;
                        }
                        case 2: {
                            // Alternating values with longer sequences
                            uint8_t val1 = dis(g);
                            uint8_t val2 = dis(g);
                            for (int i = 0; i < 500; i++) {
                                buffer.push_back(val1);
                                buffer.push_back(val2);
                            }
                            break;
                        }
                        case 3: {
                            // Long incrementing sequence
                            uint8_t start = dis(g);
                            for (int i = 0; i < 1024; i++) {
                                buffer.push_back(start + (i % 256));
                            }
                            break;
                        }
                        case 4: {
                            // Repeating word pattern
                            std::vector<uint8_t> word;
                            for (int i = 0; i < 8; i++) {
                                word.push_back(dis(g));
                            }
                            for (int i = 0; i < 256; i++) {
                                buffer.insert(buffer.end(), word.begin(), word.end());
                            }
                            break;
                        }
                        case 5: {
                            // Zero-filled blocks
                            std::uniform_int_distribution<> len(500, 2000);
                            buffer.insert(buffer.end(), len(g), 0);
                            break;
                        }
                    }
                }
                buffer.resize(size);
                return buffer;
            }
        }
    };
}

bool UnitTests::LZSSFileDecompressTest(const std::string& compressedFilename, const std::string& etalonFilename) {
    logInfo("LZSS file decompression test");
    // Read the compressed file
    std::vector<uint8_t> compressedData;
    {
        std::ifstream file(compressedFilename, std::ios::binary);
        if (!file) {
            logError("Failed to open compressed file: " + compressedFilename);
            return false;
        }
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        file.seekg(4); // Skip first 4 bytes (magic number)
        compressedData.resize(size - 4);
        file.read(reinterpret_cast<char*>(compressedData.data()), size - 4);
    }

    // Read the etalon file
    std::vector<uint8_t> etalonData;
    {
        std::ifstream file(etalonFilename, std::ios::binary);
        if (!file) {
            logError("Failed to open etalon file: " + etalonFilename);
            return false;
        }
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        etalonData.resize(size);
        file.seekg(0);
        file.read(reinterpret_cast<char*>(etalonData.data()), size);
    }

    // Decompress the compressed data
    std::vector<uint8_t> decompressedData = LZSS::Decompress(compressedData);

    // Compare contents
    for (size_t i = 0; i < decompressedData.size(); i++) {
        if (decompressedData[i] != etalonData[i]) {
            std::stringstream ss;
            ss << "Content mismatch at position " << i << ": compressed=0x" 
               << std::hex << static_cast<int>(decompressedData[i])
               << " etalon=0x" << static_cast<int>(etalonData[i]) << std::dec;
            logError(ss.str());
            return false;
        }
    }
    logInfo("LZSS file decompression test passed");

    return true;
}

bool UnitTests::LZSSFileDecompressTest() {
    logInfo("LZSS file decompression test - searching for .dat files");
    
    // Try to find any .dat file in the current directory
    std::string datFile;
    std::string decompFile;
    
    // Look for common .dat files first
    const std::vector<std::string> commonFiles = {"MapObj.dat", "test.dat", "sample.dat", "data.dat"};
    for (const auto& filename : commonFiles) {
        std::ifstream testFile(filename);
        if (testFile.good()) {
            datFile = filename;
            decompFile = filename.substr(0, filename.find_last_of('.')) + "_decomp.dat";
            testFile.close();
            break;
        }
        testFile.close();
    }
    
    // If no common files found, search for any .dat file
    if (datFile.empty()) {
        // This is a simplified search - in a real implementation you might want to use
        // platform-specific directory listing functions or boost::filesystem
        // For now, we'll just try to find any .dat file by checking a few possibilities
        for (int i = 0; i < 100; ++i) {
            std::string testFile = "file" + std::to_string(i) + ".dat";
            std::ifstream file(testFile);
            if (file.good()) {
                datFile = testFile;
                decompFile = testFile.substr(0, testFile.find_last_of('.')) + "_decomp.dat";
                file.close();
                break;
            }
            file.close();
        }
    }
    
    if (datFile.empty()) {
        logInfo("No .dat files found, skipping LZSS specific dat file decompression test");
        return true; // Skip this test, don't fail
    }
    
    logInfo("Found .dat file: " + datFile + ", looking for decompressed file: " + decompFile);
    
    // Check if decompressed file exists
    std::ifstream decompTestFile(decompFile);
    if (!decompTestFile.good()) {
        logInfo("No corresponding decompressed file found for " + datFile + ", skipping test");
        decompTestFile.close();
        return true; // Skip this test, don't fail
    }
    decompTestFile.close();
    
    // Run the test with the found files
    return LZSSFileDecompressTest(datFile, decompFile);
}

void UnitTests::PrintBuffer(const std::string& label, const std::vector<uint8_t>& buffer) {
    std::stringstream ss;
    ss << label << " data (" << buffer.size() << " bytes):\n";
    for (size_t i = 0; i < buffer.size(); i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') 
           << static_cast<int>(buffer[i]) << " ";
        if ((i + 1) % 16 == 0) ss << "\n";
    }
    ss << std::dec;
    logInfo(ss.str());
}

bool UnitTests::LZSSTests() {
    logInfo("\nRunning LZSS compression/decompression tests...\n");
    bool allTestsPassed = true;
    LZSS lzss;

    // Track total time for all tests
    auto totalStartTime = std::chrono::high_resolution_clock::now();
    double totalCompressTime = 0.0;
    double totalDecompressTime = 0.0;
    int testCount = 0;

    // First run the constant data test
    std::vector<uint8_t> constantData = {
        0x41, 0x4C, 0x4C, 0x2E, 0x00, 0x00, 0x00, 0x3E, 0x70, 0x72
    };

    logInfo("Testing constant data compression/decompression:");
    PrintBuffer("Original", constantData);

    // Compress the data with timing
    auto compressStart = std::chrono::high_resolution_clock::now();
    auto compressedData = lzss.Compress(constantData);
    auto compressEnd = std::chrono::high_resolution_clock::now();
    auto compressTime = std::chrono::duration<double>(compressEnd - compressStart).count();
    totalCompressTime += compressTime;
    PrintBuffer("Compressed", compressedData);

    // Decompress the data with timing
    auto decompressStart = std::chrono::high_resolution_clock::now();
    auto decompressedData = lzss.Decompress(compressedData);
    auto decompressEnd = std::chrono::high_resolution_clock::now();
    auto decompressTime = std::chrono::duration<double>(decompressEnd - decompressStart).count();
    totalDecompressTime += decompressTime;
    PrintBuffer("Decompressed", decompressedData);

    // Calculate throughput
    double compressThroughput = (constantData.size() / 1024.0 / 1024.0) / compressTime;  // MB/s
    double decompressThroughput = (compressedData.size() / 1024.0 / 1024.0) / decompressTime;  // MB/s

    // Compare sizes and compression ratio
    std::stringstream ss;
    ss << "Original size: " << constantData.size() << " bytes\n"
       << "Compressed size: " << compressedData.size() << " bytes\n"
       << "Decompressed size: " << decompressedData.size() << " bytes\n"
       << "Compression ratio: " << static_cast<float>(compressedData.size()) / constantData.size() << "\n"
       << "Compression time: " << std::fixed << std::setprecision(4) << compressTime 
       << "s (Throughput: " << compressThroughput << " MB/s)\n"
       << "Decompression time: " << std::fixed << std::setprecision(4) << decompressTime 
       << "s (Throughput: " << decompressThroughput << " MB/s)";
    logInfo(ss.str());

    testCount++;

    // Check if decompressed matches original
    if (!CompareBuffers(constantData, decompressedData)) {
        logError("Constant data test FAILED - decompressed data doesn't match original");
        allTestsPassed = false;
    } else {
        logInfo("Constant data test passed");
    }

    // Run all test cases
    std::mt19937 rng(42); // Fixed seed for reproducibility
    auto testCases = GetTestCases();

    for (const auto& testCase : testCases) {
        logInfo("\nTesting " + testCase.description + ":");
        
        // Generate test data
        auto originalData = testCase.generator(rng, testCase.size);
        logInfo("Generated " + std::to_string(originalData.size()) + " bytes of test data");

        // Compress with timing
        auto compressStart = std::chrono::high_resolution_clock::now();
        auto compressedData = lzss.Compress(originalData);
        auto compressEnd = std::chrono::high_resolution_clock::now();
        auto compressTime = std::chrono::duration<double>(compressEnd - compressStart).count();
        totalCompressTime += compressTime;
        logInfo("Compressed to " + std::to_string(compressedData.size()) + " bytes");

        // Decompress with timing
        auto decompressStart = std::chrono::high_resolution_clock::now();
        auto decompressedData = lzss.Decompress(compressedData);
        auto decompressEnd = std::chrono::high_resolution_clock::now();
        auto decompressTime = std::chrono::duration<double>(decompressEnd - decompressStart).count();
        totalDecompressTime += decompressTime;
        logInfo("Decompressed to " + std::to_string(decompressedData.size()) + " bytes");

        // Calculate throughput
        double compressThroughput = (originalData.size() / 1024.0 / 1024.0) / compressTime;  // MB/s
        double decompressThroughput = (compressedData.size() / 1024.0 / 1024.0) / decompressTime;  // MB/s

        // Compare sizes and compression ratio
        float ratio = static_cast<float>(compressedData.size()) / originalData.size();
        std::stringstream ss;
        ss << "Compression ratio: " << ratio << "\n"
           << "Compression time: " << std::fixed << std::setprecision(4) << compressTime 
           << "s (Throughput: " << compressThroughput << " MB/s)\n"
           << "Decompression time: " << std::fixed << std::setprecision(4) << decompressTime 
           << "s (Throughput: " << decompressThroughput << " MB/s)";
        logInfo(ss.str());

        testCount++;

        // Verify decompressed data matches original
        if (!CompareBuffers(originalData, decompressedData)) {
            logError("Test FAILED - decompressed data doesn't match original");
            
            // Find first mismatch
            for (size_t i = 0; i < std::min(originalData.size(), decompressedData.size()); i++) {
                if (originalData[i] != decompressedData[i]) {
                    std::stringstream ss;
                    ss << "First mismatch at position " << i 
                       << ": original=0x" << std::hex << static_cast<int>(originalData[i])
                       << ", decompressed=0x" << static_cast<int>(decompressedData[i]) 
                       << std::dec;
                    logError(ss.str());
                    break;
                }
            }
            
            allTestsPassed = false;
        } else {
            logInfo("Test passed");
        }
    }

    auto totalEndTime = std::chrono::high_resolution_clock::now();
    auto totalTime = std::chrono::duration<double>(totalEndTime - totalStartTime).count();

    std::stringstream summary;
    summary << "\nLZSS Test Summary:\n"
           << "Total tests run: " << testCount << "\n"
           << "Total time: " << std::fixed << std::setprecision(4) << totalTime << "s\n"
           << "Total compression time: " << totalCompressTime << "s\n"
           << "Total decompression time: " << totalDecompressTime << "s\n"
           << "Average compression time: " << (totalCompressTime / testCount) << "s\n"
           << "Average decompression time: " << (totalDecompressTime / testCount) << "s";
    logInfo(summary.str());

    if (allTestsPassed) {
        logInfo("\nAll LZSS tests PASSED!");
    } else {
        logError("\nSome LZSS tests FAILED!");
    }

    return allTestsPassed;
}

bool UnitTests::CompareBuffers(const std::vector<uint8_t>& original, const std::vector<uint8_t>& decompressed) {
    if (original.size() != decompressed.size()) {
        std::stringstream ss;
        ss << "Size mismatch: original=" << original.size() 
           << ", decompressed=" << decompressed.size();
        logError(ss.str());
        return false;
    }

    for (size_t i = 0; i < original.size(); i++) {
        if (original[i] != decompressed[i]) {
            return false;
        }
    }

    return true;
}
