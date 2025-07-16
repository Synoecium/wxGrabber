#include "../include/BitmapData.h"
#include "../include/log.h"
#include <cstdint>
#include <wx/image.h>
#include <wx/mstream.h>
#include <wx/colour.h>
#include <fstream>
#include <cstring>
#include <array>
#include <unordered_set>
#include <wx/string.h>
#include <vector>
#include <queue>
#include <map> // For visited tracking, avoids complex 2D array management
#include <algorithm> // For std::min/max
#include <wx/gdicmn.h> // For wxRect
#include <wx/bitmap.h> // For wxBitmap
#include <wx/dcmemory.h> // For wxMemoryDC
#include <wx/font.h> // For wxFont
#include "../include/CommonTypes.h"

// Define the static member
std::vector<uint8_t> BitmapData::allegro_palette;

// Initialize the palette
static void init_allegro_palette() {
    // Allegro 4 palette - Mode 13h VGA palette
    uint32_t palette[256];
    palette[  0] = rgb(255,  0,255);    // Transparent
    palette[  1] = rgb(  0,  0,170);
    palette[  2] = rgb(  0,170,  0);
    palette[  3] = rgb(  0,170,170);
    palette[  4] = rgb(170,  0,  0);
    palette[  5] = rgb(170,  0,170);
    palette[  6] = rgb(170, 85,  0);
    palette[  7] = rgb(170,170,170);
    palette[  8] = rgb( 85, 85, 85);
    palette[  9] = rgb( 85, 85,255);
    palette[ 10] = rgb( 85,255, 85);
    palette[ 11] = rgb( 85,255,255);
    palette[ 12] = rgb(255, 85, 85);
    palette[ 13] = rgb(255, 85,255);
    palette[ 14] = rgb(255,255, 85);
    palette[ 15] = rgb(255,255,255);
    palette[ 16] = rgb(  0,  0,  0);
    palette[ 17] = rgb( 20, 20, 20);
    palette[ 18] = rgb( 32, 32, 32);
    palette[ 19] = rgb( 44, 44, 44);
    palette[ 20] = rgb( 56, 56, 56);
    palette[ 21] = rgb( 68, 68, 68);
    palette[ 22] = rgb( 80, 80, 80);
    palette[ 23] = rgb( 97, 97, 97);
    palette[ 24] = rgb(113,113,113);
    palette[ 25] = rgb(129,129,129);
    palette[ 26] = rgb(145,145,145);
    palette[ 27] = rgb(161,161,161);
    palette[ 28] = rgb(182,182,182);
    palette[ 29] = rgb(202,202,202);
    palette[ 30] = rgb(226,226,226);
    palette[ 31] = rgb(255,255,255);
    palette[ 32] = rgb(  0,  0,255);
    palette[ 33] = rgb( 64,  0,255);
    palette[ 34] = rgb(125,  0,255);
    palette[ 35] = rgb(190,  0,255);
    palette[ 36] = rgb(255,  0,255);
    palette[ 37] = rgb(255,  0,190);
    palette[ 38] = rgb(255,  0,125);
    palette[ 39] = rgb(255,  0, 64);
    palette[ 40] = rgb(255,  0,  0);
    palette[ 41] = rgb(255, 64,  0);
    palette[ 42] = rgb(255,125,  0);
    palette[ 43] = rgb(255,190,  0);
    palette[ 44] = rgb(255,255,  0);
    palette[ 45] = rgb(190,255,  0);
    palette[ 46] = rgb(125,255,  0);
    palette[ 47] = rgb( 64,255,  0);
    palette[ 48] = rgb(  0,255,  0);
    palette[ 49] = rgb(  0,255, 64);
    palette[ 50] = rgb(  0,255,125);
    palette[ 51] = rgb(  0,255,190);
    palette[ 52] = rgb(  0,255,255);
    palette[ 53] = rgb(  0,190,255);
    palette[ 54] = rgb(  0,125,255);
    palette[ 55] = rgb(  0, 64,255);
    palette[ 56] = rgb(125,125,255);
    palette[ 57] = rgb(157,125,255);
    palette[ 58] = rgb(190,125,255);
    palette[ 59] = rgb(222,125,255);
    palette[ 60] = rgb(255,125,255);
    palette[ 61] = rgb(255,125,222);
    palette[ 62] = rgb(255,125,190);
    palette[ 63] = rgb(255,125,157);
    palette[ 64] = rgb(255,125,125);
    palette[ 65] = rgb(255,157,125);
    palette[ 66] = rgb(255,190,125);
    palette[ 67] = rgb(255,222,125);
    palette[ 68] = rgb(255,255,125);
    palette[ 69] = rgb(222,255,125);
    palette[ 70] = rgb(190,255,125);
    palette[ 71] = rgb(157,255,125);
    palette[ 72] = rgb(125,255,125);
    palette[ 73] = rgb(125,255,157);
    palette[ 74] = rgb(125,255,190);
    palette[ 75] = rgb(125,255,222);
    palette[ 76] = rgb(125,255,255);
    palette[ 77] = rgb(125,222,255);
    palette[ 78] = rgb(125,190,255);
    palette[ 79] = rgb(125,157,255);
    palette[ 80] = rgb(182,182,255);
    palette[ 81] = rgb(198,182,255);
    palette[ 82] = rgb(218,182,255);
    palette[ 83] = rgb(234,182,255);
    palette[ 84] = rgb(255,182,255);
    palette[ 85] = rgb(255,182,234);
    palette[ 86] = rgb(255,182,218);
    palette[ 87] = rgb(255,182,198);
    palette[ 88] = rgb(255,182,182);
    palette[ 89] = rgb(255,198,182);
    palette[ 90] = rgb(255,218,182);
    palette[ 91] = rgb(255,234,182);
    palette[ 92] = rgb(255,255,182);
    palette[ 93] = rgb(234,255,182);
    palette[ 94] = rgb(218,255,182);
    palette[ 95] = rgb(198,255,182);
    palette[ 96] = rgb(182,255,182);
    palette[ 97] = rgb(182,255,198);
    palette[ 98] = rgb(182,255,218);
    palette[ 99] = rgb(182,255,234);
    palette[100] = rgb(182,255,255);
    palette[101] = rgb(182,234,255);
    palette[102] = rgb(182,218,255);
    palette[103] = rgb(182,198,255);
    palette[104] = rgb(  0,  0,113);
    palette[105] = rgb( 28,  0,113);
    palette[106] = rgb( 56,  0,113);
    palette[107] = rgb( 85,  0,113);
    palette[108] = rgb(113,  0,113);
    palette[109] = rgb(113,  0, 85);
    palette[110] = rgb(113,  0, 56);
    palette[111] = rgb(113,  0, 28);
    palette[112] = rgb(113,  0,  0);
    palette[113] = rgb(113, 28,  0);
    palette[114] = rgb(113, 56,  0);
    palette[115] = rgb(113, 85,  0);
    palette[116] = rgb(113,113,  0);
    palette[117] = rgb( 85,113,  0);
    palette[118] = rgb( 56,113,  0);
    palette[119] = rgb( 28,113,  0);
    palette[120] = rgb(  0,113,  0);
    palette[121] = rgb(  0,113, 28);
    palette[122] = rgb(  0,113, 56);
    palette[123] = rgb(  0,113, 85);
    palette[124] = rgb(  0,113,113);
    palette[125] = rgb(  0, 85,113);
    palette[126] = rgb(  0, 56,113);
    palette[127] = rgb(  0, 28,113);
    palette[128] = rgb( 56, 56,113);
    palette[129] = rgb( 68, 56,113);
    palette[130] = rgb( 85, 56,113);
    palette[131] = rgb( 97, 56,113);
    palette[132] = rgb(113, 56,113);
    palette[133] = rgb(113, 56, 97);
    palette[134] = rgb(113, 56, 85);
    palette[135] = rgb(113, 56, 68);
    palette[136] = rgb(113, 56, 56);
    palette[137] = rgb(113, 68, 56);
    palette[138] = rgb(113, 85, 56);
    palette[139] = rgb(113, 97, 56);
    palette[140] = rgb(113,113, 56);
    palette[141] = rgb( 97,113, 56);
    palette[142] = rgb( 85,113, 56);
    palette[143] = rgb( 68,113, 56);
    palette[144] = rgb( 56,113, 56);
    palette[145] = rgb( 56,113, 68);
    palette[146] = rgb( 56,113, 85);
    palette[147] = rgb( 56,113, 97);
    palette[148] = rgb( 56,113,113);
    palette[149] = rgb( 56, 97,113);
    palette[150] = rgb( 56, 85,113);
    palette[151] = rgb( 56, 68,113);
    palette[152] = rgb( 80, 80,113);
    palette[153] = rgb( 89, 80,113);
    palette[154] = rgb( 97, 80,113);
    palette[155] = rgb(105, 80,113);
    palette[156] = rgb(113, 80,113);
    palette[157] = rgb(113, 80,105);
    palette[158] = rgb(113, 80, 97);
    palette[159] = rgb(113, 80, 89);
    palette[160] = rgb(113, 80, 80);
    palette[161] = rgb(113, 89, 80);
    palette[162] = rgb(113, 97, 80);
    palette[163] = rgb(113,105, 80);
    palette[164] = rgb(113,113, 80);
    palette[165] = rgb(105,113, 80);
    palette[166] = rgb( 97,113, 80);
    palette[167] = rgb( 89,113, 80);
    palette[168] = rgb( 80,113, 80);
    palette[169] = rgb( 80,113, 89);
    palette[170] = rgb( 80,113, 97);
    palette[171] = rgb( 80,113,105);
    palette[172] = rgb( 80,113,113);
    palette[173] = rgb( 80,105,113);
    palette[174] = rgb( 80, 97,113);
    palette[175] = rgb( 80, 89,113);
    palette[176] = rgb(  0,  0, 64);
    palette[177] = rgb( 16,  0, 64);
    palette[178] = rgb( 32,  0, 64);
    palette[179] = rgb( 48,  0, 64);
    palette[180] = rgb( 64,  0, 64);
    palette[181] = rgb( 64,  0, 48);
    palette[182] = rgb( 64,  0, 32);
    palette[183] = rgb( 64,  0, 16);
    palette[184] = rgb( 64,  0,  0);
    palette[185] = rgb( 64, 16,  0);
    palette[186] = rgb( 64, 32,  0);
    palette[187] = rgb( 64, 48,  0);
    palette[188] = rgb( 64, 64,  0);
    palette[189] = rgb( 48, 64,  0);
    palette[190] = rgb( 32, 64,  0);
    palette[191] = rgb( 16, 64,  0);
    palette[192] = rgb(  0, 64,  0);
    palette[193] = rgb(  0, 64, 16);
    palette[194] = rgb(  0, 64, 32);
    palette[195] = rgb(  0, 64, 48);
    palette[196] = rgb(  0, 64, 64);
    palette[197] = rgb(  0, 48, 64);
    palette[198] = rgb(  0, 32, 64);
    palette[199] = rgb(  0, 16, 64);
    palette[200] = rgb( 32, 32, 64);
    palette[201] = rgb( 40, 32, 64);
    palette[202] = rgb( 48, 32, 64);
    palette[203] = rgb( 56, 32, 64);
    palette[204] = rgb( 64, 32, 64);
    palette[205] = rgb( 64, 32, 56);
    palette[206] = rgb( 64, 32, 48);
    palette[207] = rgb( 64, 32, 40);
    palette[208] = rgb( 64, 32, 32);
    palette[209] = rgb( 64, 40, 32);
    palette[210] = rgb( 64, 48, 32);
    palette[211] = rgb( 64, 56, 32);
    palette[212] = rgb( 64, 64, 32);
    palette[213] = rgb( 56, 64, 32);
    palette[214] = rgb( 48, 64, 32);
    palette[215] = rgb( 40, 64, 32);
    palette[216] = rgb( 32, 64, 32);
    palette[217] = rgb( 32, 64, 40);
    palette[218] = rgb( 32, 64, 48);
    palette[219] = rgb( 32, 64, 56);
    palette[220] = rgb( 32, 64, 64);
    palette[221] = rgb( 32, 56, 64);
    palette[222] = rgb( 32, 48, 64);
    palette[223] = rgb( 32, 40, 64);
    palette[224] = rgb( 44, 44, 64);
    palette[225] = rgb( 48, 44, 64);
    palette[226] = rgb( 52, 44, 64);
    palette[227] = rgb( 60, 44, 64);
    palette[228] = rgb( 64, 44, 64);
    palette[229] = rgb( 64, 44, 60);
    palette[230] = rgb( 64, 44, 52);
    palette[231] = rgb( 64, 44, 48);
    palette[232] = rgb( 64, 44, 44);
    palette[233] = rgb( 64, 48, 44);
    palette[234] = rgb( 64, 52, 44);
    palette[235] = rgb( 64, 60, 44);
    palette[236] = rgb( 64, 64, 44);
    palette[237] = rgb( 60, 64, 44);
    palette[238] = rgb( 52, 64, 44);
    palette[239] = rgb( 48, 64, 44);
    palette[240] = rgb( 44, 64, 44);
    palette[241] = rgb( 44, 64, 48);
    palette[242] = rgb( 44, 64, 52);
    palette[243] = rgb( 44, 64, 60);
    palette[244] = rgb( 44, 64, 64);
    palette[245] = rgb( 44, 60, 64);
    palette[246] = rgb( 44, 52, 64);
    palette[247] = rgb( 44, 48, 64);
    palette[248] = rgb(  0,  0,  0);
    palette[249] = rgb(  0,  0,  0);
    palette[250] = rgb(  0,  0,  0);
    palette[251] = rgb(  0,  0,  0);
    palette[252] = rgb(  0,  0,  0);
    palette[253] = rgb(  0,  0,  0);
    palette[254] = rgb(  0,  0,  0);
    palette[255] = rgb(  255,  255,  255);

    BitmapData::allegro_palette.resize(256*3);
    for (int i = 0; i < 256; i++) {
        BitmapData::allegro_palette[i*3] = (palette[i] >> 16) & 0xFF;
        BitmapData::allegro_palette[i*3+1] = (palette[i] >> 8) & 0xFF;
        BitmapData::allegro_palette[i*3+2] = palette[i] & 0xFF;
    }
}

// Call the initialization function at startup
static const bool palette_initialized = []() {
    init_allegro_palette();
    return true;
}();

bool BitmapData::isValidFormat() const {
    return bits == 8 || bits == 15 || bits == 16 || 
           bits == 24 || bits == 32 || bits == -32;
}

int BitmapData::getBytesPerPixel() const {
    if (bits == 8) return 1;
    if (bits == 15 || bits == 16) return 2;
    if (bits == 24) return 3;
    if (bits == 32) return 3;   // 32-bit RGBA is stored as 24-bit RGB
    if (bits == -32) return 4;  // true 32-bit RGBA
    return 0;
}

bool BitmapData::parse(const std::vector<uint8_t>& buffer, ObjectType typeID, BitmapData& outBitmap) {
    // Check if it's palette data
    if (typeID == ObjectType::DAT_PALETTE) {
        // Expected size: 256 entries * 4 bytes/entry (R, G, B, Pad)
        if (buffer.size() != 1024) {
            logError("Palette data size mismatch. Expected 1024, got " + std::to_string(buffer.size()));
            return false;
        }

        outBitmap.width = 16;
        outBitmap.height = 16;
        outBitmap.bits = 24; // Store as 24-bit RGB
        outBitmap.data.resize(16 * 16 * 3);

        for (int i = 0; i < 256; ++i) {
            int x = i % 16;
            int y = i / 16;
            size_t paletteOffset = i * 4; // 4 bytes per palette entry (R, G, B, Pad)
            size_t bmpOffset = (y * 16 + x) * 3; // 3 bytes per pixel in output bitmap

            // Read R, G, B (0-63) and scale to 0-255
            uint8_t r6bit = buffer[paletteOffset + 0];
            uint8_t g6bit = buffer[paletteOffset + 1];
            uint8_t b6bit = buffer[paletteOffset + 2];
            // uint8_t pad = buffer[paletteOffset + 3]; // Padding byte is ignored

            // Scale 6-bit (0-63) to 8-bit (0-255). Multiply by 4 is a common approximation.
            outBitmap.data[bmpOffset + 0] = r6bit * 4; // R
            outBitmap.data[bmpOffset + 1] = g6bit * 4; // G
            outBitmap.data[bmpOffset + 2] = b6bit * 4; // B
        }
        outBitmap.typeID = typeID; // Store the type ID
        return true; // Successfully parsed palette data
    }

    // Check if it's RLE sprite data
    if (typeID == ObjectType::DAT_RLE_SPRITE) {
        logDebug("Starting RLE sprite parsing");
        // Check minimum size for header (10 bytes: 2+2+2+4)
        if (buffer.size() < 10) {
            logError("RLE sprite data too small for header. Size: " + std::to_string(buffer.size()));
            return false;
        }

        // Parse header (big endian)
        outBitmap.bits = (buffer[0] << 8) | buffer[1];
        outBitmap.width = (buffer[2] << 8) | buffer[3];
        outBitmap.height = (buffer[4] << 8) | buffer[5];
        if (outBitmap.bits & 0x8000) outBitmap.bits |= ~0xFFFF;  // Sign extend if negative
        uint32_t dataSize = (buffer[6] << 24) | (buffer[7] << 16) | (buffer[8] << 8) | buffer[9];
        logDebug("RLE sprite header - Bits: " + std::to_string(outBitmap.bits) + 
                ", Width: " + std::to_string(outBitmap.width) + 
                ", Height: " + std::to_string(outBitmap.height) + 
                ", Data size: " + std::to_string(dataSize));

        // Validate format and dimensions
        if (!outBitmap.isValidFormat() || outBitmap.width <= 0 || outBitmap.height <= 0) {
            logError("Invalid RLE sprite format or dimensions - Bits: " + std::to_string(outBitmap.bits) + 
                    ", Width: " + std::to_string(outBitmap.width) + 
                    ", Height: " + std::to_string(outBitmap.height));
            return false;
        }

        // Calculate bytes per pixel
        int bytesPerPixel = outBitmap.getBytesPerPixel();
        if (bytesPerPixel == 0) {
            logError("Invalid bytes per pixel for RLE sprite: " + std::to_string(bytesPerPixel));
            return false;
        }
        logDebug("Bytes per pixel: " + std::to_string(bytesPerPixel));

        // Allocate output buffer and fill with zeros
        size_t expectedDataSize = outBitmap.width * outBitmap.height * bytesPerPixel;
        logDebug("Allocating output buffer of size: " + std::to_string(expectedDataSize));
        outBitmap.data.resize(expectedDataSize, 0);
        outBitmap.alpha.resize(outBitmap.width * outBitmap.height, 255);
        size_t outPos = 0;
        size_t inPos = 10; // Start after header
        size_t currentRow = 0;
        size_t rowStartPos = 0;

        // Process RLE data
        logDebug("Starting RLE data processing");
        while (inPos < buffer.size() && outPos < outBitmap.data.size()) {
            // Read the count
            int32_t count;
            if (outBitmap.bits == 8) {
                if (inPos + 1 > buffer.size()) {
                    logError("Buffer underflow while reading 8-bit count at position " + std::to_string(inPos));
                    break;
                }
                int8_t count8 = buffer[inPos];
                count = static_cast<int32_t>(count8);
                inPos += 1;
                logDebug("Read 8-bit count: " + std::to_string(count) + " at position " + std::to_string(inPos - 1));
            }
            else if (outBitmap.bits == 15 || outBitmap.bits == 16) {
                if (inPos + 2 > buffer.size()) {
                    logError("Buffer underflow while reading 16-bit count at position " + std::to_string(inPos));
                    break;
                }
                int16_t count16 = buffer[inPos] | (buffer[inPos + 1] << 8);
                count = static_cast<int32_t>(count16);
                inPos += 2;
                logDebug("Read 16-bit count: " + std::to_string(count) + " at position " + std::to_string(inPos - 2));
            } else {
                if (inPos + 4 > buffer.size()) {
                    logError("Buffer underflow while reading 32-bit count at position " + std::to_string(inPos));
                    break;
                }
                count = buffer[inPos] | (buffer[inPos + 1] << 8) | 
                       (buffer[inPos + 2] << 16) | (buffer[inPos + 3] << 24);
                inPos += 4;
                logDebug("Read 32-bit count: " + std::to_string(count) + " at position " + std::to_string(inPos - 4));
            }

            if (outBitmap.bits == 8 && count == RLE_EOL_MARKER_8) {
                logDebug("Found 8-bit EOL marker");
                continue;
            }
            if ((outBitmap.bits == 15 || outBitmap.bits == 16) && count == RLE_EOL_MARKER_16) {
                logDebug("Found 16-bit EOL marker");
                continue;
            }
            if ((outBitmap.bits == 24 || outBitmap.bits == 32 || outBitmap.bits == -32) && count == RLE_EOL_MARKER_32) {
                logDebug("Found 32-bit EOL marker");
                continue;
            }

            if (count < 0) {
                // negative count means zero run
                count = -count;
                logDebug("Processing zero run of length: " + std::to_string(count));
                // fill the alpha channel with 0
                if (outPos/bytesPerPixel + count > outBitmap.alpha.size()) {
                    logError("Alpha channel size mismatch - Position: " + std::to_string(outPos/bytesPerPixel) + 
                            ", Count: " + std::to_string(count) + 
                            ", Alpha size: " + std::to_string(outBitmap.alpha.size()));
                    return false;
                }
                std::fill(outBitmap.alpha.begin() + outPos/bytesPerPixel, 
                         outBitmap.alpha.begin() + outPos/bytesPerPixel + count, 0);
                // skip the zero run
                outPos += count * bytesPerPixel;
            }
            else {
                // positive count means run
                int bytesToCopy = count * bytesPerPixel;
                logDebug("Processing data run of length: " + std::to_string(count) + 
                       " (" + std::to_string(bytesToCopy) + " bytes)");
                // check if we have enough data
                if (inPos + bytesToCopy > buffer.size()) {
                    logError("Not enough data to copy - Need: " + std::to_string(bytesToCopy) + 
                            ", Available: " + std::to_string(buffer.size() - inPos));
                    break;
                }
                // copy the data
                std::copy(buffer.begin() + inPos, buffer.begin() + inPos + bytesToCopy, 
                         outBitmap.data.begin() + outPos);
                if (outBitmap.bits == -32)  // copy alpha cahnnel from positive run
                {
                    for (int i = 0; i < bytesToCopy; i+=4) {
                        outBitmap.alpha[(outPos+i)/4] = buffer[inPos+i+3]; // Copy alpha channel directly
                    }
                }         
                inPos += bytesToCopy;
                outPos += bytesToCopy;
            }
        }

        logDebug("RLE data processing complete - Processed " + std::to_string(outPos) + 
                " bytes of output data from " + std::to_string(inPos) + " bytes of input data");
        outBitmap.typeID = typeID; // Store the type ID
        return true;
    }

    // --- Original Bitmap Parsing Logic ---
    // Check minimum size for header
    if (buffer.size() < 6) {
        logWarning("Invalid bitmap header - less than 6 bytes");
        return false;
    }

    // Parse header (big endian format)
    outBitmap.bits = (buffer[0] << 8) | buffer[1];  // Convert from big endian
    outBitmap.width = (buffer[2] << 8) | buffer[3];
    outBitmap.height = (buffer[4] << 8) | buffer[5];
    if (outBitmap.bits & 0x8000) outBitmap.bits |= ~0xFFFF;  // Sign extend if negative
    
    // Validate format and dimensions
    if (!outBitmap.isValidFormat() || outBitmap.width <= 0 || outBitmap.height <= 0) {
        logWarning("Invalid bitmap format or dimensions - Bits: " + std::to_string(outBitmap.bits) + 
                  ", Width: " + std::to_string(outBitmap.width) + 
                  ", Height: " + std::to_string(outBitmap.height) + 
                  ", Valid formats: 8, 15, 16, 24, 32, -32 bits" +
                  ", Data size: " + std::to_string(buffer.size()) + 
                  ", Expected header size: 6 bytes" +
                  ", Expected data size: " + std::to_string(outBitmap.width * outBitmap.height * outBitmap.getBytesPerPixel()) + " bytes");
        return false;
    }

    // Validate data size matches expected size
    size_t expectedSize = outBitmap.width * outBitmap.height * outBitmap.getBytesPerPixel();
    if (buffer.size() - 6 != expectedSize) {
        logError("Bitmap data size mismatch. Expected " + std::to_string(expectedSize) + ", got " + std::to_string(buffer.size() - 6));
        return false;
    }
    
    // Copy image data
    outBitmap.data.assign(buffer.begin() + 6, buffer.end());
    outBitmap.typeID = typeID; // Store the type ID
    outBitmap.updateAlphaChannel();

    return true;
}

void BitmapData::updateAlphaChannel() {
    // Process alpha color for compiled sprites excluding 8 bit
    // for 8-bit sprites, the alpha channel is generated when toWxImage is called
    if ((typeID == ObjectType::DAT_C_SPRITE || typeID == ObjectType::DAT_XC_SPRITE) && bits != 8) {
        logDebug("Processing alpha color for sprite, bits: " + std::to_string(bits) + " size of alpha: " + std::to_string(width * height));
        alpha.resize(width * height, 255);
        if (bits == 15 || bits == 16) {
            for (int i = 0; i < data.size(); i+=2) {
                if (compareRGBwithColor(data.data() + i, RLE_ZERO_COLOR_16, bits)) {
                    alpha[i/2] = 0;
                }
            }
        }
        else if (bits == 24 || bits == 32) {
            for (int i = 0; i < data.size(); i+=3) {
                if (compareRGBwithColor(data.data() + i, RLE_ZERO_COLOR_32)) {
                    alpha[i/3] = 0;
                }
            }
        }
        else if (bits == -32) {
            // For true 32-bit RGBA, use the alpha channel directly
            for (int i = 0; i < data.size(); i+=4) {
                alpha[i/4] = data[i+3]; // Copy alpha channel directly
            }
        }
    }
    if ((typeID == ObjectType::DAT_BITMAP || typeID == ObjectType::DAT_RLE_SPRITE) && bits == -32)
    {
        logDebug("Processing alpha color for sprite, bits: " + std::to_string(bits) + " size of alpha: " + std::to_string(width * height));
        alpha.resize(width * height, 255);
        // For true 32-bit RGBA, use the alpha channel directly
        for (int i = 0; i < data.size(); i+=4) {
            alpha[i/4] = data[i+3]; // Copy alpha channel directly
        }
    }
    // process RLE sprite with zero color
    if (typeID == ObjectType::DAT_RLE_SPRITE) {
        int32_t zeroColor = bits == 8 ? RLE_ZERO_COLOR_8 : (bits == 15 || bits == 16 ? RLE_ZERO_COLOR_16 : RLE_ZERO_COLOR_32);
        if (bits != 8) {    // for 8-bit RLE sprites, the alpha channel is generated with loadFromWxImage or parsed
            int bytesPerPixel = getBytesPerPixel();
            if (alpha.size() != data.size()/bytesPerPixel) {
                alpha.resize(data.size()/bytesPerPixel, 255);
            }
            for (int i = 0; i < data.size(); i+=bytesPerPixel) {
                if (compareRGBwithColor(data.data() + i, zeroColor, bits)) {
                    alpha[i/bytesPerPixel] = 0;
                }
            }
        }
    }
}

std::vector<uint8_t> BitmapData::serialize(std::vector<uint8_t>& palette) const {
    // Validate format and dimensions
    if (!isValidFormat() || width <= 0 || height <= 0) {
        return std::vector<uint8_t>();
    }

    if (typeID == ObjectType::DAT_PALETTE) {
        // Expected size: 256 entries * 4 bytes/entry (R, G, B, Pad)
        std::vector<uint8_t> buffer(1024);
        for (int i = 0; i < 256; i++) {
            buffer[i*4] = data[i*3]/4;
            buffer[i*4+1] = data[i*3+1]/4;
            buffer[i*4+2] = data[i*3+2]/4;
            buffer[i*4+3] = 0;
        }
        return buffer;
    }

    if (typeID == ObjectType::DAT_RLE_SPRITE) {
        // Calculate bytes per pixel
        int bytesPerPixel = getBytesPerPixel();
        if (bytesPerPixel == 0) {
            logError("RLE sprite serialization failed: Invalid bytes per pixel");
            return std::vector<uint8_t>();
        }

        logInfo("Starting RLE sprite serialization: " + std::to_string(width) + "x" + std::to_string(height) + 
                " (" + std::to_string(bits) + " bit)");

        // Create buffer for RLE data
        std::vector<uint8_t> buffer;
        buffer.reserve(10 + data.size() * 2); // Reserve space for header and worst-case RLE data
        logInfo("Reserved buffer space: " + std::to_string(10 + data.size() * 2) + " bytes");

        // Write header (big endian)
        buffer.push_back(static_cast<uint8_t>((bits >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>(bits & 0xFF));
        buffer.push_back(static_cast<uint8_t>((width >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>(width & 0xFF));
        buffer.push_back(static_cast<uint8_t>((height >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>(height & 0xFF));
        logInfo("Written header - Bits: " + std::to_string(bits) + 
                ", Width: " + std::to_string(width) + 
                ", Height: " + std::to_string(height));

        // Reserve space for data size (will be filled later)
        size_t dataSizePos = buffer.size();
        buffer.resize(buffer.size() + 4);
        uint32_t nonZeroDataSize = 0;
        logInfo("Reserved space for data size at position: " + std::to_string(dataSizePos));

        // Process each row
        logInfo("Starting row processing");
        //TODO: check 8 bit palette indexing
        int32_t zeroColor = bits == 8 ? RLE_ZERO_COLOR_8 : (bits == 15 || bits == 16 ? RLE_ZERO_COLOR_16 : RLE_ZERO_COLOR_32);
        for (int y = 0; y < height; y++) {
            size_t rowStart = y * width * bytesPerPixel;
            size_t rowEnd = rowStart + width * bytesPerPixel;
            size_t pos = rowStart;
            int32_t zeroRunLength = 0;
            size_t runStart = pos;
            int32_t runLength = 0;
            logDebug("Processing row " + std::to_string(y) + 
                    " (start: " + std::to_string(rowStart) + 
                    ", end: " + std::to_string(rowEnd) + ")");

            while (pos <= rowEnd) {
                bool bAnyRunEnded = false;
                bool bLineEnded = false;
                bool bZeroPixel = false;
                if (pos >= rowEnd) {    // end of line
                    bAnyRunEnded = true;
                    bLineEnded = true;
                    logDebug("Reached end of line at position " + std::to_string(pos));
                }
                if (!bLineEnded) {
                    if (bits == 8) {
                        int32_t paletteIndex = data[pos];
                        int32_t pixelColor = palette[paletteIndex*3] | 
                                     palette[paletteIndex*3+1] << 8 | 
                                     palette[paletteIndex*3+2] << 16;
                        bZeroPixel = pixelColor == zeroColor;
                    }
                    else {
                        bZeroPixel = compareRGBwithColor(data.data() + pos, zeroColor, bits);
                    }
                }
                if (runLength > 0 && bZeroPixel) {
                    bAnyRunEnded = true;
                    logDebug("Run ended due to zero color at position " + std::to_string(pos));
                }
                if (!bLineEnded && zeroRunLength > 0 && !bZeroPixel) {
                    bAnyRunEnded = true;
                    logDebug("Zero run ended due to non-zero color at position " + std::to_string(pos));
                }
                if (bAnyRunEnded) {
                    if (runLength > 0) {
                        // Write positive count (run of non-zero pixels)
                        logDebug("Writing non-zero run of length " + std::to_string(runLength));
                        if (bits == 8) {
                            // Split long runs into multiple runs of max 127 pixels
                            while (runLength > 0) {
                                int currentRun = std::min(runLength, 127);
                                buffer.push_back(static_cast<int8_t>(currentRun));
                                nonZeroDataSize += currentRun;
                                buffer.insert(buffer.end(), 
                                    data.begin() + runStart,
                                    data.begin() + runStart + currentRun * bytesPerPixel);
                                runLength -= currentRun;
                                runStart += currentRun * bytesPerPixel;
                            }
                        }
                        else if (bits == 15 || bits == 16) {
                            buffer.push_back(static_cast<uint8_t>(runLength & 0xFF));
                            buffer.push_back(static_cast<uint8_t>((runLength >> 8) & 0xFF));
                            nonZeroDataSize += runLength;
                            buffer.insert(buffer.end(), data.begin() + runStart, data.begin() + pos);
                        }
                        else {
                            buffer.push_back(static_cast<uint8_t>(runLength & 0xFF));
                            buffer.push_back(static_cast<uint8_t>((runLength >> 8) & 0xFF));
                            buffer.push_back(static_cast<uint8_t>((runLength >> 16) & 0xFF));
                            buffer.push_back(static_cast<uint8_t>((runLength >> 24) & 0xFF));
                            nonZeroDataSize += runLength;
                            buffer.insert(buffer.end(), data.begin() + runStart, data.begin() + pos);
                        }
                        runLength = 0;
                    }
                    else if (zeroRunLength > 0) {
                        // Write negative count (run of zero pixels)
                        logInfo("Writing zero run of length " + std::to_string(zeroRunLength));
                        if (bits == 8) {
                            // Split long zero runs into multiple runs of max 128 pixels
                            while (zeroRunLength > 0) {
                                int currentRun = std::min(zeroRunLength, 128);
                                buffer.push_back(static_cast<int8_t>(-currentRun));
                                zeroRunLength -= currentRun;
                            }
                        }
                        else if (bits == 15 || bits == 16) {
                            buffer.push_back(static_cast<uint8_t>((-zeroRunLength) & 0xFF));
                            buffer.push_back(static_cast<uint8_t>(((-zeroRunLength) >> 8) & 0xFF));
                        }
                        else {
                            buffer.push_back(static_cast<uint8_t>((-zeroRunLength) & 0xFF));
                            buffer.push_back(static_cast<uint8_t>(((-zeroRunLength) >> 8) & 0xFF));
                            buffer.push_back(static_cast<uint8_t>(((-zeroRunLength) >> 16) & 0xFF));
                            buffer.push_back(static_cast<uint8_t>(((-zeroRunLength) >> 24) & 0xFF));
                        }
                        zeroRunLength = 0;
                        runStart = pos;
                    }
                }
                if (bZeroPixel) {
                    zeroRunLength++;
                }
                else {
                    runLength++;
                }
                pos += bytesPerPixel;
            }

            // Write end-of-line marker
            if (bits == 8) {
                buffer.push_back(static_cast<uint8_t>(RLE_EOL_MARKER_8 & 0xFF));
            }
            else if (bits == 15 || bits == 16) {
                buffer.push_back(static_cast<uint8_t>(RLE_EOL_MARKER_16 & 0xFF));
                buffer.push_back(static_cast<uint8_t>((RLE_EOL_MARKER_16 >> 8) & 0xFF));
            } else {
                buffer.push_back(static_cast<uint8_t>(RLE_EOL_MARKER_32 & 0xFF));
                buffer.push_back(static_cast<uint8_t>((RLE_EOL_MARKER_32 >> 8) & 0xFF));
                buffer.push_back(static_cast<uint8_t>((RLE_EOL_MARKER_32 >> 16) & 0xFF));
                buffer.push_back(static_cast<uint8_t>((RLE_EOL_MARKER_32 >> 24) & 0xFF));
            }
        }

        // Write data size (big endian)
        uint32_t dataSize = buffer.size() - 10 + nonZeroDataSize;
        logInfo("Writing final data size: " + std::to_string(dataSize) + 
                " (buffer size: " + std::to_string(buffer.size()) + 
                ", non-zero data: " + std::to_string(nonZeroDataSize) + ")");
        buffer[dataSizePos] = static_cast<uint8_t>((dataSize >> 24) & 0xFF);
        buffer[dataSizePos + 1] = static_cast<uint8_t>((dataSize >> 16) & 0xFF);
        buffer[dataSizePos + 2] = static_cast<uint8_t>((dataSize >> 8) & 0xFF);
        buffer[dataSizePos + 3] = static_cast<uint8_t>(dataSize & 0xFF);

        logInfo("RLE sprite serialization complete: " + std::to_string(buffer.size()) + " bytes");

        return buffer;
    }

    // Validate data size matches expected size
    size_t expectedSize = width * height * getBytesPerPixel();
    if (data.size() != expectedSize) {
        return std::vector<uint8_t>();
    }

    // Create buffer with enough space for header and data
    std::vector<uint8_t> buffer;
    buffer.reserve(6 + data.size());

    // Write header (big endian)
    buffer.push_back(static_cast<uint8_t>((bits >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(bits & 0xFF));
    buffer.push_back(static_cast<uint8_t>((width >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(width & 0xFF));
    buffer.push_back(static_cast<uint8_t>((height >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(height & 0xFF));

    // Append image data
    buffer.insert(buffer.end(), data.begin(), data.end());

    return buffer;
}

bool BitmapData::toRGB(uint8_t* outBuffer, std::vector<uint8_t>& palette) const {
    if (!outBuffer || data.empty() || width <= 0 || height <= 0) {
        return false;
    }

    if (typeID == ObjectType::DAT_PALETTE) {
        // For palette data, we need to convert to 24-bit RGB
        int palWidth = width / paletteScale;
        int palHeight = height / paletteScale;
        for (int i = 0; i < width; i++) {
            for (int j = 0; j < height; j++) {
                for (int k = 0; k < 3; k++) {
                    int paletteIndex = ((i/paletteScale)*palWidth + (j/paletteScale)) * 3 + k;
                    outBuffer[i*width*3 + j*3 + k] = data[paletteIndex];
                }
            }
        }
        return true;
    }

    if (bits == 8) {
        // For 8-bit indexed color, use the Allegro 4 palette
        for (int i = 0; i < width * height; i++) {
            uint8_t index = data[i];
            outBuffer[i*3] = palette[index*3];     // R
            outBuffer[i*3+1] = palette[index*3+1];   // G
            outBuffer[i*3+2] = palette[index*3+2];   // B
        }
    }
    else if (bits == 24) {
        // For 24-bit RGB, we can copy directly
        std::copy(data.begin(), data.end(), outBuffer);
    }
    else if (bits == 32 || bits == -32) {
        // For 32-bit RGBA, convert in-place
        const uint8_t* srcData = data.data();
        for (int i = 0; i < width * height; i++) {
            outBuffer[i*3] = srcData[i*4];     // R
            outBuffer[i*3+1] = srcData[i*4+1]; // G
            outBuffer[i*3+2] = srcData[i*4+2]; // B
        }
    }
    else if (bits == 15 || bits == 16) {
        // For 15/16-bit, convert in-place
        const uint16_t* hiColorData = reinterpret_cast<const uint16_t*>(data.data());
        for (int i = 0; i < width * height; i++) {
            uint16_t pixel = hiColorData[i];
            if (bits == 15) { // RRRRRGGGGG1BBBBB
                outBuffer[i*3] = ((pixel >> 11) & 0x1F) << 3;     // R (5 bits)
                outBuffer[i*3+1] = ((pixel >> 6) & 0x1F) << 3;    // G (5 bits) 
                outBuffer[i*3+2] = (pixel & 0x1F) << 3;           // B (5 bits)
            }
            else {
                outBuffer[i*3] = ((pixel >> 11) & 0x1F) << 3;     // R
                outBuffer[i*3+1] = ((pixel >> 5) & 0x3F) << 2;    // G
                outBuffer[i*3+2] = (pixel & 0x1F) << 3;           // B
            }
        }
    }
    else {
        return false;
    }

    return true;
}

bool BitmapData::compareWithFile(const std::string& filepath, std::vector<uint8_t>* currentPalette) const {
    // Check if we have valid bitmap data
    if (data.empty() || width <= 0 || height <= 0 || !isValidFormat()) {
        return false;
    }
    
    wxImage image;
    if (!BitmapData::readFileToWxImage(filepath, image)) {
        return false;
    }
    // load the bitmap data from the wxImage
    BitmapData bitmap;
    if (!bitmap.loadFromWxImage(image, bits, currentPalette, false, false)) {
        return false;
    }
    // compare the bitmap data with the image buffer
    return (data.size() == bitmap.data.size() && 
            std::memcmp(data.data(), bitmap.data.data(), data.size()) == 0);
}

bool BitmapData::compareWithWxImage(const wxImage& image) const {
    // Convert BitmapData to wxImage
    wxImage bitmapImage;
    if (!toWxImage(bitmapImage)) {
        return false;
    }
    
    // Compare dimensions
    if (bitmapImage.GetWidth() != image.GetWidth() || 
        bitmapImage.GetHeight() != image.GetHeight()) {
        return false;
    }
    
    // Compare data
    return memcmp(bitmapImage.GetData(), image.GetData(), 
                 bitmapImage.GetWidth() * bitmapImage.GetHeight() * 3) == 0;
}

bool BitmapData::ReadPCXFile(const wxString& filename, wxImage& image) {
    std::ifstream file(filename.ToStdString(), std::ios::binary);
    if (!file.is_open()) {
        logError("Failed to open PCX file: " + filename.ToStdString());
        return false;
    }

    // Read PCX header
    uint8_t header[128];
    file.read(reinterpret_cast<char*>(header), 128);
    if (file.gcount() != 128) {
        logError("Failed to read PCX header");
        return false;
    }

    // Check PCX signature
    if (header[0] != 0x0A) {
        logError("Invalid PCX signature");
        return false;
    }

    // Get image dimensions and format
    int xMin = (header[5] << 8) | header[4];
    int yMin = (header[7] << 8) | header[6];
    int xMax = (header[9] << 8) | header[8];
    int yMax = (header[11] << 8) | header[10];
    int width = xMax - xMin + 1;
    int height = yMax - yMin + 1;
    int bitsPerPixel = header[3];
    int numPlanes = header[65];
    
    // Calculate bytes per line (must be even)
    int bytesPerLine = ((width + 1) & ~1);

    // Validate header values
    if (width <= 0 || height <= 0 || width > 65535 || height > 65535) {
        logError("Invalid PCX dimensions");
        return false;
    }

    if (bitsPerPixel != 1 && bitsPerPixel != 8) {
        logError("Invalid bits per pixel: " + std::to_string(bitsPerPixel));
        return false;
    }

    if (numPlanes != 1 && numPlanes != 3) {
        logError("Invalid number of planes: " + std::to_string(numPlanes));
        return false;
    }

    logDebug("PCX Info - Width: " + std::to_string(width) + 
            ", Height: " + std::to_string(height) + 
            ", BPP: " + std::to_string(bitsPerPixel) + 
            ", Planes: " + std::to_string(numPlanes) +
            ", BytesPerLine: " + std::to_string(bytesPerLine));

    // Validate format
    bool is24BitColor = (bitsPerPixel == 8 && numPlanes == 3);
    bool is8BitIndexed = (bitsPerPixel == 8 && numPlanes == 1);
    bool is1BitMono = (bitsPerPixel == 1 && numPlanes == 1);

    if (!is24BitColor && !is8BitIndexed && !is1BitMono) {
        logError("Unsupported PCX format. Only 24-bit color, 8-bit indexed color, or 1-bit monochrome is supported");
        return false;
    }

    // Allocate memory for scanline and final image
    std::vector<uint8_t> scanline(bytesPerLine * numPlanes);
    std::vector<uint8_t> rgbData(width * height * 3);

    // Process each scanline
    for (int y = 0; y < height; y++) {
        size_t linePos = 0;
        size_t bytesReadForLine = 0;
        
        // Read one scanline (all planes)
        while (linePos < bytesPerLine * numPlanes) {
            uint8_t byte;
            if (!file.read(reinterpret_cast<char*>(&byte), 1)) {
                logWarning("End of file reached early at line " + std::to_string(y) + " position " + std::to_string(linePos) + " of " + std::to_string(bytesPerLine * numPlanes) + ". Padding rest of scanline with zeros.");
                // Pad the rest of the scanline with zeros
                std::fill(scanline.begin() + linePos, scanline.end(), 0);
                break;
            }
            bytesReadForLine++;

            if ((byte & 0xC0) == 0xC0) {
                // RLE compressed data
                int count = byte & 0x3F;
                if (!file.read(reinterpret_cast<char*>(&byte), 1)) {
                    logError("Failed to read RLE data");
                    return false;
                }
                bytesReadForLine++;

                // Only fill up to the end of the scanline buffer
                while (count > 0 && linePos < bytesPerLine * numPlanes) {
                    scanline[linePos++] = byte;
                    count--;
                }
            } else {
                // Uncompressed data
                scanline[linePos++] = byte;
            }
        }
        logDebug("ReadPCXFile: Read " + std::to_string(bytesReadForLine) + " bytes for line " + std::to_string(y));

        // Convert scanline to RGB pixels
        if (is24BitColor) {
            // 24-bit color (3 planes)
            for (int x = 0; x < width; x++) {
                size_t dstPos = (y * width + x) * 3;
                rgbData[dstPos]     = scanline[x];
                rgbData[dstPos + 1] = scanline[x + bytesPerLine];
                rgbData[dstPos + 2] = scanline[x + bytesPerLine * 2];
            }
        } else if (is1BitMono) {
            // 1-bit monochrome
            for (int x = 0; x < width; x++) {
                int byteIndex = x / 8;
                int bitIndex = 7 - (x % 8);
                if (byteIndex < bytesPerLine) {
                    uint8_t pixel = (scanline[byteIndex] & (1 << bitIndex)) ? 255 : 0;
                    size_t dstPos = (y * width + x) * 3;
                    if (dstPos + 2 < rgbData.size()) {
                        rgbData[dstPos] = pixel;     // R
                        rgbData[dstPos + 1] = pixel; // G
                        rgbData[dstPos + 2] = pixel; // B
                    }
                }
            }
        } else {
            // 8-bit indexed color
            for (int x = 0; x < width; x++) {
                if (x < bytesPerLine) {
                    size_t dstPos = (y * width + x) * 3;
                    if (dstPos + 2 < rgbData.size()) {
                        // Store the color index temporarily in all channels
                        rgbData[dstPos] = scanline[x];     // R
                        rgbData[dstPos + 1] = scanline[x]; // G
                        rgbData[dstPos + 2] = scanline[x]; // B
                    }
                }
            }
        }

        // After RLE decoding, print the first 8 bytes of each plane for the first scanline
        if (y == 0) {
            std::string rPlane, gPlane, bPlane;
            for (int i = 0; i < std::min(8, bytesPerLine); ++i) {
                rPlane += std::to_string(scanline[i]) + " ";
                gPlane += std::to_string(scanline[i + bytesPerLine]) + " ";
                bPlane += std::to_string(scanline[i + bytesPerLine * 2]) + " ";
            }
            logDebug("First scanline R: " + rPlane);
            logDebug("First scanline G: " + gPlane);
            logDebug("First scanline B: " + bPlane);
        }
    }

    // Handle palette for 8-bit indexed color
    if (is8BitIndexed) {
        file.seekg(-769, std::ios::end);
        uint8_t paletteMarker;
        file.read(reinterpret_cast<char*>(&paletteMarker), 1);
        if (!file || paletteMarker != 0x0C) {
            logError("Invalid or missing palette marker");
            return false;
        }

        std::vector<uint8_t> palette(768);
        file.read(reinterpret_cast<char*>(palette.data()), 768);
        if (file.gcount() != 768) {
            logError("Failed to read palette data");
            return false;
        }

        // Apply palette
        for (size_t i = 0; i < rgbData.size(); i += 3) {
            uint8_t index = rgbData[i]; // The index was stored in all channels
            rgbData[i] = palette[index * 3];
            rgbData[i + 1] = palette[index * 3 + 1];
            rgbData[i + 2] = palette[index * 3 + 2];
        }
    }

    // Create wxImage
    image.Create(width, height);
    memcpy(image.GetData(), rgbData.data(), rgbData.size());

    logInfo("Successfully loaded PCX image");
    return true;
}

bool BitmapData::loadFromWxImage(const wxImage& image, int bits, std::vector<uint8_t>* currentPalette, bool useDithering, bool preserveTransparency) {
    if (!image.IsOk()) {
        return false;
    }

    // Get image dimensions
    int width = image.GetWidth();
    int height = image.GetHeight();

    // Set properties directly
    this->width = width;
    this->height = height;
    
    // If bits is 0, use image's alpha channel to determine bit depth
    // Otherwise use the specified bit depth
    this->bits = (bits == 0) ? (image.HasAlpha() ? -32 : 24) : bits;

    logDebug("Loading image with bits: " + std::to_string(this->bits) + " dithering: " + std::to_string(useDithering) + " preserveTransparency: " + std::to_string(preserveTransparency));
    
    // Create a copy of the image for dithering if needed
    wxImage workingImage = image;
    if ((this->bits == 8 || this->bits == 15 || this->bits == 16) && useDithering) {
        std::vector<uint8_t>& palette = currentPalette ? *currentPalette : BitmapData::allegro_palette;
        applyDithering(workingImage, palette, this->bits);
    }

    // Get raw data from working image
    unsigned char* rgbData = workingImage.GetData();
    unsigned char* alphaData = workingImage.HasAlpha() ? workingImage.GetAlpha() : nullptr;

    // Convert RGB data to BitmapData format
    if (this->bits == 8) {
        std::vector<uint8_t>& palette = currentPalette ? *currentPalette : BitmapData::allegro_palette;
        logDebug("Starting 8-bit color conversion with " + std::to_string(width) + "x" + std::to_string(height) + " image");

        // Resize data vector for 8-bit indexed color
        this->data.resize(width * height);
        logDebug("Allocated " + std::to_string(this->data.size()) + " bytes for 8-bit data");
        bool compiledSprite = (typeID == 'CMP ' || typeID == 'XCMP');
        bool rleSprite = (typeID == 'RLE ');
        if (rleSprite) {
            this->alpha.resize(width * height, 255);
        }

        // Convert each pixel to its closest palette index
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 3;
                int pixelIdx = y * width + x;

                if (compiledSprite && compareRGBwithColor(rgbData + idx, RLE_ZERO_COLOR_32, 24)) {
                    this->data[pixelIdx] = 0;
                    continue;
                }
                if (rleSprite && compareRGBwithColor(rgbData + idx, RLE_ZERO_COLOR_32, 24)) {
                    this->data[pixelIdx] = 0;
                    this->alpha[pixelIdx] = 0;
                    continue;
                }
                if (preserveTransparency && compareRGBwithColor(rgbData + idx, TRANSPARENT_COLOR, 24)) {
                    this->data[pixelIdx] = 0;
                    continue;
                }

                // Get RGB values
                uint8_t r = rgbData[idx];
                uint8_t g = rgbData[idx + 1];
                uint8_t b = rgbData[idx + 2];

                if (x == 0 && y == 0) {
                    logDebug("First pixel RGB: (" + std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b) + ")");
                }

                // Find closest color in palette
                int startIndex = compiledSprite || preserveTransparency ? 1 : 0;
                int bestIndex = startIndex;
                int bestDistance = INT_MAX;

                for (int i = startIndex; i < 256; i++) {
                    int paletteIdx = i * 3;
                    int dr = r - palette[paletteIdx];
                    int dg = g - palette[paletteIdx + 1];
                    int db = b - palette[paletteIdx + 2];
                    int distance = dr * dr + dg * dg + db * db;

                    if (distance < bestDistance) {
                        bestDistance = distance;
                        bestIndex = i;
                    }
                }                

                // Store palette index
                this->data[pixelIdx] = bestIndex;
            }
        }
        logDebug("Completed 8-bit color conversion");
    } else if (this->bits == 24 || this->bits == 32) {
        this->data.resize(width * height * 3);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 3;
                int pixelIdx = y * width + x;

                // Set RGB values
                this->data[pixelIdx * 3] = rgbData[idx];     // R
                this->data[pixelIdx * 3 + 1] = rgbData[idx + 1]; // G
                this->data[pixelIdx * 3 + 2] = rgbData[idx + 2]; // B
            }
        }
    } else if (this->bits == -32) {
        this->data.resize(width * height * 4);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 3;
                int pixelIdx = y * width + x;

                // Set RGB values
                this->data[pixelIdx * 4] = rgbData[idx];     // R
                this->data[pixelIdx * 4 + 1] = rgbData[idx + 1]; // G
                this->data[pixelIdx * 4 + 2] = rgbData[idx + 2]; // B
                this->data[pixelIdx * 4 + 3] = alphaData ? alphaData[pixelIdx] : 255; // A (use 255 if no alpha)
            }
        }
    } else if (this->bits == 15 || this->bits == 16) {
        this->data.resize(width * height * 2); // 2 bytes per pixel for RGB
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int idx = (y * width + x) * 3;
                int pixelIdx = y * width + x;

                if (preserveTransparency && compareRGBwithColor(rgbData + idx, TRANSPARENT_COLOR, 24)) {
                    logDebug("Preserving transparency for pixel " + std::to_string(x) + "," + std::to_string(y));
                    if (this->bits == 15) {
                        this->data[pixelIdx * 2] = TRANSPARENT_COLOR_15 & 0xFF;
                        this->data[pixelIdx * 2 + 1] = (TRANSPARENT_COLOR_15 >> 8) & 0xFF;
                    } else {
                        this->data[pixelIdx * 2] = TRANSPARENT_COLOR_16 & 0xFF;
                        this->data[pixelIdx * 2 + 1] = (TRANSPARENT_COLOR_16 >> 8) & 0xFF;
                    }
                    continue;
                }

                // Convert 24-bit RGB to 16-bit RGB
                unsigned char r = rgbData[idx];
                unsigned char g = rgbData[idx + 1];
                unsigned char b = rgbData[idx + 2];

                // Convert to RGB16 (5 bits for R, 5/6 bits for G, 5 bits for B)
                uint16_t rgb16bit;
                if (this->bits == 15) {   // RRRRRGGGGG1BBBBB
                    rgb16bit = ((r >> 3) << 11) | ((g >> 3) << 6) | (b >> 3);
                    rgb16bit = rgb16bit | (1 << 5);
                    if (preserveTransparency && rgb16bit == TRANSPARENT_COLOR_15) {
                        rgb16bit = REFERENCE_COLOR_1516;
                    }
                }
                else {
                    rgb16bit = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
                    if (preserveTransparency && rgb16bit == TRANSPARENT_COLOR_16) {
                        rgb16bit = REFERENCE_COLOR_1516;
                    }
                }

                // Store as little-endian
                this->data[pixelIdx * 2] = rgb16bit & 0xFF;         // Low byte
                this->data[pixelIdx * 2 + 1] = (rgb16bit >> 8) & 0xFF; // High byte
            }
        }
    } else {
        return false; // Unsupported bit depth
    }

    updateAlphaChannel();

    return true;
}

bool BitmapData::importFromFile(const std::string& filepath, std::vector<uint8_t>* currentPalette, bool useDithering, bool preserveTransparency) {
    wxImage image;
    if (!BitmapData::readFileToWxImage(filepath, image)) {
        return false;
    }
    return loadFromWxImage(image, bits, currentPalette, useDithering, preserveTransparency);
}

bool BitmapData::generateOptimalPalette(const wxImage& image, std::vector<uint8_t>& palette) {
    if (!image.IsOk()) {
        palette = BitmapData::allegro_palette;  // Set to default Allegro palette on failure
        return false;
    }

    // Get image dimensions and data
    int width = image.GetWidth();
    int height = image.GetHeight();
    unsigned char* rgbData = image.GetData();

    // Create a vector to store unique colors (RGB triples stored consecutively)
    std::vector<uint8_t> uniqueColors;
    std::unordered_set<uint32_t> colorSet;

    // Collect unique colors from the image
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            uint8_t r = rgbData[idx];
            uint8_t g = rgbData[idx + 1];
            uint8_t b = rgbData[idx + 2];

            // Create a unique key for the color
            uint32_t colorKey = (r << 16) | (g << 8) | b;
            
            // If this is a new color, add it to our collection
            if (colorSet.insert(colorKey).second) {
                uniqueColors.push_back(r);
                uniqueColors.push_back(g);
                uniqueColors.push_back(b);
            }
        }
    }

    // If we have fewer than 256 unique colors, just use them all
    if (uniqueColors.size() <= 256 * 3) {
        // Copy the unique colors to the palette
        palette = uniqueColors;
        // Fill remaining slots with black
        palette.resize(256 * 3, 0);
        return true;
    }

    // Use median cut algorithm to reduce to 256 colors
    std::vector<std::vector<uint8_t>> buckets(1);
    buckets[0] = uniqueColors;

    while (buckets.size() < 256) {
        // Find the bucket with the largest range
        size_t maxRangeBucket = 0;
        int maxRange = 0;
        int maxChannel = 0;

        for (size_t i = 0; i < buckets.size(); i++) {
            if (buckets[i].size() <= 3) continue;  // Need at least one complete RGB triple

            // Find the channel with the largest range
            int minR = 255, maxR = 0;
            int minG = 255, maxG = 0;
            int minB = 255, maxB = 0;

            for (size_t j = 0; j < buckets[i].size(); j += 3) {
                minR = std::min(minR, static_cast<int>(buckets[i][j]));
                maxR = std::max(maxR, static_cast<int>(buckets[i][j]));
                minG = std::min(minG, static_cast<int>(buckets[i][j + 1]));
                maxG = std::max(maxG, static_cast<int>(buckets[i][j + 1]));
                minB = std::min(minB, static_cast<int>(buckets[i][j + 2]));
                maxB = std::max(maxB, static_cast<int>(buckets[i][j + 2]));
            }

            int rangeR = maxR - minR;
            int rangeG = maxG - minG;
            int rangeB = maxB - minB;

            int maxChannelRange = std::max({rangeR, rangeG, rangeB});
            if (maxChannelRange > maxRange) {
                maxRange = maxChannelRange;
                maxRangeBucket = i;
                maxChannel = (rangeR == maxChannelRange) ? 0 : 
                            (rangeG == maxChannelRange) ? 1 : 2;
            }
        }

        // Sort the bucket by the channel with the largest range
        auto& bucket = buckets[maxRangeBucket];
        std::vector<std::array<uint8_t, 3>> rgbTriples;
        rgbTriples.reserve(bucket.size() / 3);
        
        // Convert flat array to RGB triples
        for (size_t i = 0; i < bucket.size(); i += 3) {
            rgbTriples.push_back({bucket[i], bucket[i + 1], bucket[i + 2]});
        }
        
        // Sort RGB triples by the channel with largest range
        std::sort(rgbTriples.begin(), rgbTriples.end(),
            [maxChannel](const std::array<uint8_t, 3>& a, const std::array<uint8_t, 3>& b) {
                return a[maxChannel] < b[maxChannel];
            });
        
        // Convert back to flat array
        bucket.clear();
        for (const auto& rgb : rgbTriples) {
            bucket.push_back(rgb[0]);
            bucket.push_back(rgb[1]);
            bucket.push_back(rgb[2]);
        }

        // Split the bucket at the median
        size_t median = (bucket.size() / 6) * 3;  // Ensure we split at a complete RGB triple
        std::vector<uint8_t> newBucket(
            bucket.begin() + median, bucket.end());
        bucket.resize(median);
        buckets.push_back(newBucket);
    }

    // Calculate the average color for each bucket
    palette.clear();
    palette.reserve(256 * 3);
    for (const auto& bucket : buckets) {
        if (bucket.empty()) {
            palette.push_back(0);
            palette.push_back(0);
            palette.push_back(0);
            continue;
        }

        uint32_t sumR = 0, sumG = 0, sumB = 0;
        size_t count = bucket.size() / 3;
        for (size_t i = 0; i < bucket.size(); i += 3) {
            sumR += bucket[i];
            sumG += bucket[i + 1];
            sumB += bucket[i + 2];
        }

        palette.push_back(static_cast<uint8_t>(sumR / count));
        palette.push_back(static_cast<uint8_t>(sumG / count));
        palette.push_back(static_cast<uint8_t>(sumB / count));
    }

    // Fill any remaining palette entries with black
    palette.resize(256 * 3, 0);

    return true;
}

wxString BitmapData::getPreviewCaption() const {
    if (isPalette()) {
        return "Palette";
    }
    // For other bitmap types, return dimensions and bit depth
    wxString sizeHumanReadable = wxString::Format("%d B", static_cast<int>(data.size()));
    if (data.size() > 1024) {
        sizeHumanReadable = wxString::Format("%.2f KB", static_cast<double>(data.size()) / 1024);
    }
    if (data.size() > 1024 * 1024) {
        sizeHumanReadable = wxString::Format("%.2f MB", static_cast<double>(data.size()) / (1024 * 1024));
    }
    wxString TypeString;
    bool isAlphaNeeded = false;
    if (typeID == 'RLE ') {
        TypeString = "RLE sprite";
        if (bits == -32) {
            isAlphaNeeded = true;
        }
    }
    else if (typeID == 'CMP ') {
        TypeString = "Compiled sprite";
    }
    else if (typeID == 'XCMP') {
        if (bits == 8) {
            TypeString = "Mode-X compiled sprite";
        }
        else {
            return wxString::Format("!!! %d bit XC sprite not possible !!!", bits);
        }
    }
    else {
        TypeString = "Bitmap";
        if (bits == -32) {
            isAlphaNeeded = true;
        }
    }
    return wxString::Format("%s (%dx%d, %d bit)%s size: %s", 
                           TypeString, width, height, abs(bits), isAlphaNeeded ? " +alpha," : "", sizeHumanReadable);
}

bool BitmapData::isPalette() const {
    return typeID == 'PAL ';
}

bool BitmapData::createFromPalette(const std::vector<uint8_t>& palette, BitmapData& bitmap) {
    // Check if palette size is valid (must be 768 bytes for 256 RGB colors)
    if (palette.size() != 256 * 3) {
        // If invalid, use Allegro default palette
        logWarning("Invalid palette size, using Allegro default palette");
        return createFromPalette(allegro_palette, bitmap);
    }

    // Initialize BitmapData fields
    bitmap.typeID = ObjectType::DAT_PALETTE;  // Set as palette type
    bitmap.width = 16;
    bitmap.height = 16;
    bitmap.bits = 24;  // Store as 24-bit RGB
    bitmap.data.resize(16 * 16 * 3);  // 16x16 grid of colors

    // Copy colors from palette to the grid
    memcpy(bitmap.data.data(), palette.data(), 256 * 3);

    return true;
}

bool BitmapData::toWxImage(wxImage& outImage, std::vector<uint8_t>& palette, bool PreserveTransparency) const {
    
    if (typeID == ObjectType::DAT_PALETTE) {
        
        // For palette data, we need to convert to 24-bit RGB
        int palWidth = width * paletteScale;
        int palHeight = height * paletteScale;
        outImage.Create(palWidth, palHeight);
        unsigned char* rgbData = outImage.GetData();
        for (int i = 0; i < palWidth; i++) {
            for (int j = 0; j < palHeight; j++) {
                for (int k = 0; k < 3; k++) {
                    int paletteIndex = ((i/paletteScale)*width + (j/paletteScale)) * 3 + k;
                    rgbData[i*palWidth*3 + j*3 + k] = data[paletteIndex];
                }
            }
        }
        return true;
    }

    uint8_t transparentR = (TRANSPARENT_COLOR >> 16) & 0xFF;
    uint8_t transparentG = (TRANSPARENT_COLOR >> 8) & 0xFF;
    uint8_t transparentB = TRANSPARENT_COLOR & 0xFF;
    // Create a new wxImage with the same dimensions
    outImage.Create(width, height);
    unsigned char* rgbData = outImage.GetData();

    bool compiledSprite = (typeID == 'CMP ' || typeID == 'XCMP');
    if (compiledSprite && bits == 8) {
        outImage.SetAlpha();
        std::fill(outImage.GetAlpha(), outImage.GetAlpha() + width * height, 255);
        for (int i = 0; i < width * height; i++) {
            uint8_t index = data[i];
            rgbData[i*3] = palette[index*3];     // R
            rgbData[i*3+1] = palette[index*3+1];   // G
            rgbData[i*3+2] = palette[index*3+2];   // B
            if (index == 0) {
                outImage.GetAlpha()[i] = 0;
            }
        }
        return true;
    }
    
    if (bits == 8) {
        // For 8-bit indexed color, use palette from argument
        for (int i = 0; i < width * height; i++) {
            uint8_t index = data[i];
            rgbData[i*3] = palette[index*3];     // R
            rgbData[i*3+1] = palette[index*3+1];   // G
            rgbData[i*3+2] = palette[index*3+2];   // B
            if (PreserveTransparency) {
                if (index == 0) {
                    rgbData[i*3] = transparentR;
                    rgbData[i*3+1] = transparentG;
                    rgbData[i*3+2] = transparentB;
                } else if (compareRGBwithColor(palette.data() + index*3, TRANSPARENT_COLOR, 24)) {
                    rgbData[i*3] = transparentR;
                    rgbData[i*3+1] = transparentG+1;    // make it slightly different from transparent color
                    rgbData[i*3+2] = transparentB;
                }
            }
        }
    }
    else if (bits == 24 || bits == 32) {
        // For 24-bit RGB, we can copy directly
        std::copy(data.begin(), data.end(), rgbData);
    }
    else if (bits == -32) {
        // For 32-bit RGBA, convert in-place
        const uint8_t* srcData = data.data();
        for (int i = 0; i < width * height; i++) {
            rgbData[i*3] = srcData[i*4];     // R
            rgbData[i*3+1] = srcData[i*4+1]; // G
            rgbData[i*3+2] = srcData[i*4+2]; // B
        }
    }
    else if (bits == 15 || bits == 16) {
        // For 15/16-bit, convert in-place
        const uint16_t* hiColorData = reinterpret_cast<const uint16_t*>(data.data());
        for (int i = 0; i < width * height; i++) {
            uint16_t pixel = hiColorData[i];
            if (bits == 15) { // RRRRRGGGGG1BBBBB
                rgbData[i*3] = ((pixel >> 11) & 0x1F) << 3;     // R (5 bits)
                rgbData[i*3+1] = ((pixel >> 6) & 0x1F) << 3;    // G (5 bits) 
                rgbData[i*3+2] = (pixel & 0x1F) << 3;           // B (5 bits)
            }
            else {
                rgbData[i*3] = ((pixel >> 11) & 0x1F) << 3;     // R
                rgbData[i*3+1] = ((pixel >> 5) & 0x3F) << 2;    // G
                rgbData[i*3+2] = (pixel & 0x1F) << 3;           // B
            }
            if (PreserveTransparency) {
                if (bits == 15 && pixel == TRANSPARENT_COLOR_15) {
                    rgbData[i*3] = transparentR;
                    rgbData[i*3+1] = transparentG;
                    rgbData[i*3+2] = transparentB;
                } else if (bits == 16 && pixel == TRANSPARENT_COLOR_16) {
                    rgbData[i*3] = transparentR;
                    rgbData[i*3+1] = transparentG;
                    rgbData[i*3+2] = transparentB;
                }
            }
        }
    }
    else {
        return false;
    }
    if (alpha.size() > 0) {
        // copy the alpha channel
        outImage.SetAlpha();
        std::copy(alpha.begin(), alpha.end(), outImage.GetAlpha());
    }

    return true;
}

bool BitmapData::readFileToWxImage(const wxString& filepath, wxImage& image) {
    return image.LoadFile(filepath);
}

bool BitmapData::isMonocolor() const {
    if (data.empty() || width == 0 || height == 0) {
        return true; // Empty bitmap is considered monocolor
    }

    // Get bytes per pixel based on bit depth
    int bytesPerPixel = getBytesPerPixel();
    if (bytesPerPixel == 0) {
        return false; // Invalid format
    }

    // Get the first pixel's color
    if (bits == 8) {
        // For 8-bit indexed color, check if all pixels have the same index
        uint8_t firstIndex = data[0];
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int offset = y * width + x;
                if (data[offset] != firstIndex) {
                    return false;
                }
            }
        }
    }
    else if (bits == 15 || bits == 16) {
        // For 15/16-bit color, compare 16-bit values
        uint16_t firstPixel = *reinterpret_cast<const uint16_t*>(data.data());
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int offset = (y * width + x) * bytesPerPixel;
                uint16_t pixel = *reinterpret_cast<const uint16_t*>(data.data() + offset);
                if (pixel != firstPixel) {
                    return false;
                }
            }
        }
    }
    else if (bits == 24 || bits == 32) {
        // For 24/32-bit color, compare RGB components
        uint8_t firstR = data[0];
        uint8_t firstG = data[1];
        uint8_t firstB = data[2];

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int offset = (y * width + x) * bytesPerPixel;
                if (data[offset] != firstR || 
                    data[offset + 1] != firstG || 
                    data[offset + 2] != firstB) {
                    return false;
                }
            }
        }
    }
    else if (bits == -32) {
        // For true 32-bit RGBA, compare all components
        uint8_t firstR = data[0];
        uint8_t firstG = data[1];
        uint8_t firstB = data[2];
        uint8_t firstA = data[3];

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int offset = (y * width + x) * bytesPerPixel;
                if (data[offset] != firstR || 
                    data[offset + 1] != firstG || 
                    data[offset + 2] != firstB ||
                    data[offset + 3] != firstA) {
                    return false;
                }
            }
        }
    }
    else {
        return false; // Unsupported bit depth
    }

    return true; // All pixels are the same color
}

bool BitmapData::autoCrop(int& xCrop, int& yCrop) {
    xCrop = 0;
    yCrop = 0;
    if (data.empty() || width == 0 || height == 0) {
        return false; // Nothing to crop
    }

    // Get bytes per pixel based on bit depth
    int bytesPerPixel = getBytesPerPixel();
    if (bytesPerPixel == 0) {
        return false; // Invalid format
    }

    bool wasCropped = false;
    // Find bounds of non-transparent content
    int minX = width, minY = height, maxX = 0, maxY = 0;

    // First pass: find the dominant color
    std::map<std::vector<uint8_t>, int> colorCounts;
    for (int py = 0; py < height; py++) {
        for (int px = 0; px < width; px++) {
            int offset = (py * width + px) * bytesPerPixel;
            std::vector<uint8_t> color;
            
            if (bits == 8) {
                // For 8-bit, use the palette index as the color
                color.push_back(data[offset]);
            }
            else if (bits == 15 || bits == 16) {
                // For 15/16-bit, convert to RGB
                uint16_t pixel = *reinterpret_cast<const uint16_t*>(data.data() + offset);
                if (bits == 15) {
                    color.push_back(((pixel >> 11) & 0x1F) << 3);     // R
                    color.push_back(((pixel >> 6) & 0x1F) << 3);      // G
                    color.push_back((pixel & 0x1F) << 3);             // B
                } else {
                    color.push_back(((pixel >> 11) & 0x1F) << 3);     // R
                    color.push_back(((pixel >> 5) & 0x3F) << 2);      // G
                    color.push_back((pixel & 0x1F) << 3);             // B
                }
            }
            else if (bits == 24 || bits == 32) {
                // For 24/32-bit, use RGB components
                color.push_back(data[offset]);     // R
                color.push_back(data[offset + 1]); // G
                color.push_back(data[offset + 2]); // B
            }
            else if (bits == -32) {
                // For true 32-bit, use RGBA components
                color.push_back(data[offset]);     // R
                color.push_back(data[offset + 1]); // G
                color.push_back(data[offset + 2]); // B
                color.push_back(data[offset + 3]); // A
            }
            
            colorCounts[color]++;
        }
    }
    
    // Find the most common color
    std::vector<uint8_t> dominantColor;
    int maxCount = 0;
    for (const auto& [color, count] : colorCounts) {
        if (count > maxCount) {
            maxCount = count;
            dominantColor = color;
        }
    }
     
    // Second pass: find bounds of non-dominant color content
    for (int py = 0; py < height; py++) {
        for (int px = 0; px < width; px++) {
            int offset = (py * width + px) * bytesPerPixel;
            std::vector<uint8_t> currentColor;
            
            if (bits == 8) {
                currentColor.push_back(data[offset]);
            }
            else if (bits == 15 || bits == 16) {
                uint16_t pixel = *reinterpret_cast<const uint16_t*>(data.data() + offset);
                if (bits == 15) {
                    currentColor.push_back(((pixel >> 11) & 0x1F) << 3);
                    currentColor.push_back(((pixel >> 6) & 0x1F) << 3);
                    currentColor.push_back((pixel & 0x1F) << 3);
                } else {
                    currentColor.push_back(((pixel >> 11) & 0x1F) << 3);
                    currentColor.push_back(((pixel >> 5) & 0x3F) << 2);
                    currentColor.push_back((pixel & 0x1F) << 3);
                }
            }
            else if (bits == 24 || bits == 32) {
                currentColor.push_back(data[offset]);
                currentColor.push_back(data[offset + 1]);
                currentColor.push_back(data[offset + 2]);
            }
            else if (bits == -32) {
                currentColor.push_back(data[offset]);
                currentColor.push_back(data[offset + 1]);
                currentColor.push_back(data[offset + 2]);
                currentColor.push_back(data[offset + 3]);
            }
            
            if (currentColor != dominantColor) {
                minX = std::min(minX, px);
                minY = std::min(minY, py);
                maxX = std::max(maxX, px);
                maxY = std::max(maxY, py);
            }
        }
    }
     
    // Check if we found any non-dominant color content
    wasCropped = (minX <= maxX && minY <= maxY) && 
                (minX > 0 || minY > 0 || maxX < width-1 || maxY < height-1);

    if (wasCropped) {
        // Set the crop offsets
        xCrop = minX;
        yCrop = minY;

        // Crop the bitmap
        int cropWidth = maxX - minX + 1;
        int cropHeight = maxY - minY + 1;
        BitmapData croppedBitmap;
        croppedBitmap.width = cropWidth;
        croppedBitmap.height = cropHeight;
        croppedBitmap.bits = bits;
        croppedBitmap.data.resize(cropWidth * cropHeight * bytesPerPixel);
        
        // Copy the cropped area
        for (int y = 0; y < cropHeight; y++) {
            for (int x = 0; x < cropWidth; x++) {
                int srcOffset = ((minY + y) * width + (minX + x)) * bytesPerPixel;
                int dstOffset = (y * cropWidth + x) * bytesPerPixel;
                std::copy(data.begin() + srcOffset,
                         data.begin() + srcOffset + bytesPerPixel,
                         croppedBitmap.data.begin() + dstOffset);
            }
        }
        
        *this = croppedBitmap;
    }

    return wasCropped;
}

std::vector<BitmapData::GridCell> BitmapData::gridByColor(const wxImage& image, const wxColour& gridColor) {
    std::vector<GridCell> gridCells;

    if (!image.IsOk() || !gridColor.IsOk()) {
        logError("gridByColor: Input image or grid color is invalid.");
        return gridCells;
    }

    int width = image.GetWidth();
    int height = image.GetHeight();
    std::vector<bool> visited(width * height, false);

    unsigned char gridR = gridColor.Red();
    unsigned char gridG = gridColor.Green();
    unsigned char gridB = gridColor.Blue();
    bool checkAlpha = image.HasAlpha();
    unsigned char gridA = checkAlpha ? gridColor.Alpha() : 255;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int currentIdx = y * width + x;

            // Skip visited pixels
            if (visited[currentIdx]) {
                continue;
            }

            // Check if the current pixel is the grid color
            unsigned char r = image.GetRed(x, y);
            unsigned char g = image.GetGreen(x, y);
            unsigned char b = image.GetBlue(x, y);
            unsigned char a = checkAlpha ? image.GetAlpha(x, y) : 255;

            bool isGridPixel = (r == gridR && g == gridG && b == gridB && (!checkAlpha || a == gridA));

            // Mark grid pixels as visited and continue
            if (isGridPixel) {
                visited[currentIdx] = true;
                continue;
            }

            // Found the start of a non-grid region
            std::queue<std::pair<int, int>> q;
            q.push({x, y});
            visited[currentIdx] = true;

            int minX = x, minY = y, maxX = x, maxY = y; // Bounding box for the region

            while (!q.empty()) {
                std::pair<int, int> current = q.front();
                q.pop();
                int cx = current.first;
                int cy = current.second;

                // Update bounding box
                minX = std::min(minX, cx);
                minY = std::min(minY, cy);
                maxX = std::max(maxX, cx);
                maxY = std::max(maxY, cy);

                // Check neighbors (4-connectivity)
                int dx[] = {0, 0, 1, -1};
                int dy[] = {1, -1, 0, 0};

                for (int i = 0; i < 4; ++i) {
                    int nx = cx + dx[i];
                    int ny = cy + dy[i];
                    int neighborIdx = ny * width + nx;

                    // Check bounds
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                         // Check visited status
                        if (!visited[neighborIdx]) {
                            // Check if neighbor is NOT the grid color
                            unsigned char nr = image.GetRed(nx, ny);
                            unsigned char ng = image.GetGreen(nx, ny);
                            unsigned char nb = image.GetBlue(nx, ny);
                            unsigned char na = checkAlpha ? image.GetAlpha(nx, ny) : 255;
                            bool isNeighborGrid = (nr == gridR && ng == gridG && nb == gridB && (!checkAlpha || na == gridA));

                            if (!isNeighborGrid) {
                                // Add non-grid, unvisited neighbor to queue
                                visited[neighborIdx] = true;
                                q.push({nx, ny});
                            } else {
                                // Mark grid neighbors as visited so we don't process them later
                                visited[neighborIdx] = true;
                            }
                        }
                    }
                }
            }

            // BFS for this region is complete, calculate properties
            int cellWidth = maxX - minX + 1;
            int cellHeight = maxY - minY + 1;

            if (cellWidth > 0 && cellHeight > 0) {
                // Extract the sub-image based on the bounding box
                wxRect cellRect(minX, minY, cellWidth, cellHeight);
                wxImage subImage = image.GetSubImage(cellRect);

                // Add the found region as a GridCell
                gridCells.push_back({minX, minY, cellWidth, cellHeight, subImage});
                 logInfo(wxString::Format("Found cell at (%d, %d) size %dx%d", minX, minY, cellWidth, cellHeight).ToStdString());
            } else {
                 logWarning(wxString::Format("Found invalid cell at (%d, %d) size %dx%d", minX, minY, cellWidth, cellHeight).ToStdString());
            }
        }
    }
    logDebug("gridByColor finished. Found " + std::to_string(gridCells.size()) + " cells.");
    return gridCells;
}

std::vector<BitmapData::GridCell> BitmapData::gridBySize(const wxImage& image, int cellWidth, int cellHeight) {
    std::vector<GridCell> gridCells;
    
    if (!image.IsOk()) {
        return gridCells;
    }

    int imageWidth = image.GetWidth();
    int imageHeight = image.GetHeight();
    
    // Calculate the number of cells in each dimension
    int numCols = (imageWidth + cellWidth - 1) / cellWidth;
    int numRows = (imageHeight + cellHeight - 1) / cellHeight;
    
    // Iterate through each cell
    for (int row = 0; row < numRows; row++) {
        for (int col = 0; col < numCols; col++) {
            int x = col * cellWidth;
            int y = row * cellHeight;
            
            int width = cellWidth;
            if (width > imageWidth - x) width = imageWidth - x;
            int height = cellHeight;
            if (height > imageHeight - y) height = imageHeight - y;
            // Skip if cell is too small (just a grid line)
            if (width <= 1 || height <= 1) {
                continue;
            }

            // Create sub-image for this cell
            wxImage cellImage = image.GetSubImage(wxRect(x, y, width, height));
            gridCells.push_back(GridCell{x, y, width, height, cellImage});
        }
    }

    return gridCells;
}

double BitmapData::calculatePaletteMatch(const wxImage& image, const std::vector<uint8_t>& palette) {
    // Call the overloaded version with a temporary map that we discard
    std::unordered_map<uint32_t, int> tempColorMap;
    return calculatePaletteMatch(image, palette, tempColorMap);
}

double BitmapData::calculatePaletteMatch(const wxImage& image, const std::vector<uint8_t>& palette, 
                                        std::unordered_map<uint32_t, int>& colorMap) {
    colorMap.clear(); // Clear the output map
    
    if (!image.IsOk()) {
        return 0.0; // Return worst match for invalid inputs
    }

    int paletteSize = palette.size() / 3;
    // Get image dimensions and data
    int width = image.GetWidth();
    int height = image.GetHeight();
    unsigned char* rgbData = image.GetData();

    // Create a map to store unique colors and their frequencies
    std::map<uint32_t, int> colorFrequencies;
    int totalPixels = 0;

    // Collect all unique colors from the image
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            uint8_t r = rgbData[idx];
            uint8_t g = rgbData[idx + 1];
            uint8_t b = rgbData[idx + 2];

            // Create a unique key for the color
            uint32_t colorKey = (r << 16) | (g << 8) | b;
            colorFrequencies[colorKey]++;
            totalPixels++;
        }
    }

    // Calculate the total color distance for all pixels and populate the color map
    double totalDistance = 0.0;
    for (const auto& [colorKey, frequency] : colorFrequencies) {
        uint8_t r = (colorKey >> 16) & 0xFF;
        uint8_t g = (colorKey >> 8) & 0xFF;
        uint8_t b = colorKey & 0xFF;

        // Find the closest color in the palette
        int minDistance = INT_MAX;
        int bestPaletteIndex = 0;
        for (int i = 0; i < paletteSize; i++) {
            int paletteIdx = i * 3;
            int dr = r - palette[paletteIdx];
            int dg = g - palette[paletteIdx + 1];
            int db = b - palette[paletteIdx + 2];
            int distance = dr * dr + dg * dg + db * db;

            if (distance < minDistance) {
                minDistance = distance;
                bestPaletteIndex = i;
            }
        }

        // Store the best palette index for this color
        colorMap[colorKey] = bestPaletteIndex;

        // Add weighted distance to total
        totalDistance += minDistance * frequency;
    }

    // Normalize the result to a value between 0 and 1
    // The maximum possible distance for a color is 255^2 * 3 = 195075
    double maxPossibleDistance = paletteSize * paletteSize * 3 * totalPixels;
    return 1.0 - totalDistance / maxPossibleDistance;
}

bool BitmapData::compareRGBwithColor(const uint8_t* rgb, const int32_t color, int bits) {
    // Compare with input RGB array
    if (bits == 24 || bits == 32 || bits == -32) {
        // Extract RGB components from color integer
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t g = (color >> 8) & 0xFF; 
        uint8_t b = color & 0xFF;
        return (rgb[0] == r && rgb[1] == g && rgb[2] == b);
    }
    else if (bits == 16) {
        uint16_t pixel = *reinterpret_cast<const uint16_t*>(rgb);
        return (pixel == color);
    }
    else if (bits == 15) {
        uint16_t pixel = *reinterpret_cast<const uint16_t*>(rgb);
        pixel |= (1 << 5);
        int32_t colorToCompare = color | (1 << 5);
        return (colorToCompare == pixel);
    }
    return false;
}

bool BitmapData::extractAlphaChannel(const wxImage& image, std::vector<uint8_t>& outAlpha) {
    if (!image.IsOk()) {
        logWarning("Image for alpha is not correct");
        return false;
    }

    int width = image.GetWidth();
    int height = image.GetHeight();
    outAlpha.clear();
    outAlpha.resize(width * height);
    if (image.HasAlpha())
    {
        std::copy(image.GetAlpha(), image.GetAlpha() + (width * height), outAlpha.begin());
    }
    else
    {
        // Create a new alpha channel from brightness
        const unsigned char* rgb = image.GetData();
        for (int i = 0; i < width * height; i++) {
            // Calculate brightness just as mean of RGB
            outAlpha[i] = (rgb[i*3] + rgb[i*3+1] + rgb[i*3+2]) / 3;
        }
    }

    return true;
}

bool BitmapData::WritePCXFile(const wxString& filepath, const wxImage& image) {
    if (!image.IsOk()) {
        logError("Cannot save invalid image as PCX");
        return false;
    }

    int width = image.GetWidth();
    int height = image.GetHeight();
    
    std::ofstream file(filepath.ToStdString(), std::ios::binary);
    if (!file.is_open()) {
        logError("Failed to create PCX file: " + filepath.ToStdString());
        return false;
    }
    
    // Create PCX header
    uint8_t header[128] = {0};
    
    // Fill header with standard values
    header[0] = 0x0A;                  // Manufacturer: ZSoft
    header[1] = 0x05;                  // Version: 5
    header[2] = 0x01;                  // Encoding: RLE
    header[3] = 0x08;                  // Bits per pixel: 8
    
    // Image dimensions
    header[4] = 0;                     // X min (low byte)
    header[5] = 0;                     // X min (high byte)
    header[6] = 0;                     // Y min (low byte)
    header[7] = 0;                     // Y min (high byte)
    header[8] = (width - 1) & 0xFF;          // X max (low byte)
    header[9] = (width - 1) >> 8;      // X max (high byte)
    header[10] = (height - 1) & 0xFF;        // Y max (low byte)
    header[11] = (height - 1) >> 8;    // Y max (high byte)
    
    // Resolution (72 DPI)
    header[12] = 72 & 0xFF;            // Horizontal DPI (low byte)
    header[13] = 72 >> 8;              // Horizontal DPI (high byte)
    header[14] = 72 & 0xFF;            // Vertical DPI (low byte)
    header[15] = 72 >> 8;              // Vertical DPI (high byte)

    // Default 16-color palette (not used)
    for (int i = 0; i < 48; i++) {
        header[16 + i] = 0;
    }
    
    header[64] = 0;                    // Reserved
    header[65] = 3;                    // Number of planes (3 for RGB)
    
    // Bytes per line (must be even)
    int bytesPerLine = ((width + 1) & ~1);
    header[66] = bytesPerLine & 0xFF;  // Bytes per line (low byte)
    header[67] = bytesPerLine >> 8;    // Bytes per line (high byte)
    
    header[68] = 1;                    // Palette info (1 = color)
    
    // Fill the rest with zeros
    for (int i = 69; i < 128; i++) {
        header[i] = 0;
    }
    
    // Write header
    file.write(reinterpret_cast<const char*>(header), 128);
    
    // Get image data
    const unsigned char* imageData = image.GetData();
    
    // Helper function for RLE encoding a scanline
    auto encodeRLE = [&file](const unsigned char* data, int size) {
        for (int i = 0; i < size; i++) {
            unsigned char byte = data[i];
            int count = 1;
            
            // Count consecutive identical bytes
            while (i + count < size && data[i + count] == byte && count < 63) {
                count++;
            }
            
            if (count > 1 || (byte & 0xC0) == 0xC0) {
                // RLE encoding needed
                unsigned char countByte = 0xC0 | count;
                file.write(reinterpret_cast<const char*>(&countByte), 1);
                file.write(reinterpret_cast<const char*>(&byte), 1);
                i += count - 1;
            } else {
                // Single byte
                file.write(reinterpret_cast<const char*>(&byte), 1);
            }
        }
    };
    
    // Buffer for one plane's scanline
    std::vector<unsigned char> scanline(bytesPerLine);
    
    // Write image data plane by plane, line by line
    for (int y = 0; y < height; y++) {
        // Red plane
        for (int x = 0; x < width; x++) {
            scanline[x] = imageData[(y * width + x) * 3];
        }
        // Pad to even bytes
        if (width & 1) scanline[width] = 0;
        encodeRLE(scanline.data(), bytesPerLine);
        logDebug(wxString::Format("WritePCXFile: Wrote %d bytes for red plane, line %d", bytesPerLine, y).ToStdString());
        
        // Green plane
        for (int x = 0; x < width; x++) {
            scanline[x] = imageData[(y * width + x) * 3 + 1];
        }
        if (width & 1) scanline[width] = 0;
        encodeRLE(scanline.data(), bytesPerLine);
        logDebug(wxString::Format("WritePCXFile: Wrote %d bytes for green plane, line %d", bytesPerLine, y).ToStdString());
        
        // Blue plane
        for (int x = 0; x < width; x++) {
            scanline[x] = imageData[(y * width + x) * 3 + 2];
        }
        if (width & 1) scanline[width] = 0;
        encodeRLE(scanline.data(), bytesPerLine);
        logDebug(wxString::Format("WritePCXFile: Wrote %d bytes for blue plane, line %d", bytesPerLine, y).ToStdString());
    }
    
    // PCX files don't require a palette for 24-bit RGB images
    
    file.close();
    logInfo("Successfully saved PCX file: " + filepath.ToStdString());
    return true;
}

BitmapData BitmapData::createSampleBitmap(ObjectType typeID) {
    // Create a 32x32 wxImage with black background
    wxImage image(32, 32);
    image.SetRGB(wxRect(0, 0, 32, 32), 0, 0, 0);

    // Draw blue 'Hi!' text using wxMemoryDC and wxBitmap
    wxBitmap bmp(image);
    wxMemoryDC dc(bmp);
    dc.SetBackground(*wxBLACK_BRUSH);
    dc.Clear();
    wxFont font(16, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
    dc.SetFont(font);
    dc.SetTextForeground(wxColour(0, 0, 255));
    dc.DrawText("Hi!", 4, 4); // Centered-ish
    dc.SelectObject(wxNullBitmap);
    wxImage result = bmp.ConvertToImage();

    // Set background to zero color based on type
    if (typeID == ObjectType::DAT_RLE_SPRITE || typeID == ObjectType::DAT_C_SPRITE || typeID == ObjectType::DAT_XC_SPRITE) {
        result.SetAlpha();
        unsigned char* alpha = result.GetAlpha();
        unsigned char* rgb = result.GetData();
        
        // Extract RGB from RLE_ZERO_COLOR_32 for 24-bit color
        uint8_t zeroR = (RLE_ZERO_COLOR_32 >> 16) & 0xFF;
        uint8_t zeroG = (RLE_ZERO_COLOR_32 >> 8) & 0xFF;
        uint8_t zeroB = RLE_ZERO_COLOR_32 & 0xFF;

        for (int i = 0; i < 32 * 32; i++) {
            // If pixel is black (background), set to zero color and alpha to 0
            if (rgb[i*3] == 0 && rgb[i*3+1] == 0 && rgb[i*3+2] == 0) {
                rgb[i*3] = zeroR;
                rgb[i*3+1] = zeroG;
                rgb[i*3+2] = zeroB;
                alpha[i] = 0;
            } else {
                alpha[i] = 255;
            }
        }
    }

    // Convert wxImage to BitmapData
    BitmapData bdata;
    bdata.typeID = typeID;
    
    // Use 8-bit color for compiled sprites, 24-bit for others
    int bits = (typeID == ObjectType::DAT_C_SPRITE || typeID == ObjectType::DAT_XC_SPRITE) ? 8 : 24;
    bdata.loadFromWxImage(result, bits, nullptr, false, false);
    return bdata;
}

bool BitmapData::hasAlphaData() const {
    if ((typeID == ObjectType::DAT_BITMAP || typeID == ObjectType::DAT_RLE_SPRITE) && bits == -32) {
        return !alpha.empty() && alpha.size() == static_cast<size_t>(width * height);
    }
    return false;
}

wxImage BitmapData::getAlphaDataAsImage() const {
    if (!hasAlphaData()) {
        // Return white image if no alpha data
        wxImage alphaImg(width, height);
        alphaImg.SetRGB(wxRect(0, 0, width, height), 255, 255, 255);
        return alphaImg;
    }
    wxImage alphaImg(width, height);
    unsigned char* rgb = alphaImg.GetData();
    for (int i = 0; i < width * height; ++i) {
        unsigned char v = 255;
        if (i < alpha.size()) {
            v = alpha[i];
        }
        rgb[i * 3 + 0] = v;
        rgb[i * 3 + 1] = v;
        rgb[i * 3 + 2] = v;
    }
    return alphaImg;
}

void BitmapData::deleteAlphaData() {
    if (bits == -32) {
        alpha.clear();
        bits = 32;
        // convert 4 bytes to 3 bytes
        for (int i = 0; i < width * height; i++) {
            data[i*3] = data[i*4];
            data[i*3+1] = data[i*4+1];
            data[i*3+2] = data[i*4+2];
        }
        data.resize(width * height * 3);
    }
}

bool BitmapData::importAlphaDataFromImage(const wxImage& image) {
    // convert non 32 bits to 32 bits with alpha
    if (bits != -32) {
        // get as wxImage
        wxImage convertedImage;
        if (!toWxImage(convertedImage)) {
            return false;
        }
        data.clear();
        data.resize(width * height * 4);
        unsigned char* rgb = convertedImage.GetData();
        for (int i = 0; i < width * height; i++) {
            data[i*4] = rgb[i*3];
            data[i*4+1] = rgb[i*3+1];
            data[i*4+2] = rgb[i*3+2];
        }
        bits = -32;
    }

    // Resize alpha vector to match image dimensions
    alpha.resize(width * height);
    
    // Get image data
    const unsigned char* imageData = image.GetData();
    int imageWidth = image.GetWidth();
    int imageHeight = image.GetHeight();
    
    // Convert RGB values to brightness and store in alpha channel
    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            unsigned char grayscale = 255;
            if (i < imageWidth && j < imageHeight) {
                // Calculate brightness using standard luminance formula: 0.299*R + 0.587*G + 0.114*B
                unsigned char r = imageData[(j * imageWidth + i) * 3];
                unsigned char g = imageData[(j * imageWidth + i) * 3 + 1];
                unsigned char b = imageData[(j * imageWidth + i) * 3 + 2];
                
                grayscale = static_cast<unsigned char>(
                    ((int)r + (int)g + (int)b) / 3
                );
            }
        
            alpha[i*height+j] = grayscale;
            data[i*4+3] = grayscale;
        }
    }
    
    return true;
}

bool BitmapData::setColorDepth(int newBits, std::vector<uint8_t>& palette, bool useDithering, bool preserveTransparency) {
    // Check if the new bit depth is supported
    if (newBits != 8 && newBits != 15 && newBits != 16 && newBits != 24 && newBits != 32 && newBits != -32) {
        return false;
    }
    
    // If the current bit depth is already the target, no conversion needed
    if (abs(bits) == abs(newBits)) {
        return true;
    }
    
    wxImage tempImage;
    if (!toWxImage(tempImage, palette, preserveTransparency)) {
        logError("Failed to convert to wxImage");
        return false;
    }
    
    // Clear current data and convert to new format
    data.clear();
    alpha.clear();
    
    // Use loadFromWxImage to convert to the new format
    return loadFromWxImage(tempImage, newBits, &palette, useDithering, preserveTransparency);
}

bool BitmapData::setType(ObjectType newType, bool useDithering) {
    // If already the desired type, do nothing
    if (typeID == newType)
        return true;

    // Don't convert palette objects
    if (isPalette())
        return false;

    // Only support Allegro bitmap types
    if (newType != ObjectType::DAT_BITMAP &&
        newType != ObjectType::DAT_RLE_SPRITE &&
        newType != ObjectType::DAT_C_SPRITE &&
        newType != ObjectType::DAT_XC_SPRITE)
        return false;

    // Convert the data to the new type
    wxImage tempImage;
    if (!toWxImage(tempImage)) {
        return false;
    }
    // Update typeID
    typeID = newType;
    alpha.clear();
    if (bits == -32 && (newType == ObjectType::DAT_C_SPRITE || newType == ObjectType::DAT_XC_SPRITE)) {
        bits = 32;
    }
    // Convert the data to the new type
    if (!loadFromWxImage(tempImage, bits, nullptr, useDithering, false)) {
        return false;
    }

    return true;
}

void BitmapData::applyDithering(wxImage& image, const std::vector<uint8_t>& palette, int bits) {
    if (!image.IsOk()) {
        return;
    }

    logDebug("Applying dithering with bits: " + std::to_string(bits));
    
    int width = image.GetWidth();
    int height = image.GetHeight();
    unsigned char* rgbData = image.GetData();
    
    // Create error diffusion arrays for Floyd-Steinberg dithering
    std::vector<std::vector<int>> errorR(width, std::vector<int>(height, 0));
    std::vector<std::vector<int>> errorG(width, std::vector<int>(height, 0));
    std::vector<std::vector<int>> errorB(width, std::vector<int>(height, 0));
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            
            // Get original RGB values
            int oldR = rgbData[idx] + errorR[x][y];
            int oldG = rgbData[idx + 1] + errorG[x][y];
            int oldB = rgbData[idx + 2] + errorB[x][y];
            
            // Clamp values to 0-255 range
            oldR = std::max(0, std::min(255, oldR));
            oldG = std::max(0, std::min(255, oldG));
            oldB = std::max(0, std::min(255, oldB));
            
            int newR, newG, newB;
            
            if (bits == 8) {
                // 8-bit palette-based dithering
                // Find closest color in palette
                int bestIndex = 0;
                int bestDistance = INT_MAX;
                
                for (int i = 0; i < 256; i++) {
                    int paletteIdx = i * 3;
                    int dr = oldR - palette[paletteIdx];
                    int dg = oldG - palette[paletteIdx + 1];
                    int db = oldB - palette[paletteIdx + 2];
                    int distance = dr * dr + dg * dg + db * db;
                    
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        bestIndex = i;
                    }
                }
                
                // Get palette color
                int paletteIdx = bestIndex * 3;
                newR = palette[paletteIdx];
                newG = palette[paletteIdx + 1];
                newB = palette[paletteIdx + 2];
            }
            else if (bits == 15) {
                // 15-bit RGB dithering (RRRRRGGGGG1BBBBB)
                // Convert to 15-bit format and back to 24-bit for dithering
                uint16_t rgb15bit = ((oldR >> 3) << 11) | ((oldG >> 3) << 6) | (oldB >> 3);
                rgb15bit |= (1 << 5); // Set the unused bit
                
                // Convert back to 24-bit for display
                newR = ((rgb15bit >> 11) & 0x1F) << 3;
                newG = ((rgb15bit >> 6) & 0x1F) << 3;
                newB = (rgb15bit & 0x1F) << 3;
            }
            else if (bits == 16) {
                // 16-bit RGB dithering (RRRRRGGGGGGBBBBB)
                // Convert to 16-bit format and back to 24-bit for dithering
                uint16_t rgb16bit = ((oldR >> 3) << 11) | ((oldG >> 2) << 5) | (oldB >> 3);
                
                // Convert back to 24-bit for display
                newR = ((rgb16bit >> 11) & 0x1F) << 3;
                newG = ((rgb16bit >> 5) & 0x3F) << 2;
                newB = (rgb16bit & 0x1F) << 3;
            }
            else {
                // Unsupported bit depth, use original values
                newR = oldR;
                newG = oldG;
                newB = oldB;
            }
            
            // Calculate quantization error
            int errorR_val = oldR - newR;
            int errorG_val = oldG - newG;
            int errorB_val = oldB - newB;
            
            // Set new pixel value
            rgbData[idx] = newR;
            rgbData[idx + 1] = newG;
            rgbData[idx + 2] = newB;
            
            // Distribute error to neighboring pixels (Floyd-Steinberg)
            if (x + 1 < width) {
                errorR[x + 1][y] += errorR_val * 7 / 16;
                errorG[x + 1][y] += errorG_val * 7 / 16;
                errorB[x + 1][y] += errorB_val * 7 / 16;
            }
            
            if (x - 1 >= 0 && y + 1 < height) {
                errorR[x - 1][y + 1] += errorR_val * 3 / 16;
                errorG[x - 1][y + 1] += errorG_val * 3 / 16;
                errorB[x - 1][y + 1] += errorB_val * 3 / 16;
            }
            
            if (y + 1 < height) {
                errorR[x][y + 1] += errorR_val * 5 / 16;
                errorG[x][y + 1] += errorG_val * 5 / 16;
                errorB[x][y + 1] += errorB_val * 5 / 16;
            }
            
            if (x + 1 < width && y + 1 < height) {
                errorR[x + 1][y + 1] += errorR_val * 1 / 16;
                errorG[x + 1][y + 1] += errorG_val * 1 / 16;
                errorB[x + 1][y + 1] += errorB_val * 1 / 16;
            }
        }
    }
}

wxRect BitmapData::findCharacterRegion(const std::shared_ptr<wxImage>& image, int x, int y) {
    if (!image || !image->IsOk()) {
        return wxRect();
    }
    int width = image->GetWidth();
    int height = image->GetHeight();
    // Always use BOUNDARY_COLOR and SEPARATOR_COLOR from CommonTypes.h
    uint8_t boundaryR = red(BOUNDARY_COLOR);
    uint8_t boundaryG = green(BOUNDARY_COLOR);
    uint8_t boundaryB = blue(BOUNDARY_COLOR);
    uint8_t sepR = red(SEPARATOR_COLOR);
    uint8_t sepG = green(SEPARATOR_COLOR);
    uint8_t sepB = blue(SEPARATOR_COLOR);
    auto isBoundary = [&](int px, int py) -> bool {
        if (px < 0 || py < 0 || px >= width || py >= height) return false;
        uint8_t r = image->GetRed(px, py);
        uint8_t g = image->GetGreen(px, py);
        uint8_t b = image->GetBlue(px, py);
        return (r == boundaryR && g == boundaryG && b == boundaryB);
    };
    auto isBoundaryOrSeparator = [&](int px, int py) -> bool {
        if (px < 0 || py < 0 || px >= width || py >= height) return false;
        uint8_t r = image->GetRed(px, py);
        uint8_t g = image->GetGreen(px, py);
        uint8_t b = image->GetBlue(px, py);
        return ((r == boundaryR && g == boundaryG && b == boundaryB) || (r == sepR && g == sepG && b == sepB));
    };
    // Find top-left corner: scan up and left from (x, y) until hitting boundary or separator
    int left = x, top = y;
    while (left > 0 && !isBoundaryOrSeparator(left - 1, y)) --left;
    while (top > 0 && !isBoundaryOrSeparator(x, top - 1)) --top;
    // Now find right edge: scan right until hitting boundary (not separator)
    int right = x;
    while (right < width - 1 && !isBoundary(right + 1, y)) ++right;
    // Now find bottom edge: scan down until hitting boundary (not separator)
    int bottom = y;
    while (bottom < height - 1 && !isBoundary(x, bottom + 1)) ++bottom;
    // Validate region
    if (left >= right || top >= bottom) {
        // No valid region found, return whole image
        return wxRect(0, 0, width, height);
    }
    return wxRect(left, top, right - left + 1, bottom - top + 1);
}