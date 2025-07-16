#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <wx/string.h>
#include <wx/statbmp.h>
#include "CommonTypes.h"

class VideoData {
public:
    static const uint16_t FLI_MAGIC_NUMBER = 0xAF11;
    static const uint16_t FLC_MAGIC_NUMBER = 0xAF12;
    static const uint16_t FLI_FRAME_MAGIC_NUMBER = 0xF1FA;

    int width = 0;           // Width of the animation
    int height = 0;          // Height of the animation
    int frameCount = 0;      // Number of frames
    int frameRate = 0;       // Frames per second
    int frameDelay = 0;      // Frame delay in milliseconds
    int durationMs = 0;      // Duration in milliseconds
    int offsetFrame1 = 0;    // Offset of frame 1
    int offsetFrame2 = 0;    // Offset of frame 2
    std::vector<uint8_t> data; // Raw FLIC animation data
    ObjectType typeID = ObjectType::DAT_FLI; // Original type ID from parser (DAT_FLI)

    // Helper function to check if format is valid
    bool isValidFormat() const;

    // Parse FLIC data from buffer
    static bool parse(const std::vector<uint8_t>& dataBuffer, ObjectType typeID, VideoData& outVideo);

    // Create a sample empty FLI animation
    static VideoData createSampleFLI();

    // Serialize video data to raw buffer
    std::vector<uint8_t> serialize() const;

    // Get video duration in milliseconds
    int getDurationMs() const;

    // Compare video data with the content of a file
    bool compareWithFile(const std::string& filepath) const;

    // import video data from a file
    bool importFromFile(const std::string& filepath);

    // Equality operator
    bool operator==(const VideoData& other) const {
        return data.size() == other.data.size() &&
               std::memcmp(data.data(), other.data.data(), data.size()) == 0;
    }

    bool operator!=(const VideoData& other) const {
        return !(*this == other);
    }

    // Get a descriptive caption for preview display
    wxString getPreviewCaption() const;

    // Get a vector of frames as wxImages
    std::vector<wxImage> getFrameArray(int32_t requestedFrameCount = -1) const;

    wxImage getPreviewFrame() const{
        if (frameCount == 0) return wxImage();
        return getFrameArray(1)[0];
    }

private:
    size_t readChunk(size_t offset, std::vector<uint8_t>& outPixels, std::vector<uint8_t>& colormap) const;
}; 