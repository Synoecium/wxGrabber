#include "../include/VideoData.h"
#include <cstdint>
#include <fstream>
#include <cstring>
#include <filesystem>
#include "../include/log.h"
#include <wx/image.h>
#include <wx/bitmap.h>

bool VideoData::isValidFormat() const {
    return width > 0 && height > 0 && frameCount > 0 && frameRate > 0 && !data.empty();
}

int VideoData::getDurationMs() const {
    return durationMs;
}

bool VideoData::parse(const std::vector<uint8_t>& dataBuffer, ObjectType typeID, VideoData& outVideo) {
    outVideo.typeID = typeID;
    if (typeID == ObjectType::DAT_FLI) {
        // Minimal FLIC header parsing (for .FLI/.FLC)
        /* 4 bytes: file size
           2 bytes: magic number (0xAF11 for FLI, 0xAF12 for FLC)
           2 bytes: frames
           2 bytes: width
           2 bytes: height
           2 bytes: color depth
           2 bytes: flags
           4 bytes: speed
        */
        if (dataBuffer.size() < 128) {
            logError("FLIC data too small");
            return false;
        }
        // Parse FLIC header fields
        uint32_t fileSize = dataBuffer[0] | (dataBuffer[1] << 8) | (dataBuffer[2] << 16) | (dataBuffer[3] << 24);
        uint16_t magic = dataBuffer[4] | (dataBuffer[5] << 8);
        uint16_t frames = dataBuffer[6] | (dataBuffer[7] << 8);
        uint16_t width = dataBuffer[8] | (dataBuffer[9] << 8);
        uint16_t height = dataBuffer[10] | (dataBuffer[11] << 8);
        uint16_t colorDepth = dataBuffer[12] | (dataBuffer[13] << 8);
        uint16_t flags = dataBuffer[14] | (dataBuffer[15] << 8);
        uint32_t speed = dataBuffer[16] | (dataBuffer[17] << 8) | (dataBuffer[18] << 16) | (dataBuffer[19] << 24);
        logDebug("FLIC header: fileSize=" + std::to_string(fileSize) + ", type=" + (magic == FLI_MAGIC_NUMBER ? "FLI" : "FLC") + ", frames=" + std::to_string(frames) + ", width=" + std::to_string(width) + ", height=" + std::to_string(height) + ", colorDepth=" + std::to_string(colorDepth) + ", flags=0x" + std::to_string(flags) + ", speed=" + std::to_string(speed));
        outVideo.width = width;
        outVideo.height = height;
        outVideo.frameCount = frames;
        outVideo.frameDelay = speed;
        outVideo.data = dataBuffer;
        if (magic != FLI_MAGIC_NUMBER && magic != FLC_MAGIC_NUMBER) {
            logError("Invalid FLIC magic number: 0x" + std::to_string(magic));
            return false;
        }
        //adjust speed
        if (speed == 0)
        {
            outVideo.frameRate = 70;
        }
        else
        {
            if (magic == FLI_MAGIC_NUMBER) {
                outVideo.frameRate = 1000 * speed / 70;
            }
            else if (magic == FLC_MAGIC_NUMBER) 
                outVideo.frameRate = 1000 / speed;
        }
        outVideo.durationMs = (outVideo.frameCount * 1000) / outVideo.frameRate;
        std::stringstream ss;
        ss << "Duration, sec: " << outVideo.durationMs / 1000.0;
        // get offset of frame 1 and frame 2
        if (magic == FLC_MAGIC_NUMBER)
        {
            outVideo.offsetFrame1 = dataBuffer[80] | (dataBuffer[81] << 8) | (dataBuffer[82] << 16) | (dataBuffer[83] << 24);
            outVideo.offsetFrame2 = dataBuffer[84] | (dataBuffer[85] << 8) | (dataBuffer[86] << 16) | (dataBuffer[87] << 24);
        }
        
        return true;
    } else {
        logError("Invalid typeID for VideoData::parse: " + std::to_string(static_cast<int>(typeID)));
        return false;
    }
}

std::vector<uint8_t> VideoData::serialize() const {
    if (!isValidFormat()) {
        return std::vector<uint8_t>();
    }
    return data;
}

bool VideoData::compareWithFile(const std::string& filepath) const {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return false;
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    return data.size() == buffer.size() && std::memcmp(data.data(), buffer.data(), data.size()) == 0;
}

bool VideoData::importFromFile(const std::string& filepath) {
    // Open file
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return false;
    }

    // Get file size
    size_t fileSize = std::filesystem::file_size(filepath);

    // Read file into buffer
    std::vector<uint8_t> buffer(fileSize);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return false;
    }
    return parse(buffer, typeID, *this);
}

wxString VideoData::getPreviewCaption() const {
    return wxString::Format("FLIC Animation: %dx%d, %d frames, %.2f sec", width, height, frameCount, durationMs/1000.0); // duration in seconds, 2 decimal places
}

std::vector<wxImage> VideoData::getFrameArray(int32_t requestedFrameCount) const {
    std::vector<wxImage> frames;
    std::vector<uint8_t> colormap(256*3, 0);
    size_t frameOffset = 128; // Default for FLI
    if (requestedFrameCount == -1) {
        requestedFrameCount = frameCount;
    }
    std::vector<uint8_t> pixels(width * height);
    for (int i = 0; i < requestedFrameCount; ++i) {
        if (i == 0 && offsetFrame1) {
            frameOffset = offsetFrame1;
        }
        if (i == 1 && offsetFrame2) {
            frameOffset = offsetFrame2;
        }
        if (data.size() < frameOffset + 16) {
            return frames;
        }
        // Frame header: 16 bytes
        uint32_t frameSize = data[frameOffset+0] | (data[frameOffset+1]<<8) | (data[frameOffset+2]<<16) | (data[frameOffset+3]<<24);
        uint16_t frameMagic = data[frameOffset+4] | (data[frameOffset+5]<<8);
        std::stringstream ss;
        ss << std::hex << std::uppercase << frameMagic;
        if (frameMagic != FLI_FRAME_MAGIC_NUMBER) {
            return frames;
        }
        uint16_t chunks = data[frameOffset+6] | (data[frameOffset+7]<<8);
        // skip padding 8 bytes
        size_t chunkOffset = frameOffset + 16;
        for (int j = 0; j < chunks; j++) {
            size_t chunkSize = readChunk(chunkOffset, pixels, colormap);
            chunkOffset += chunkSize;
        }
        // convert pixels to wxImage
        wxImage img(width, height);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                uint8_t pixelValue = pixels[y*width+x];
                uint8_t r = colormap[pixelValue*3];
                uint8_t g = colormap[pixelValue*3+1];
                uint8_t b = colormap[pixelValue*3+2];
                img.SetRGB(x, y, r, g, b);
            }
        }
        
        frames.push_back(img);
        frameOffset += frameSize;
    }
    return frames;
}

namespace {
// FLIC chunk type constants as constexpr
static constexpr uint16_t FLI_COLOR_256_CHUNK  = 4;
static constexpr uint16_t FLI_DELTA_CHUNK      = 7;
static constexpr uint16_t FLI_COLOR_64_CHUNK   = 11;
static constexpr uint16_t FLI_LC_CHUNK         = 12;
static constexpr uint16_t FLI_BLACK_CHUNK      = 13;
static constexpr uint16_t FLI_BRUN_CHUNK       = 15;
static constexpr uint16_t FLI_COPY_CHUNK       = 16;

// Helper functions for little-endian reads
inline uint32_t read32(const std::vector<uint8_t>& buf, size_t& pos) {
    uint32_t v = buf[pos] | (buf[pos+1]<<8) | (buf[pos+2]<<16) | (buf[pos+3]<<24);
    pos += 4;
    return v;
}
inline uint16_t read16(const std::vector<uint8_t>& buf, size_t& pos) {
    uint16_t v = buf[pos] | (buf[pos+1]<<8);
    pos += 2;
    return v;
}
inline int8_t read8s(const std::vector<uint8_t>& buf, size_t& pos) {
    return static_cast<int8_t>(buf[pos++]);
}
inline uint8_t read8(const std::vector<uint8_t>& buf, size_t& pos) {
    return buf[pos++];
}

// Returns a string representation of a FLIC chunk type
std::string getChunkTypeString(uint16_t chunkType) {
    switch (chunkType) {
        case FLI_COLOR_256_CHUNK:  return "FLI_COLOR_256_CHUNK (4)";
        case FLI_DELTA_CHUNK:      return "FLI_DELTA_CHUNK (7)";
        case FLI_COLOR_64_CHUNK:   return "FLI_COLOR_64_CHUNK (11)";
        case FLI_LC_CHUNK:         return "FLI_LC_CHUNK (12)";
        case FLI_BLACK_CHUNK:      return "FLI_BLACK_CHUNK (13)";
        case FLI_BRUN_CHUNK:       return "FLI_BRUN_CHUNK (15)";
        case FLI_COPY_CHUNK:       return "FLI_COPY_CHUNK (16)";
        default: {
            char buf[32];
            snprintf(buf, sizeof(buf), "Unknown (0x%04X - %d)", chunkType, chunkType);
            return std::string(buf);
        }
    }
}

// Chunk Handlers
void readColorChunk(const std::vector<uint8_t>& data, size_t& pos, std::vector<uint8_t>& colormap, bool oldColorChunk) {
    int npackets = read16(data, pos);
    int i = 0;
    while (npackets--) {
        i += read8(data, pos); // Colors to skip
        int colors = read8(data, pos);
        if (colors == 0) colors = 256;
        for (int j = 0; j < colors && i + j < 256; ++j) {
            uint8_t r = read8(data, pos);
            uint8_t g = read8(data, pos);
            uint8_t b = read8(data, pos);
            if (oldColorChunk) {
                r = 255 * int(r) / 63;
                g = 255 * int(g) / 63;
                b = 255 * int(b) / 63;
            }
            colormap[(i + j) * 3 + 0] = r;
            colormap[(i + j) * 3 + 1] = g;
            colormap[(i + j) * 3 + 2] = b;
        }
        i += colors;
    }
}

void readDeltaChunk(const std::vector<uint8_t>& data, size_t& pos, std::vector<uint8_t>& pixels, int width, int height) {
    int nlines = read16(data, pos);
    int y = 0;
    while (nlines-- != 0) {
        int npackets = 0;
        while (pos + 2 <= data.size()) {
            int16_t word = read16(data, pos);
            if (word < 0) {
                if (word & 0x4000) {
                    y += -word;
                } else {
                    if (y >= 0 && y < height) {
                        size_t idx = y * width + width - 1;
                        if (idx < pixels.size())
                            pixels[idx] = word & 0xff;
                    }
                    ++y;
                    if (nlines-- == 0) return;
                }
            } else {
                npackets = word;
                break;
            }
        }
        if (y >= height) break;
        int x = 0;
        while (npackets-- != 0) {
            x += read8(data, pos);
            int8_t count = read8s(data, pos);
            size_t idx = y * width + x;
            if (count >= 0) {
                while (count-- != 0 && x < width) {
                    int color1 = read8(data, pos);
                    int color2 = read8(data, pos);
                    if (idx < pixels.size()) pixels[idx] = color1;
                    ++idx; ++x;
                    if (x < width && idx < pixels.size()) pixels[idx] = color2;
                    ++idx; ++x;
                }
            } else {
                int color1 = read8(data, pos);
                int color2 = read8(data, pos);
                while (count++ != 0 && x < width) {
                    if (idx < pixels.size()) pixels[idx] = color1;
                    ++idx; ++x;
                    if (x < width && idx < pixels.size()) pixels[idx] = color2;
                    ++idx; ++x;
                }
            }
        }
        ++y;
    }
}

void readLcChunk(const std::vector<uint8_t>& data, size_t& pos, std::vector<uint8_t>& pixels, int width, int height) {
    int skipLines = read16(data, pos);
    int nlines = read16(data, pos);
    for (int y = skipLines; y < skipLines + nlines; ++y) {
        if (y < 0 || y >= height) break;
        size_t row = y * width;
        int x = 0;
        int npackets = read8(data, pos);
        while (npackets-- && x < width) {
            int skip = read8(data, pos);
            x += skip;
            int count = int(read8s(data, pos));
            if (count >= 0) {
                while (count-- && x < width) {
                    pixels[row + x] = read8(data, pos);
                    ++x;
                }
            } else {
                uint8_t color = read8(data, pos);
                while (count++ && x < width) {
                    pixels[row + x] = color;
                    ++x;
                }
            }
        }
    }
}

void readBlackChunk(std::vector<uint8_t>& pixels) {
    std::fill(pixels.begin(), pixels.end(), 0);
}

void readBrunChunk(const std::vector<uint8_t>& data, size_t& pos, std::vector<uint8_t>& pixels, int width, int height) {
    for (int y = 0; y < height; ++y) {
        size_t row = y * width;
        int npackets = read8(data, pos);
        int x = 0;
        while (npackets-- && x < width) {
            int count = int(read8s(data, pos));
            if (count >= 0) {
                uint8_t color = read8(data, pos);
                while (count-- && x < width) {
                    pixels[row + x] = color;
                    ++x;
                }
            } else {
                while (count++ && x < width) {
                    pixels[row + x] = read8(data, pos);
                    ++x;
                }
            }
        }
    }
}

void readCopyChunk(const std::vector<uint8_t>& data, size_t& pos, std::vector<uint8_t>& pixels, int width, int height) {
    for (int y = 0; y < height; ++y) {
        size_t row = y * width;
        for (int x = 0; x < width; ++x) {
            if (row + x < pixels.size() && pos < data.size())
                pixels[row + x] = read8(data, pos);
        }
    }
}
}

size_t VideoData::readChunk(size_t offset, std::vector<uint8_t>& outPixels, std::vector<uint8_t>& colormap) const {
    uint32_t chunkStartPos = offset;
    uint32_t chunkSize = read32(data, offset);
    uint16_t type = read16(data, offset);

    switch (type) {
        case FLI_COLOR_256_CHUNK: readColorChunk(data, offset, colormap, false); break;
        case FLI_DELTA_CHUNK:     readDeltaChunk(data, offset, outPixels, width, height); break;
        case FLI_COLOR_64_CHUNK:  readColorChunk(data, offset, colormap, true); break;
        case FLI_LC_CHUNK:        readLcChunk(data, offset, outPixels, width, height); break;
        case FLI_BLACK_CHUNK:     readBlackChunk(outPixels); break;
        case FLI_BRUN_CHUNK:      readBrunChunk(data, offset, outPixels, width, height); break;
        case FLI_COPY_CHUNK:      readCopyChunk(data, offset, outPixels, width, height); break;
        default:
            // Ignore all other kind of chunks
            break;
    }
    return chunkSize;
}

VideoData VideoData::createSampleFLI() {
    VideoData video;
    
    // Set basic properties for an empty FLI file
    video.width = 320;
    video.height = 200;
    video.frameCount = 0;      // No frames
    video.frameRate = 18;      // 18.2 FPS (standard FLI rate)
    video.frameDelay = 70;     // Delay in milliseconds 
    video.durationMs = 0;      // No duration since no frames
    video.typeID = ObjectType::DAT_FLI;
    
    // Create empty FLI file structure - just the header (128 bytes)
    std::vector<uint8_t> fliData(128, 0);
    
    // File size (just the header)
    uint32_t fileSize = 128;
    fliData[0] = fileSize & 0xFF;
    fliData[1] = (fileSize >> 8) & 0xFF;
    fliData[2] = (fileSize >> 16) & 0xFF;
    fliData[3] = (fileSize >> 24) & 0xFF;
    
    // Magic number (FLI = 0xAF11)
    fliData[4] = 0x11;
    fliData[5] = 0xAF;
    
    // Frame count (0 for empty animation)
    fliData[6] = 0;
    fliData[7] = 0;
    
    // Width and height (standard FLI dimensions)
    fliData[8] = 320 & 0xFF;
    fliData[9] = (320 >> 8) & 0xFF;
    fliData[10] = 200 & 0xFF;
    fliData[11] = (200 >> 8) & 0xFF;
    
    // Color depth (8-bit)
    fliData[12] = 8;
    fliData[13] = 0;
    
    // Flags
    fliData[14] = 0;
    fliData[15] = 0;
    
    // Speed (70 for FLI)
    fliData[16] = 70;
    fliData[17] = 0;
    fliData[18] = 0;
    fliData[19] = 0;
    
    // Rest of header filled with zeros (already done by vector initialization)
    // No frame data since frameCount = 0
    
    video.data = fliData;
    
    return video;
}