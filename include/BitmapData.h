#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <unordered_map>
#include <wx/image.h>
#include "CommonTypes.h"

class BitmapData {
public:
    int bits;           // Color depth (8, 15, 16, 24, 32, -32)
    int width;          // Image width
    int height;         // Image height
    std::vector<uint8_t> data;  // Raw image data
    std::vector<uint8_t> alpha; // Alpha channel data
    ObjectType typeID = ObjectType::DAT_BITMAP;  // Original type ID from parser (e.g., DAT_BITMAP, DAT_PALETTE)
    static const int paletteScale = 8;
    static const int32_t RLE_EOL_MARKER_32 = 0x00ff00ff;  // End-of-line marker for RLE sprite data for 24-32 bit
    static const int32_t RLE_EOL_MARKER_16 = 0xfffff81f;  // End-of-line marker for RLE sprite data for 15-16 bit
    static const int32_t RLE_EOL_MARKER_8 = 0x00;  // End-of-line marker for RLE sprite data for 8 bit
    static const int32_t RLE_ZERO_COLOR_32 = 0x00ff00ff;  // Zero color for RLE sprite data
    static const int32_t RLE_ZERO_COLOR_16 = 0x0000f81f;  // Zero color for RLE sprite data
    static const int32_t RLE_ZERO_COLOR_8 = 0x00ff00ff;  // Zero color for RLE sprite data for 8 bit
    static std::vector<uint8_t> allegro_palette;  // Allegro default palette

    // Helper function to check if format is valid
    bool isValidFormat() const;

    // Helper function to get bytes per pixel
    int getBytesPerPixel() const;

    // Update alpha channel based on zero color values
    void updateAlphaChannel();

    // Parse bitmap data from raw buffer
    // Returns true if parsing was successful
    static bool parse(const std::vector<uint8_t>& buffer, ObjectType typeID, BitmapData& outBitmap);

    // Serialize bitmap data to raw buffer
    // Returns a buffer containing the serialized bitmap data
    std::vector<uint8_t> serialize(std::vector<uint8_t>& palette = allegro_palette) const;
    
    // Convert bitmap data to RGB format and write directly to provided buffer
    // Returns true if conversion was successful
    bool toRGB(uint8_t* outBuffer, std::vector<uint8_t>& palette = allegro_palette) const;

    // Convert bitmap data to wxImage view
    bool toWxImage(wxImage& outImage, std::vector<uint8_t>& palette = allegro_palette, bool PreserveTransparency = false) const;
    
    // Compare bitmap data with the content of a file
    // Returns true if the bitmap data equals the file content
    bool compareWithFile(const std::string& filepath, std::vector<uint8_t>* currentPalette = nullptr) const;

    // Compare bitmap data with a wxImage
    // Returns true if the bitmap data equals the wxImage content
    bool compareWithWxImage(const wxImage& image) const;

    // Equality operator
    bool operator==(const BitmapData& other) const {
        return bits == other.bits &&
               width == other.width &&
               height == other.height &&
               data.size() == other.data.size() &&
               std::memcmp(data.data(), other.data.data(), data.size()) == 0;
    }

    bool operator!=(const BitmapData& other) const {
        return !(*this == other);
    }
    // load to this BitmapData from wxImage, non static
    bool loadFromWxImage(const wxImage& image, int bits, std::vector<uint8_t>* currentPalette = nullptr, bool useDithering = false, bool preserveTransparency = false);

    // import BitmapData from a file
    bool importFromFile(const std::string& filepath, std::vector<uint8_t>* currentPalette = nullptr, bool useDithering = false, bool preserveTransparency = false);

    // Get a descriptive caption for preview display
    wxString getPreviewCaption() const;

    bool isPalette() const;

    // Check if the bitmap contains only one color (or is empty)
    bool isMonocolor() const;

    // Autocrop the bitmap by removing empty borders
    // Returns true if cropping was performed, false if no cropping was needed
    bool autoCrop(int& xCrop, int& yCrop);

    static bool ReadPCXFile(const wxString& filename, wxImage& image);
    
    // Generate an optimal palette for the bitmap
    static bool generateOptimalPalette(const wxImage& image, std::vector<uint8_t>& palette);
    
    // Extract alpha channel from a wxImage into a vector of bytes
    // Returns true if successful, false if the image has no alpha channel
    static bool extractAlphaChannel(const wxImage& image, std::vector<uint8_t>& outAlpha);
    
    // Create a BitmapData object from a palette vector
    // palette: Input vector containing 256 RGB colors (768 bytes)
    // bitmap: Output BitmapData object to initialize
    // Returns true if successful, false if palette size is invalid
    static bool createFromPalette(const std::vector<uint8_t>& palette, BitmapData& bitmap);

    // Read any supported file format into a wxImage
    static bool readFileToWxImage(const wxString& filepath, wxImage& image);

    // Write wxImage to a PCX file
    static bool WritePCXFile(const wxString& filepath, const wxImage& image);

    // struct to store grid cell information
    struct GridCell {
        int x, y, width, height;
        wxImage image;
    };

    // Split image into smaller images using a specified color as grid marker
    // Returns a vector of GridCell objects representing the grid cells
    static std::vector<GridCell> gridByColor(const wxImage& image, const wxColour& gridColor);

    // Split image into smaller images using grid cell size
    static std::vector<GridCell> gridBySize(const wxImage& image, int cellWidth, int cellHeight);

    // Calculate how well a palette represents an image's colors
    // Returns a value between 0 and 1, where:
    // 1 means the palette is a perfect match (all colors in the image are in the palette)
    // 0 means the palette is a poor match (many colors in the image are not in the palette)
    static double calculatePaletteMatch(const wxImage& image, const std::vector<uint8_t>& palette);

    // Overloaded version that also returns a map of RGB colors to their best palette indices
    // colorMap: Output parameter that maps RGB colors (as uint32_t) to palette indices
    static double calculatePaletteMatch(const wxImage& image, const std::vector<uint8_t>& palette, 
                                      std::unordered_map<uint32_t, int>& colorMap);

    static bool compareRGBwithColor(const uint8_t* rgb, const int32_t color, int bits = 24);

    // Create a 32x32 bitmap with black background and blue 'Hi!' text
    static BitmapData createSampleBitmap(ObjectType typeID);

    // Utility methods
    // Check if the bitmap has alpha byte data
    bool hasAlphaData() const;

    // Return the alpha channel as a grayscale wxImage
    wxImage getAlphaDataAsImage() const;

    // Delete alpha channel if present (bits == -32)
    void deleteAlphaData();

    // Import alpha data from a wxImage using brightness values
    // Returns true if successful, false if the image dimensions don't match
    bool importAlphaDataFromImage(const wxImage& image);

    // Convert bitmap data to a different color depth
    // Returns true if conversion was successful, false if the target depth is not supported
    bool setColorDepth(int newBits, std::vector<uint8_t>& palette = allegro_palette, bool useDithering = false, bool preserveTransparency = false);

    // Convert this BitmapData to a different Allegro bitmap type (bitmap, RLE sprite, compiled sprite, etc)
    // Returns true if conversion was successful, false otherwise
    bool setType(ObjectType newType, bool useDithering = false);

    // Dithering function for color reduction
    static void applyDithering(wxImage& image, const std::vector<uint8_t>& palette, int bits = 8);

    // Find a character region in the image using BOUNDARY_COLOR as the boundary color.
    // Scans from (x, y) to find the top-left, right, and bottom edges of a character region.
    // Returns wxRect of the found region, or the whole image if not found.
    static wxRect findCharacterRegion(const std::shared_ptr<wxImage>& image, int x, int y);

};