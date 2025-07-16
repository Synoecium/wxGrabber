#include "../include/FontData.h"
#include <fstream>
#include <cstring>
#include <wx/font.h>
#include <wx/filename.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/regex.h>
#include <wx/textfile.h>
#include <wx/tokenzr.h>
#include <wx/filefn.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include "../include/log.h"
#include "../include/BitmapData.h"
#include <map>

static uint16_t read16(const std::vector<uint8_t>& buf, size_t& pos, bool littleEndian = false) {
    uint16_t v = buf[pos] | (buf[pos+1] << 8);
    if (littleEndian) {
        v = buf[pos+1] | (buf[pos] << 8);
    }
    pos += 2;
    return v;
}
static uint32_t read32(const std::vector<uint8_t>& buf, size_t& pos, bool littleEndian = false) {
    uint32_t v = buf[pos] | (buf[pos+1] << 8) | (buf[pos+2] << 16) | (buf[pos+3] << 24);
    if (littleEndian) {
        v = buf[pos+3] | (buf[pos+2] << 8) | (buf[pos+1] << 16) | (buf[pos] << 24);
    }
    pos += 4;
    return v;
}
static uint8_t read8(const std::vector<uint8_t>& buf, size_t& pos) {
    return buf[pos++];
}

// Write helper functions
static void write16(std::vector<uint8_t>& buf, uint16_t value, bool littleEndian = false) {
    if (littleEndian) {
        buf.push_back((value >> 8) & 0xFF);
        buf.push_back(value & 0xFF);
    } else {
        buf.push_back(value & 0xFF);
        buf.push_back((value >> 8) & 0xFF);
    }
}

static void write32(std::vector<uint8_t>& buf, uint32_t value, bool littleEndian = false) {
    if (littleEndian) {
        buf.push_back((value >> 24) & 0xFF);
        buf.push_back((value >> 16) & 0xFF);
        buf.push_back((value >> 8) & 0xFF);
        buf.push_back(value & 0xFF);
    } else {
        buf.push_back(value & 0xFF);
        buf.push_back((value >> 8) & 0xFF);
        buf.push_back((value >> 16) & 0xFF);
        buf.push_back((value >> 24) & 0xFF);
    }
}

static void write8(std::vector<uint8_t>& buf, uint8_t value) {
    buf.push_back(value);
}

FontData::FontData() : typeID(ObjectType::DAT_FONT), fontSize(0), glyphCount(0) {}

bool FontData::isValidFormat() const {
    return glyphCount > 0 && !ranges.empty();
}

int FontData::getGlyphCount() const {
    return glyphCount;
}

int FontData::getFontSize() const {
    return fontSize;
}

bool FontData::parse(const std::vector<uint8_t>& dataBuffer, ObjectType typeID, FontData& outFont) {
    outFont.typeID = typeID;
    outFont.glyphCount = 0;
    outFont.fontSize = 0;
    outFont.ranges.clear();
    
    if (dataBuffer.size() < 2) {
        logError("FontData::parse: dataBuffer is too small");
        return false;
    }
    
    size_t pos = 0;
    int16_t fsize = (int16_t)read16(dataBuffer, pos);
    outFont.fontSize = fsize;
    logDebug(wxString::Format("FontData::parse: font size: %d", fsize));
    
    // Handle obsolete 8x8 and 8x16 formats
    if (fsize == 8 || fsize == 16) {
        logDebug("FontData::parse: handling obsolete 8x8 or 8x16 format");
        size_t expected = 2 + 95 * fsize;
        if (dataBuffer.size() < expected) {
            logError("FontData::parse: dataBuffer is too small");
            return false;
        }
        
        // Create a single range for the 95 glyphs (ASCII 32-126)
        Range range;
        range.mono = 1;  // These are monochrome bit-packed fonts
        range.start = 32; // ASCII space character
        range.end = 126;  // ASCII tilde character
        
        // Parse each glyph
        for (int i = 0; i < 95; ++i) {
            Glyph glyph;
            glyph.width = 8;
            glyph.height = fsize;
            
            // Extract glyph data (bit-packed)
            size_t glyphDataSize = fsize; // Each glyph is fsize bytes
            if (pos + glyphDataSize > dataBuffer.size()) {
                logError("FontData::parse: dataBuffer is too small for glyph data");
                return false;
            }
            
            glyph.data = std::vector<uint8_t>(dataBuffer.begin() + pos, dataBuffer.begin() + pos + glyphDataSize);
            pos += glyphDataSize;
            
            range.glyphs.push_back(glyph);
        }
        
        outFont.ranges.push_back(range);
        outFont.glyphCount = 95;
        return true;
    } 
    else if (fsize == -1) {
        logDebug("FontData::parse: handling new format with 95 glyphs");
        
        // Create a single range for the 95 glyphs (ASCII 32-126)
        Range range;
        range.mono = 0;  // These can be grayscale/color
        range.start = 32; // ASCII space character
        range.end = 126;  // ASCII tilde character
        
        // Parse each glyph
        for (int i = 0; i < 95; ++i) {
            if (pos + 4 > dataBuffer.size()) {
                logError("FontData::parse: dataBuffer is too small for glyph header");
                return false;
            }
            
            Glyph glyph;
            glyph.width = read16(dataBuffer, pos, true);
            glyph.height = read16(dataBuffer, pos, true);
            size_t datalen = glyph.width * glyph.height;
            
            if (pos + datalen > dataBuffer.size()) {
                logError("FontData::parse: dataBuffer is too small for glyph data");
                return false;
            }
            
            glyph.data = std::vector<uint8_t>(dataBuffer.begin() + pos, dataBuffer.begin() + pos + datalen);
            pos += datalen;
            
            range.glyphs.push_back(glyph);
        }
        
        outFont.ranges.push_back(range);
        outFont.glyphCount = 95;
        return true;
    } 
    else if (fsize == 0) {
        // New format: ranges
        logDebug("FontData::parse: handling new format with ranges");
        if (pos + 2 > dataBuffer.size()) {
            logError("FontData::parse: dataBuffer is too small for ranges count");
            return false;
        }
        
        uint16_t rangeCount = read16(dataBuffer, pos, true);
        logDebug(wxString::Format("FontData::parse: found %d ranges", rangeCount));
        
        int totalGlyphs = 0;
        for (uint16_t r = 0; r < rangeCount; ++r) {
            if (pos + 1 + 4 + 4 > dataBuffer.size()) {
                logError(wxString::Format("FontData::parse: dataBuffer is too small for range %d", r));
                return false;
            }
            
            Range range;
            range.mono = read8(dataBuffer, pos);
            range.start = read32(dataBuffer, pos, true);
            range.end = read32(dataBuffer, pos, true);
            int glyphsInRange = range.end - range.start + 1;
            logDebug(wxString::Format("FontData::parse: range %d: %d glyphs, start %d, end %d, depth flag %d", r, glyphsInRange, range.start, range.end, range.mono));
            
            for (int g = 0; g < glyphsInRange; ++g) {
                if (pos + 4 > dataBuffer.size()) {
                    logError(wxString::Format("FontData::parse: dataBuffer is too small for range %d, glyph %d header", r, g));
                    return false;
                }
                
                Glyph glyph;
                glyph.width = read16(dataBuffer, pos, true);
                glyph.height = read16(dataBuffer, pos, true);
                int bitDepth = range.getBitDepth();
                int bytesPerPixel = bitDepth / 8;
                size_t datalen = glyph.width * glyph.height * bytesPerPixel;
                if (bitDepth == 1) {
                    size_t bytesPerRow = (glyph.width + 7) / 8;
                    datalen = bytesPerRow * glyph.height;
                }
                
                if (pos + datalen > dataBuffer.size()) {
                    logError(wxString::Format("FontData::parse: dataBuffer is too small for range %d, glyph %d data", r, g));
                    return false;
                }
                
                glyph.data = std::vector<uint8_t>(dataBuffer.begin() + pos, dataBuffer.begin() + pos + datalen);
                pos += datalen;
                range.glyphs.push_back(glyph);
            }
            
            totalGlyphs += glyphsInRange;
            outFont.ranges.push_back(range);
        }
        
        outFont.glyphCount = totalGlyphs;
        return true;
    }
    
    // Unknown format
    logError("FontData::parse: unknown format");
    return false;
}

std::vector<uint8_t> FontData::serialize() const {
    if (!isValidFormat()) {
        return std::vector<uint8_t>();
    }
    
    std::vector<uint8_t> buffer;
    
    // Write font size (0 for new range format)
    write16(buffer, 0, true); // little endian
    
    // Write range count
    write16(buffer, static_cast<uint16_t>(ranges.size()), true);
    
    // Write each range
    for (const auto& range : ranges) {
        // Write mono flag
        write8(buffer, range.mono);
        
        // Write start and end codepoints
        write32(buffer, range.start, true);
        write32(buffer, range.end, true);
        
        // Write each glyph in the range
        for (const auto& glyph : range.glyphs) {
            // Write glyph dimensions
            write16(buffer, static_cast<uint16_t>(glyph.width), true);
            write16(buffer, static_cast<uint16_t>(glyph.height), true);
            
            // Write glyph data
            buffer.insert(buffer.end(), glyph.data.begin(), glyph.data.end());
        }
    }
    
    return buffer;
}

bool FontData::compareWithFile(const std::string& filepath) const {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return false;
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    std::vector<uint8_t> serialized = serialize();
    return serialized.size() == buffer.size() && std::memcmp(serialized.data(), buffer.data(), serialized.size()) == 0;
}

bool FontData::importFromFile(const std::string& filepath) {
    if (ranges.empty()) {
        logError("No existing ranges to update in FontData.");
        return false;
    }

    wxString wxFilepath(filepath);
    wxString extension = wxFileName(wxFilepath).GetExt().Lower();

    bool isFntFile = extension == "fnt";

    // Group ranges by mono (color format)
    std::map<int8_t, std::vector<Range*>> monoGroups;
    for (auto& range : ranges) {
        monoGroups[range.mono].push_back(&range);
    }

    // Import once per unique mono
    std::map<int8_t, FontData::Range> importedRanges;
    for (const auto& [mono, group] : monoGroups) {
        if (isFntFile) {
            importedRanges[mono] = FontData::ImportFntAsRange(wxFilepath, 0);
        } else {
            importedRanges[mono] = FontData::ImportBitmapAsRange(wxFilepath, 0, mono);
        }
        if (importedRanges[mono].glyphs.empty()) {
            logError(wxString::Format("Failed to import glyphs for mono=%d from file: %s", (int)mono, filepath).ToStdString());
        }
    }

    bool allOk = true;
    size_t glyphIndex = 0;
    for (auto& range : ranges) {
        auto* rangePtr = &range;
        auto importedRange = importedRanges[range.mono];

        size_t expectedCount = rangePtr->end - rangePtr->start + 1;
        size_t available = importedRange.glyphs.size() - glyphIndex;
        size_t count = std::min(expectedCount, available);

        rangePtr->glyphs.assign(
            importedRange.glyphs.begin() + glyphIndex,
            importedRange.glyphs.begin() + glyphIndex + count
        );
        glyphIndex += count;

        if (count < expectedCount) {
            logWarning(wxString::Format(
                "Range U+%04X-U+%04X only partially updated: %zu of %zu glyphs available",
                rangePtr->start, rangePtr->end, count, expectedCount
            ).ToStdString());
            allOk = false;
        } else {
            logInfo(wxString::Format(
                "Updated %zu glyphs for range U+%04X-U+%04X (mono=%d)",
                count, rangePtr->start, rangePtr->end, (int)rangePtr->mono
            ).ToStdString());
        }

        if (glyphIndex >= importedRange.glyphs.size()) {
            // No more glyphs to distribute
            break;
        }
    }
    
    UpdateGlyphCount();
    return allOk;
}

wxString FontData::getPreviewCaption() const {
    // The screenshot format: 'mono font, 4 ranges, 320 glyphs'
    int maxBitDepth = getMaxBitDepth();
    wxString monoString = getColorDepthString(maxBitDepth);
    return wxString::Format("%s font, %d ranges, %d glyphs, format %s", monoString, (int)ranges.size(), glyphCount, getFontFormatString());
}

wxFont FontData::getWxFont() const {
    // Return a wxFont object for preview (dummy implementation)
    return wxFont(fontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
} 

wxString FontData::getFontFormatString() const {
    if (fontSize == 0) {
        return wxString::Format("dynamic");
    } else if (fontSize == -1) {
        return wxString::Format("dynamic");
    } else if (fontSize == 8) {
        return wxString::Format("8x8");
    } else if (fontSize == 16) {
        return wxString::Format("16x16");
    }
    return wxString::Format("unknown format, size %d", fontSize);
}

wxImage FontData::getPreviewImage() const {
    // Parameters for preview
    const int glyphsPerRow = 31; // 31 columns as in screenshot
    const int margin = 2; // margin between glyphs
    const int fgBlack = 0;  // glyph color

    if (!isValidFormat() || ranges.empty()) {
        return wxImage();
    }
    
    // Find max glyph width/height for layout
    int glyphW = 0, glyphH = 0;
    for (const auto& range : ranges) {
        for (const auto& glyph : range.glyphs) {
            if ((int)glyph.width > glyphW) glyphW = glyph.width;
            if ((int)glyph.height > glyphH) glyphH = glyph.height;
        }
    }
    
    // Calculate image dimensions
    int rows = (glyphCount + glyphsPerRow - 1) / glyphsPerRow;
    int imgW = glyphsPerRow * (glyphW + margin) + margin;
    int imgH = rows * (glyphH + margin) + margin;
    
    // Create transparent image
    wxImage img(imgW, imgH);
    img.InitAlpha();
    memset(img.GetAlpha(), 0, imgW * imgH); // fully transparent background
    logDebug("FontData::getPreviewImage: created transparent image");
    
    // Render all glyphs
    int glyphIdx = 0;
    for (const auto& range : ranges) {
        for (const auto& glyph : range.glyphs) {
            int gx = glyphIdx % glyphsPerRow;
            int gy = glyphIdx / glyphsPerRow;
            int x0 = margin + gx * (glyphW + margin);
            int y0 = margin + gy * (glyphH + margin);
            
            // Use the helper method to render glyph pixels
            range.renderGlyphPixels(glyph, img, x0, y0, fontSize);
            ++glyphIdx;
        }
    }
    
    return img;
}

wxImage FontData::renderGlyph(uint32_t codePoint) const {
    // Search through all ranges to find the one containing this code point
    for (const auto& range : ranges) {
        if (codePoint >= range.start && codePoint <= range.end) {
            // Calculate the index within this range
            uint32_t index = codePoint - range.start;
            return range.renderGlyph(index);
        }
    }
    
    // Code point not found in any range
    return wxImage(); // Return empty image
}

int FontData::getMaxBitDepth() const {
    int maxBitDepth = 0;
    for (const auto& range : ranges) {
        int bitDepth = range.getBitDepth();
        if (bitDepth > maxBitDepth) {
            maxBitDepth = bitDepth;
        }
    }
    return maxBitDepth;
}

void FontData::UpdateGlyphCount() {
    glyphCount = 0;
    for (const auto& range : ranges) {
        glyphCount += range.end - range.start + 1;
    }
}

bool FontData::CheckRangeOverlap(const FontData::Range& newRange, std::vector<wxString>& overlappingRanges) const {
    for (const auto& range : ranges) {
        if (newRange.start <= range.end && newRange.end >= range.start) {
            overlappingRanges.push_back(FontData::formatUnicodeRange(range.start, range.end));
        }
    }
    return overlappingRanges.empty();
}

int FontData::getBitDepthFromMono(int8_t mono) {
    if (mono == 1 || mono == -1) {
        return 1;   // monochrome
    } else if (mono == 0 || mono == 8) {
        return 8;   // 1 byte per pixel
    } else if (mono == 15 || mono == 16) {
        return 16;   // 2 bytes per pixel
    } else if (mono == 24 || mono == 32) {
        return 24;   // 3 bytes per pixel
    } else if (mono == -32) {
        return 32;   // 4 bytes per pixel
    }
    return 0;   // unknown
}

wxString FontData::getColorDepthString(int bitDepth) {
    switch (bitDepth) {
        case 1:
            return "mono";
        case 8:
            return "color (indexed)";
        case 16:
            return "color (high color)";
        case 24:
            return "color (true color)";
        case 32:
            return "color (true color with alpha)";
        default:
            return "unknown";
    }
}

void FontData::Range::renderGlyphPixels(const Glyph& glyph, wxImage& img, int offsetX, int offsetY, int fontSize) const {
    unsigned char* rgb = img.GetData();
    unsigned char* alpha = img.GetAlpha();
    int imgWidth = img.GetWidth();
    int bitDepth = getBitDepth();
    int bytesPerPixel = bitDepth / 8;
    
    if (bitDepth == 1) {
        // Monochrome, bit-packed
        size_t bit = 0;
        size_t bytesPerRow = (glyph.width + 7) / 8;
        for (int y = 0; y < glyph.height; ++y) {
            bit = y * bytesPerRow * 8;
            for (int x = 0; x < glyph.width; ++x, ++bit) {
                size_t byteIdx = bit / 8;
                if (byteIdx < glyph.data.size()) {
                    uint8_t b = glyph.data[byteIdx];
                    if (b & (1 << (7 - (bit % 8)))) {
                        int px = offsetX + x;
                        int py = offsetY + y;
                        if (px >= 0 && py >= 0 && px < img.GetWidth() && py < img.GetHeight()) {
                            size_t pixelIdx = py * imgWidth + px;
                            rgb[pixelIdx * 3] = 0;     // R - black
                            rgb[pixelIdx * 3 + 1] = 0; // G - black
                            rgb[pixelIdx * 3 + 2] = 0; // B - black
                            alpha[pixelIdx] = 255;     // Opaque
                        }
                    }
                }
            }
        }
    } else if (bitDepth == 8) {
        // Grayscale or color with palette
        std::vector<uint8_t> palette = BitmapData::allegro_palette;
        for (int y = 0; y < glyph.height; ++y) {
            for (int x = 0; x < glyph.width; ++x) {
                size_t idx = y * glyph.width + x;
                if (idx < glyph.data.size()) {
                    uint8_t v = glyph.data[idx];
                    if (v != TRANSPARENT_COLOR_INDEX) { // Not transparent
                        int px = offsetX + x;
                        int py = offsetY + y;
                        if (px >= 0 && py >= 0 && px < img.GetWidth() && py < img.GetHeight()) {
                            size_t pixelIdx = py * imgWidth + px;
                            rgb[pixelIdx * 3] = palette[v*3];     // R
                            rgb[pixelIdx * 3 + 1] = palette[v*3+1]; // G
                            rgb[pixelIdx * 3 + 2] = palette[v*3+2]; // B
                            alpha[pixelIdx] = 255;         // Opaque
                        }
                    }
                }
            }
        }
    } else if (bitDepth == 16) {
        // 16-bit color (RGB565 or RGB555 format)
        for (int y = 0; y < glyph.height; ++y) {
            for (int x = 0; x < glyph.width; ++x) {
                size_t idx = (y * glyph.width + x) * 2; // 2 bytes per pixel
                if (idx + 1 < glyph.data.size()) {
                    // Read 16-bit value (little endian)
                    uint16_t pixel = glyph.data[idx] | (glyph.data[idx + 1] << 8);
                    
                    // Check for transparency
                    if (pixel != TRANSPARENT_COLOR_16) {
                        int px = offsetX + x;
                        int py = offsetY + y;
                        if (px >= 0 && py >= 0 && px < img.GetWidth() && py < img.GetHeight()) {
                            size_t pixelIdx = py * imgWidth + px;
                            
                            // Convert RGB565 to RGB888
                            uint8_t r = ((pixel >> 11) & 0x1F) << 3; // 5 bits -> 8 bits
                            uint8_t g = ((pixel >> 5) & 0x3F) << 2;  // 6 bits -> 8 bits  
                            uint8_t b = (pixel & 0x1F) << 3;        // 5 bits -> 8 bits
                            
                            rgb[pixelIdx * 3] = r;
                            rgb[pixelIdx * 3 + 1] = g;
                            rgb[pixelIdx * 3 + 2] = b;
                            alpha[pixelIdx] = 255; // Opaque
                        }
                    }
                }
            }
        }
    } else if (bitDepth == 24) {
        // 24-bit color (RGB888 format)
        for (int y = 0; y < glyph.height; ++y) {
            for (int x = 0; x < glyph.width; ++x) {
                size_t idx = (y * glyph.width + x) * 3; // 3 bytes per pixel
                if (idx + 2 < glyph.data.size()) {
                    uint8_t r = glyph.data[idx];
                    uint8_t g = glyph.data[idx + 1];
                    uint8_t b = glyph.data[idx + 2];
                    
                    // Check for transparency (assuming bright pink is transparent)
                    uint32_t pixelColor = (r << 16) | (g << 8) | b;
                    if (pixelColor != TRANSPARENT_COLOR) {
                        int px = offsetX + x;
                        int py = offsetY + y;
                        if (px >= 0 && py >= 0 && px < img.GetWidth() && py < img.GetHeight()) {
                            size_t pixelIdx = py * imgWidth + px;
                            rgb[pixelIdx * 3] = r;
                            rgb[pixelIdx * 3 + 1] = g;
                            rgb[pixelIdx * 3 + 2] = b;
                            alpha[pixelIdx] = 255; // Opaque
                        }
                    }
                }
            }
        }
    } else if (bitDepth == 32) {
        // 32-bit color (RGBA8888 format)
        for (int y = 0; y < glyph.height; ++y) {
            for (int x = 0; x < glyph.width; ++x) {
                size_t idx = (y * glyph.width + x) * 4; // 4 bytes per pixel
                if (idx + 3 < glyph.data.size()) {
                    uint8_t r = glyph.data[idx];
                    uint8_t g = glyph.data[idx + 1];
                    uint8_t b = glyph.data[idx + 2];
                    uint8_t a = glyph.data[idx + 3];
                    
                    // Use alpha channel for transparency
                    if (a > 0) {
                        int px = offsetX + x;
                        int py = offsetY + y;
                        if (px >= 0 && py >= 0 && px < img.GetWidth() && py < img.GetHeight()) {
                            size_t pixelIdx = py * imgWidth + px;
                            rgb[pixelIdx * 3] = r;
                            rgb[pixelIdx * 3 + 1] = g;
                            rgb[pixelIdx * 3 + 2] = b;
                            alpha[pixelIdx] = a; // Use original alpha
                        }
                    }
                }
            }
        }
    }
}

int FontData::Range::getBitDepth() const {
    return FontData::getBitDepthFromMono(mono);
}

wxImage FontData::Range::renderGlyph(uint32_t index) const
{
    // Check if index is valid
    if (index >= glyphs.size()) {
        return wxImage(); // Return empty image for invalid index
    }
    
    const Glyph& glyph = glyphs[index];
    
    // Check if glyph has valid dimensions
    if (glyph.width == 0 || glyph.height == 0 || glyph.data.empty()) {
        return wxImage(); // Return empty image for invalid glyph
    }
    
    // Create image with glyph dimensions
    wxImage img(glyph.width, glyph.height);
    
    // Fill with transparent background
    img.SetRGB(wxRect(0, 0, glyph.width, glyph.height), red(TRANSPARENT_COLOR), green(TRANSPARENT_COLOR), blue(TRANSPARENT_COLOR));
    img.InitAlpha();
    memset(img.GetAlpha(), 0, glyph.width * glyph.height);
    
    // Use the helper method to render pixels
    renderGlyphPixels(glyph, img, 0, 0, 0); // fontSize=0 for modern format
    
    return img;
}

// Import/Export functions moved from FontEditDialog

FontData::Range FontData::ImportBitmapAsRange(const wxString& filename, uint32_t baseCharacter, int8_t colorFormat)
{
    // Load the image
    wxImage image;
    if (!BitmapData::readFileToWxImage(filename, image)) {
        logError("Failed to load image file: " + filename.ToStdString());
        return FontData::Range(); // Return empty range
    }
    
    if (!image.IsOk() || image.GetWidth() == 0 || image.GetHeight() == 0) {
        logError("Invalid image dimensions");
        return FontData::Range(); // Return empty range
    }
    
    // Create range from bitmap with enhanced color analysis
    FontData::Range newRange = CreateRangeFromBitmap(image, baseCharacter, colorFormat);
    
    if (newRange.glyphs.empty()) {
        logError("No glyphs extracted from bitmap");
        return FontData::Range(); // Return empty range
    }
    
    return newRange;
}

FontData::Range FontData::ImportFntAsRange(const wxString& filename, uint32_t baseCharacter)
{
    // Load the .fnt file
    std::ifstream file(filename.ToStdString(), std::ios::binary);
    if (!file) {
        logError("Failed to open .fnt file: " + filename.ToStdString());
        return FontData::Range(); // Return empty range
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (fileSize == 0) {
        logError("Empty .fnt file: " + filename.ToStdString());
        return FontData::Range(); // Return empty range
    }

    if (fileSize < 4) {
        logError("Too small .fnt file size: " + filename.ToStdString());
        return FontData::Range(); // Return empty range
    }
    
    // Read file data
    std::vector<uint8_t> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    
    // Try to detect and parse the .fnt format
    FontData::Range newRange;
    
    // Read 4 bytes from the file
    uint32_t magic = *reinterpret_cast<uint32_t*>(buffer.data());
    
    // Check for GRX magic number
    if (magic == FONT_GRX_MAGIC) {
        // GRX format
        logInfo("Detected GRX font format");
        newRange = CreateRangeFromGrxFnt(buffer);
        
        // Adjust base character if specified
        if (baseCharacter != newRange.start) {
            uint32_t shift = baseCharacter - newRange.start;
            newRange.start = baseCharacter;
            newRange.end = newRange.start + newRange.glyphs.size() - 1;
            logInfo(wxString::Format("Adjusted GRX font base character to U+%04X", baseCharacter).ToStdString());
        }
    } else {
        // Try BIOS font format (common sizes: 8, 14, 16 pixels high)
        logInfo("Attempting to parse as BIOS font format");
        
        // Common BIOS font heights: 8, 14, 16
        std::vector<int> commonHeights = {8, 14, 16};
        bool found = false;
        
        for (int height : commonHeights) {
            size_t expectedSize = 95 * height; // 95 ASCII characters from space to tilde
            size_t expectedSize2 = 256 * height; // 256 ASCII characters from space to tilde
            if (fileSize == expectedSize || fileSize == expectedSize2) {
                logInfo(wxString::Format("Detected BIOS font format with height %d", height).ToStdString());
                newRange = CreateRangeFromBiosFnt(buffer, height, baseCharacter);
                found = true;
                break;
            }
        }
        
        if (!found) {
            // Try default height based on file size
            int charHeight = fileSize / 95;
            int charHeight2 = fileSize / 256;
            if (charHeight > 0 && (fileSize == 95 * charHeight || fileSize == 256 * charHeight2)) {
                logInfo(wxString::Format("Attempting BIOS font format with calculated height %d", charHeight).ToStdString());
                newRange = CreateRangeFromBiosFnt(buffer, charHeight, baseCharacter);
            } else {
                logError("Unknown .fnt file format");
                return FontData::Range(); // Return empty range
            }
        }
    }
    
    if (newRange.glyphs.empty()) {
        logError("Failed to extract glyphs from .fnt file");
        return FontData::Range(); // Return empty range
    }
    
    return newRange;
}

bool FontData::ImportScriptRanges(std::vector<FontData::ScriptRange> scriptRanges, FontData& fontData)
{
    if (scriptRanges.empty()) {
        logError("No ranges provided to import");
        return false;
    }
    
    bool fileExists = false;
    // Import all ranges from script
    for (const auto& rangeInfo : scriptRanges) {
        wxString rangeFilename = rangeInfo.filename;
        uint32_t startCode = rangeInfo.start;
        uint32_t endCode = rangeInfo.end;

        // check if file exists
        if (!wxFileName::FileExists(rangeFilename)) {
            logError("File does not exist: " + rangeFilename.ToStdString());
            continue;
        }
        fileExists = true;
        
        wxString rangeExt = wxFileName(rangeFilename).GetExt().Lower();
        
        FontData::Range newRange;
        
        // Import based on file type
        if (rangeExt == "fnt") {
            newRange = ImportFntAsRange(rangeFilename, startCode);
        } else {
            // Default to monochrome for bitmap files
            newRange = ImportBitmapAsRange(rangeFilename, startCode, 1);
        }
        
        if (!newRange.glyphs.empty()) {
            // Adjust range to match script specification
            newRange.start = startCode;
            newRange.end = endCode;
            
            // Add range to font data
            fontData.ranges.push_back(newRange);
            logInfo("Imported range U+" + wxString::Format("%04X-U+%04X", startCode, endCode).ToStdString() + 
                   " from " + rangeFilename.ToStdString());
        } else {
            logError("Failed to import range from " + rangeFilename.ToStdString());
        }
    }

    if (!fileExists) {
        logError("No files found in script");
        return false;
    }
    
    fontData.UpdateGlyphCount();
    
    logInfo("Successfully imported " + std::to_string(scriptRanges.size()) + " ranges from script");
    return !fontData.ranges.empty();
}

FontData::Range FontData::CreateRangeFromBitmap(const wxImage& image, uint32_t baseCharacter, int colorFormat)
{
    FontData::Range range;
    range.start = baseCharacter;
    
    if (colorFormat == 1) {
        // Force monochrome
        range.mono = 1;
        // Extract glyphs from the bitmap
        std::vector<FontData::Glyph> glyphs = ExtractGlyphsFromBitmap(image, colorFormat);
        
        if (!glyphs.empty()) {
            range.glyphs = glyphs;
            range.end = baseCharacter + glyphs.size() - 1;
        } else {
            range.end = baseCharacter;
        }
    } else {
        // Use the user-selected color format
        range.mono = colorFormat;  // Set to user's choice: 0=8-bit indexed, 24=24-bit RGB, -32=32-bit RGBA
        
        logInfo(wxString::Format("Using user-selected color format: %s", 
                                colorFormat == 0 ? "8-bit indexed" : 
                                colorFormat == 24 ? "24-bit RGB" : 
                                colorFormat == -32 ? "32-bit RGBA" : "unknown").ToStdString());
        
        // For auto palette matching (when user selects indexed mode), still check if colors match
        if (colorFormat == 0) {
            std::vector<uint8_t> defaultPalette;
            for (int i = 0; i < BitmapData::allegro_palette.size() / 3; i++) {
                if (i != SEPARATOR_COLOR_INDEX && i != TRANSPARENT_COLOR_INDEX) {
                    defaultPalette.push_back(BitmapData::allegro_palette[i * 3]);
                    defaultPalette.push_back(BitmapData::allegro_palette[i * 3 + 1]);
                    defaultPalette.push_back(BitmapData::allegro_palette[i * 3 + 2]);
                }
            }

            double score = BitmapData::calculatePaletteMatch(image, defaultPalette);
            logInfo(wxString::Format("Palette match score: %f", score).ToStdString());
            
            if (score < 0.95) {
                logWarning("Warning: Image colors may not match the default palette well. Consider using 24-bit RGB mode for better quality.");
            }
        }
        
        // Extract glyphs with selected format
        std::vector<FontData::Glyph> glyphs = ExtractGlyphsFromBitmap(image, colorFormat);
        
        if (!glyphs.empty()) {
            range.glyphs = glyphs;
            range.end = baseCharacter + glyphs.size() - 1;
        } else {
            range.end = baseCharacter;
        }
    }
    
    return range;
}

FontData::Range FontData::CreateRangeFromBiosFnt(const std::vector<uint8_t>& buffer, int charHeight, uint32_t baseCharacter)
{
    FontData::Range range;
    range.mono = 1;  // BIOS fonts are monochrome
    range.start = baseCharacter;
    range.end = baseCharacter + 95 - 1;  // ASCII tilde character
    
    // Parse each glyph (95 characters from space to tilde)
    for (int i = 0; i < 95; ++i) {
        FontData::Glyph glyph;
        glyph.width = 8;
        glyph.height = charHeight;
        
        // Extract glyph data (each character is charHeight bytes)
        size_t startPos = i * charHeight;
        if (startPos + charHeight > buffer.size()) {
            logError("Buffer too small for BIOS font glyph data");
            break;
        }
        
        glyph.data = std::vector<uint8_t>(buffer.begin() + startPos, buffer.begin() + startPos + charHeight);
        range.glyphs.push_back(glyph);
    }
    
    logInfo(wxString::Format("Created BIOS font range U+%04X-U+%04X with %d glyphs (%dx%d)", 
                            range.start, range.end, (int)range.glyphs.size(), 8, charHeight).ToStdString());
    return range;
}

FontData::Range FontData::CreateRangeFromGrxFnt(const std::vector<uint8_t>& buffer)
{
    FontData::Range range;
    range.mono = 1;  // GRX fonts are monochrome
    
    if (buffer.size() < 16) {  // Minimum header size
        logError("GRX font file too small for header");
        return range;
    }
    
    size_t pos = 0;
    
    // Read and verify magic number (already verified before calling this function)
    uint32_t magic = *reinterpret_cast<const uint32_t*>(&buffer[pos]);
    pos += 4;
    
    // Skip the second 4-byte value (pack_igetl(pack) in C code)
    pos += 4;
    
    // Read width and height (16-bit values)
    uint16_t fontWidth = *reinterpret_cast<const uint16_t*>(&buffer[pos]);
    pos += 2;
    uint16_t fontHeight = *reinterpret_cast<const uint16_t*>(&buffer[pos]);
    pos += 2;
    
    if (fontWidth == 0 || fontHeight == 0 || fontWidth > 64 || fontHeight > 64) {
        logError("Invalid GRX font dimensions");
        return range;
    }
    
    // Read begin and end character codes
    uint16_t beginChar = *reinterpret_cast<const uint16_t*>(&buffer[pos]);
    pos += 2;
    uint16_t endChar = *reinterpret_cast<const uint16_t*>(&buffer[pos]) + 1;  // C code adds 1
    pos += 2;
    
    range.start = beginChar;
    range.end = endChar - 1;  // Our end is inclusive, C code's is exclusive
    
    int numChars = endChar - beginChar;
    if (numChars <= 0 || numChars > 1000) {  // Sanity check
        logError("Invalid character range in GRX font");
        return range;
    }
    
    // Read width table flag
    uint16_t widthTableFlag = *reinterpret_cast<const uint16_t*>(&buffer[pos]);
    pos += 2;
    
    // Skip 38 bytes (as in C code)
    if (pos + 38 > buffer.size()) {
        logError("GRX font file too small for header padding");
        return range;
    }
    pos += 38;
    
    // Read width table if present
    std::vector<uint16_t> widthTable;
    bool hasWidthTable = (widthTableFlag == 0);
    
    if (hasWidthTable) {
        if (pos + numChars * 2 > buffer.size()) {
            logError("GRX font file too small for width table");
            return range;
        }
        
        widthTable.resize(numChars);
        for (int i = 0; i < numChars; i++) {
            widthTable[i] = *reinterpret_cast<const uint16_t*>(&buffer[pos]);
            pos += 2;
        }
    }
    
    // Read glyph data for each character
    for (int i = 0; i < numChars; i++) {
        FontData::Glyph glyph;
        
        // Determine width for this character
        uint16_t charWidth = hasWidthTable ? widthTable[i] : fontWidth;
        
        glyph.width = charWidth;
        glyph.height = fontHeight;
        
        // Calculate size in bytes: ((width + 7) / 8) * height
        size_t glyphSize = ((charWidth + 7) / 8) * fontHeight;
        
        if (pos + glyphSize > buffer.size()) {
            logError("GRX font file too small for glyph data");
            break;
        }
        
        // Copy glyph data
        glyph.data = std::vector<uint8_t>(buffer.begin() + pos, buffer.begin() + pos + glyphSize);
        pos += glyphSize;
        
        range.glyphs.push_back(glyph);
    }
    
    logInfo(wxString::Format("Created GRX font range U+%04X-U+%04X with %d glyphs (%dx%d%s)", 
                            range.start, range.end, (int)range.glyphs.size(), 
                            fontWidth, fontHeight, 
                            hasWidthTable ? ", variable width" : "").ToStdString());
    return range;
}

std::vector<FontData::Glyph> FontData::ExtractGlyphsFromBitmap(const wxImage& image, int8_t colorFormat)
{
    std::vector<FontData::Glyph> glyphs;
    
    // Determine if monochrome from color format
    bool isMonochrome = (colorFormat == 1);
    
    int imgWidth = image.GetWidth();
    int imgHeight = image.GetHeight();
    
    if (imgWidth == 0 || imgHeight == 0) {
        logError("Invalid image dimensions");
        return glyphs;
    }
    
    // Determine separator colors based on image format
    bool isPaletted = image.HasPalette();
    wxColour separatorColor;
    wxColour transparentColor;
    
    if (isPaletted) {
        // 8-bit paletted: separator = color 255, transparent = color 0
        // Get the palette
        const wxPalette& palette = image.GetPalette();
        unsigned char red, green, blue;
        palette.GetRGB(SEPARATOR_COLOR_INDEX, &red, &green, &blue);
        separatorColor = wxColour(red, green, blue);
        palette.GetRGB(TRANSPARENT_COLOR_INDEX, &red, &green, &blue);
        transparentColor = wxColour(red, green, blue);
    } else {
        // High/true color: separator = bright yellow, transparent = bright pink
        separatorColor = wxColor(red(SEPARATOR_COLOR), green(SEPARATOR_COLOR), blue(SEPARATOR_COLOR));
        transparentColor = wxColor(red(TRANSPARENT_COLOR), green(TRANSPARENT_COLOR), blue(TRANSPARENT_COLOR));
    }
    
    logInfo(wxString::Format("Extracting ASCII characters from %s image (%dx%d)", 
                            isPaletted ? "paletted" : "true color", imgWidth, imgHeight).ToStdString());
    logInfo(wxString::Format("Using separator color RGB(%d,%d,%d) and transparent color RGB(%d,%d,%d)",
                            separatorColor.Red(), separatorColor.Green(), separatorColor.Blue(),
                            transparentColor.Red(), transparentColor.Green(), transparentColor.Blue()).ToStdString());
    
    // Find character grid dimensions by detecting separator lines
    std::vector<int> columnSeparators;
    std::vector<int> rowSeparators;
    
    // Find vertical separators (columns)
    for (int x = 0; x < imgWidth; ++x) {
        bool isVerticalSeparator = true;
        for (int y = 0; y < imgHeight; ++y) {
            wxColour pixelColor(image.GetRed(x, y), image.GetGreen(x, y), image.GetBlue(x, y));
            if (pixelColor != separatorColor) {
                isVerticalSeparator = false;
                break;
            }
        }
        if (isVerticalSeparator) {
            columnSeparators.push_back(x);
        }
    }
    logInfo(wxString::Format("Found %d vertical separators", (int)columnSeparators.size()).ToStdString());
    if (!columnSeparators.empty()) {
        logDebug(wxString::Format("First few vertical separators: %d, %d, %d...", 
                                columnSeparators.size() > 0 ? columnSeparators[0] : -1,
                                columnSeparators.size() > 1 ? columnSeparators[1] : -1,
                                columnSeparators.size() > 2 ? columnSeparators[2] : -1).ToStdString());
        logDebug(wxString::Format("Last few vertical separators: ...%d, %d, %d", 
                                columnSeparators.size() > 2 ? columnSeparators[columnSeparators.size()-3] : -1,
                                columnSeparators.size() > 1 ? columnSeparators[columnSeparators.size()-2] : -1,
                                columnSeparators.size() > 0 ? columnSeparators[columnSeparators.size()-1] : -1).ToStdString());
    }
    
    // Find horizontal separators (rows)
    for (int y = 0; y < imgHeight; ++y) {
        bool isHorizontalSeparator = true;
        for (int x = 0; x < imgWidth; ++x) {
            wxColour pixelColor(image.GetRed(x, y), image.GetGreen(x, y), image.GetBlue(x, y));
            if (pixelColor != separatorColor) {
                isHorizontalSeparator = false;
                break;
            }
        }
        if (isHorizontalSeparator) {
            rowSeparators.push_back(y);
        }
    }
    logInfo(wxString::Format("Found %d horizontal separators", (int)rowSeparators.size()).ToStdString());
    if (!rowSeparators.empty()) {
        logDebug(wxString::Format("First few horizontal separators: %d, %d, %d...", 
                                rowSeparators.size() > 0 ? rowSeparators[0] : -1,
                                rowSeparators.size() > 1 ? rowSeparators[1] : -1,
                                rowSeparators.size() > 2 ? rowSeparators[2] : -1).ToStdString());
        logDebug(wxString::Format("Last few horizontal separators: ...%d, %d, %d", 
                                rowSeparators.size() > 2 ? rowSeparators[rowSeparators.size()-3] : -1,
                                rowSeparators.size() > 1 ? rowSeparators[rowSeparators.size()-2] : -1,
                                rowSeparators.size() > 0 ? rowSeparators[rowSeparators.size()-1] : -1).ToStdString());
    }
    
    // Calculate character cell boundaries
    std::vector<int> charLeftBounds, charRightBounds;
    std::vector<int> charTopBounds, charBottomBounds;
    
    // Calculate character column boundaries
    for (size_t i = 0; i < columnSeparators.size() - 1; ++i) {
        int left = columnSeparators[i] + 1;
        int right = columnSeparators[i + 1] - 1;
        if (left <= right) {
            charLeftBounds.push_back(left);
            charRightBounds.push_back(right);
        }
    }
    
    // Calculate character row boundaries
    for (size_t i = 0; i < rowSeparators.size() - 1; ++i) {
        int top = rowSeparators[i] + 1;
        int bottom = rowSeparators[i + 1] - 1;
        if (top <= bottom) {
            charTopBounds.push_back(top);
            charBottomBounds.push_back(bottom);
        }
    }
    
    int numCols = charLeftBounds.size();
    int numRows = charTopBounds.size();
    int totalChars = numCols * numRows;
    
    logInfo(wxString::Format("Grid: %d columns x %d rows = %d characters", numCols, numRows, totalChars).ToStdString());
    
    if (numCols == 0 || numRows == 0) {
        logError("No valid character grid found in bitmap");
        return glyphs;
    }
    
    // Extract each character
    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numCols; ++col) {
            int charIndex = row * numCols + col;
            
            int glyphLeft = charLeftBounds[col];
            int glyphTop = charTopBounds[row];
            int glyphRight = charRightBounds[col];
            int glyphBottom = charBottomBounds[row];
            
            int glyphWidth = glyphRight - glyphLeft + 1;
            int glyphHeight = glyphBottom - glyphTop + 1;
            
            if (glyphWidth <= 0 || glyphHeight <= 0) {
                logWarning(wxString::Format("Invalid glyph dimensions for character %d: %dx%d", 
                                          charIndex, glyphWidth, glyphHeight).ToStdString());
                continue;
            }
            
            FontData::Glyph glyph;
            glyph.width = glyphWidth;
            glyph.height = glyphHeight;
            
            // Extract glyph data based on color format
            if (colorFormat == 1) {
                // Monochrome format - pack into bits
                size_t bytesPerRow = (glyphWidth + 7) / 8;
                glyph.data.resize(bytesPerRow * glyphHeight);
                
                for (int y = 0; y < glyphHeight; ++y) {
                    for (int x = 0; x < glyphWidth; ++x) {
                        int imgX = glyphLeft + x;
                        int imgY = glyphTop + y;
                        wxColour pixelColor(image.GetRed(imgX, imgY), image.GetGreen(imgX, imgY), image.GetBlue(imgX, imgY));
                        
                        if (pixelColor != transparentColor && pixelColor != separatorColor) {
                            // Non-transparent pixel - set bit
                            size_t byteIndex = y * bytesPerRow + x / 8;
                            size_t bitIndex = 7 - (x % 8);
                            glyph.data[byteIndex] |= (1 << bitIndex);
                        }
                        // Transparent pixels remain 0 (already initialized)
                    }
                }
            } else if (colorFormat == 0) {
                // 8-bit indexed color format
                glyph.data.resize(glyphWidth * glyphHeight);
                
                for (int y = 0; y < glyphHeight; ++y) {
                    for (int x = 0; x < glyphWidth; ++x) {
                        int imgX = glyphLeft + x;
                        int imgY = glyphTop + y;
                        size_t pixelIndex = y * glyphWidth + x;
                        
                        if (pixelIndex < glyph.data.size()) {
                            wxColour pixelColor(image.GetRed(imgX, imgY), image.GetGreen(imgX, imgY), image.GetBlue(imgX, imgY));
                            
                            // Check pixel type
                            if (pixelColor == transparentColor || pixelColor == separatorColor) {
                                glyph.data[pixelIndex] = TRANSPARENT_COLOR_INDEX; // Transparent
                            } else {
                                // Find closest palette color (simplified - just use red component as index for now)
                                // In a real implementation, you'd find the closest match in the palette
                                uint8_t paletteIndex = (pixelColor.Red() + pixelColor.Green() + pixelColor.Blue()) / 3;
                                if (paletteIndex == TRANSPARENT_COLOR_INDEX) paletteIndex = 1; // Avoid transparency index
                                glyph.data[pixelIndex] = paletteIndex;
                            }
                        }
                    }
                }
            } else if (colorFormat == 24) {
                // 24-bit RGB format
                glyph.data.resize(glyphWidth * glyphHeight * 3);
                
                for (int y = 0; y < glyphHeight; ++y) {
                    for (int x = 0; x < glyphWidth; ++x) {
                        int imgX = glyphLeft + x;
                        int imgY = glyphTop + y;
                        size_t pixelIndex = (y * glyphWidth + x) * 3;
                        
                        if (pixelIndex + 2 < glyph.data.size()) {
                            wxColour pixelColor(image.GetRed(imgX, imgY), image.GetGreen(imgX, imgY), image.GetBlue(imgX, imgY));
                            
                            // Check pixel type
                            if (pixelColor == transparentColor || pixelColor == separatorColor) {
                                // Store transparent color (bright pink for transparency)
                                glyph.data[pixelIndex] = red(TRANSPARENT_COLOR);     // R
                                glyph.data[pixelIndex + 1] = green(TRANSPARENT_COLOR); // G
                                glyph.data[pixelIndex + 2] = blue(TRANSPARENT_COLOR);  // B
                            } else {
                                // Store actual RGB values
                                glyph.data[pixelIndex] = pixelColor.Red();     // R
                                glyph.data[pixelIndex + 1] = pixelColor.Green(); // G
                                glyph.data[pixelIndex + 2] = pixelColor.Blue();  // B
                            }
                        }
                    }
                } 
            } else if (colorFormat == -32) {
                // 32-bit RGBA format
                glyph.data.resize(glyphWidth * glyphHeight * 4);
                
                for (int y = 0; y < glyphHeight; ++y) {
                    for (int x = 0; x < glyphWidth; ++x) {
                        int imgX = glyphLeft + x;
                        int imgY = glyphTop + y;
                        size_t pixelIndex = (y * glyphWidth + x) * 4;
                        
                        if (pixelIndex + 3 < glyph.data.size()) {
                            wxColour pixelColor(image.GetRed(imgX, imgY), image.GetGreen(imgX, imgY), image.GetBlue(imgX, imgY));
                            
                            // Check pixel type
                            if (pixelColor == transparentColor || pixelColor == separatorColor) {
                                // Store transparent color with alpha = 0
                                glyph.data[pixelIndex] = red(TRANSPARENT_COLOR);     // R
                                glyph.data[pixelIndex + 1] = green(TRANSPARENT_COLOR); // G
                                glyph.data[pixelIndex + 2] = blue(TRANSPARENT_COLOR);  // B
                                glyph.data[pixelIndex + 3] = 0;                      // A (transparent)
                            } else {
                                // Store actual RGBA values
                                glyph.data[pixelIndex] = pixelColor.Red();     // R
                                glyph.data[pixelIndex + 1] = pixelColor.Green(); // G
                                glyph.data[pixelIndex + 2] = pixelColor.Blue();  // B
                                glyph.data[pixelIndex + 3] = 255;             // A (opaque)
                            }
                        }
                    }
                }
            }
            
            glyphs.push_back(glyph);
        }
    }
    
    logInfo(wxString::Format("Extracted %d glyphs from bitmap", (int)glyphs.size()).ToStdString());
    return glyphs;
}

bool FontData::ExportRangeAsBitmap(const FontData::Range& range, const wxString& filename)
{
    if (range.glyphs.empty()) {
        logError("Cannot export empty range");
        return false;
    }
    
    // Calculate grid dimensions for the bitmap
    int numGlyphs = range.glyphs.size();
    int cols = (int)std::ceil(std::sqrt(numGlyphs)); // Square-ish grid
    int rows = (numGlyphs + cols - 1) / cols; // Round up
    
    // Find maximum glyph dimensions
    int maxWidth = 0, maxHeight = 0;
    for (const auto& glyph : range.glyphs) {
        maxWidth = std::max(maxWidth, (int)glyph.width);
        maxHeight = std::max(maxHeight, (int)glyph.height);
    }
    
    // Add padding and separator lines
    int cellWidth = maxWidth;
    int cellHeight = maxHeight;
    int separatorWidth = 1;
    
    // Calculate total bitmap size
    int bitmapWidth = cols * cellWidth + (cols + 1) * separatorWidth;
    int bitmapHeight = rows * cellHeight + (rows + 1) * separatorWidth;
    
    // Create the bitmap
    wxImage exportImage(bitmapWidth, bitmapHeight);
    exportImage.SetRGB(wxRect(0, 0, bitmapWidth, bitmapHeight), red(SEPARATOR_COLOR), green(SEPARATOR_COLOR), blue(SEPARATOR_COLOR)); // Yellow background (separator color)
    
    // Draw each glyph
    for (int i = 0; i < numGlyphs; ++i) {
        int row = i / cols;
        int col = i % cols;
        
        // Calculate cell position
        int cellX = separatorWidth + col * (cellWidth + separatorWidth);
        int cellY = separatorWidth + row * (cellHeight + separatorWidth);
        
        // Fill cell background with transparent color
        for (int y = 0; y < cellHeight; ++y) {
            for (int x = 0; x < cellWidth; ++x) {
                exportImage.SetRGB(cellX + x, cellY + y, red(TRANSPARENT_COLOR), green(TRANSPARENT_COLOR), blue(TRANSPARENT_COLOR)); // Bright pink (transparent)
            }
        }
        
        // Render the glyph
        wxImage glyphImage = range.renderGlyph(i);
        if (glyphImage.IsOk()) {
            // Center the glyph in the cell
            int offsetX = cellX + (cellWidth - glyphImage.GetWidth()) / 2;
            int offsetY = cellY + (cellHeight - glyphImage.GetHeight()) / 2;
            
            // Copy glyph pixels
            for (int y = 0; y < glyphImage.GetHeight(); ++y) {
                for (int x = 0; x < glyphImage.GetWidth(); ++x) {
                    int destX = offsetX + x;
                    int destY = offsetY + y;
                    if (destX >= 0 && destX < bitmapWidth && destY >= 0 && destY < bitmapHeight) {
                        exportImage.SetRGB(destX, destY, 
                                         glyphImage.GetRed(x, y),
                                         glyphImage.GetGreen(x, y),
                                         glyphImage.GetBlue(x, y));
                    }
                }
            }
        }
    }
    
    // Save the image - detect format from file extension
    wxString ext = filename.AfterLast('.').Lower();
    wxBitmapType imageType;
    
    if (ext == "bmp") {
        imageType = wxBITMAP_TYPE_BMP;
    } else if (ext == "png") {
        imageType = wxBITMAP_TYPE_PNG;
    } else if (ext == "tga") {
        imageType = wxBITMAP_TYPE_TGA;
    } else if (ext == "pcx") {
        imageType = wxBITMAP_TYPE_PCX;
    } else {
        // Default to BMP for unknown extensions
        imageType = wxBITMAP_TYPE_BMP;
    }
    
    if (!exportImage.SaveFile(filename, imageType)) {
        logError("Failed to save bitmap file: " + filename.ToStdString());
        return false;
    }
    
    return true;
}

bool FontData::ExportRangeAsFnt(const FontData::Range& range, const wxString& filename)
{
    if (range.glyphs.empty()) {
        logError("Cannot export empty range");
        return false;
    }
    
    // Export as GRX format
    std::ofstream file(filename.ToStdString(), std::ios::binary);
    if (!file) {
        logError("Failed to create font file: " + filename.ToStdString());
        return false;
    }
    
    // Determine if we need a width table (variable width font)
    bool needsWidthTable = false;
    uint16_t commonWidth = range.glyphs[0].width;
    uint16_t commonHeight = range.glyphs[0].height;
    
    for (const auto& glyph : range.glyphs) {
        if (glyph.width != commonWidth) {
            needsWidthTable = true;
        }
        // All glyphs should have the same height in GRX format
        if (glyph.height != commonHeight) {
            logError("Cannot export as GRX: all glyphs must have the same height");
            return false;
        }
    }
    
    // Write GRX header
    // Magic number (FONTMAGIC)
    uint32_t magic = FONT_GRX_MAGIC;
    file.write(reinterpret_cast<const char*>(&magic), 4);
    
    // Second 4-byte value (version/flags - set to 0)
    uint32_t version = 0;
    file.write(reinterpret_cast<const char*>(&version), 4);
    
    // Font width and height (16-bit values)
    uint16_t fontWidth = needsWidthTable ? 0 : commonWidth; // 0 indicates variable width
    uint16_t fontHeight = commonHeight;
    file.write(reinterpret_cast<const char*>(&fontWidth), 2);
    file.write(reinterpret_cast<const char*>(&fontHeight), 2);
    
    // Character range (begin and end)
    uint16_t beginChar = static_cast<uint16_t>(range.start);
    uint16_t endChar = static_cast<uint16_t>(range.end); // GRX expects end-1, but we'll adjust
    file.write(reinterpret_cast<const char*>(&beginChar), 2);
    file.write(reinterpret_cast<const char*>(&endChar), 2);
    
    // Width table flag (0 = has width table, non-zero = fixed width)
    uint16_t widthTableFlag = needsWidthTable ? 0 : 1;
    file.write(reinterpret_cast<const char*>(&widthTableFlag), 2);
    
    // Write 38 bytes of padding (as in original GRX format)
    std::vector<uint8_t> padding(38, 0);
    file.write(reinterpret_cast<const char*>(padding.data()), 38);
    
    // Write width table if needed
    if (needsWidthTable) {
        for (const auto& glyph : range.glyphs) {
            uint16_t glyphWidth = static_cast<uint16_t>(glyph.width);
            file.write(reinterpret_cast<const char*>(&glyphWidth), 2);
        }
    }
    
    // Write glyph data
    for (const auto& glyph : range.glyphs) {
        // Verify glyph data size matches expected size
        size_t expectedSize = ((glyph.width + 7) / 8) * glyph.height;
        if (glyph.data.size() != expectedSize) {
            logError(wxString::Format("Glyph data size mismatch: expected %zu, got %zu", 
                                    expectedSize, glyph.data.size()).ToStdString());
            return false;
        }
        
        file.write(reinterpret_cast<const char*>(glyph.data.data()), glyph.data.size());
    }
    
    file.close();
    
    logInfo(wxString::Format("Exported as GRX font format: %s range U+%04X-U+%04X, %d glyphs (%dx%d%s)",
                            range.mono ? "monochrome" : "color",
                            range.start, range.end, 
                            (int)range.glyphs.size(),
                            needsWidthTable ? 0 : commonWidth, commonHeight,
                            needsWidthTable ? ", variable width" : "").ToStdString());
    
    return true;
}

// Helper method to parse range information from filename
bool FontData::ParseRangeFromFilename(const wxString& filename, uint32_t& startCode, uint32_t& endCode)
{
    // Extract just the filename without path and extension
    wxFileName fn(filename);
    wxString baseName = fn.GetName();
    
    // Look for patterns like: 
    // Two values: font_0x0020_0x007F or font_20_7F or font_U0020_U007F
    // Single value: font_0x0020 or font_U0020 (assumes standard 95-char range)
    wxRegEx hexDoublePattern(".*[_-](?:0x|U\\+?)([0-9A-Fa-f]+)[_-](?:0x|U\\+?)([0-9A-Fa-f]+)");
    wxRegEx decDoublePattern(".*[_-]([0-9]+)[_-]([0-9]+)");
    wxRegEx hexSinglePattern(".*[_-](?:0x|U\\+?)([0-9A-Fa-f]+)$");
    wxRegEx decSinglePattern(".*[_-]([0-9]+)$");
    
    if (hexDoublePattern.Matches(baseName)) {
        wxString startStr = hexDoublePattern.GetMatch(baseName, 1);
        wxString endStr = hexDoublePattern.GetMatch(baseName, 2);
        
        unsigned long start, end;
        if (startStr.ToULong(&start, 16) && endStr.ToULong(&end, 16)) {
            startCode = static_cast<uint32_t>(start);
            endCode = static_cast<uint32_t>(end);
            return true;
        }
    } else if (decDoublePattern.Matches(baseName)) {
        wxString startStr = decDoublePattern.GetMatch(baseName, 1);
        wxString endStr = decDoublePattern.GetMatch(baseName, 2);
        
        unsigned long start, end;
        if (startStr.ToULong(&start, 10) && endStr.ToULong(&end, 10)) {
            startCode = static_cast<uint32_t>(start);
            endCode = static_cast<uint32_t>(end);
            return true;
        }
    } else if (hexSinglePattern.Matches(baseName)) {
        wxString startStr = hexSinglePattern.GetMatch(baseName, 1);
        
        unsigned long start;
        if (startStr.ToULong(&start, 16)) {
            startCode = static_cast<uint32_t>(start);
            endCode = std::numeric_limits<uint32_t>::max();
            return true;
        }
    } else if (decSinglePattern.Matches(baseName)) {
        wxString startStr = decSinglePattern.GetMatch(baseName, 1);
        
        unsigned long start;
        if (startStr.ToULong(&start, 10)) {
            startCode = static_cast<uint32_t>(start);
            endCode = std::numeric_limits<uint32_t>::max();
            return true;
        }
    }
    
    return false;
}

// Helper method to parse script file format
bool FontData::ParseScriptFile(const wxString& scriptPath, std::vector<ScriptRange>& ranges)
{
    ranges.clear();
    
    wxTextFile file;
    if (!file.Open(scriptPath)) {
        return false;
    }
    
    wxString currentFilename;
    wxString scriptDir = wxFileName(scriptPath).GetPath();
    
    for (size_t i = 0; i < file.GetLineCount(); i++) {
        wxString line = file.GetLine(i).Trim().Trim(false);
        
        // Skip empty lines and comments
        if (line.IsEmpty() || line.StartsWith("#") || line.StartsWith("//")) {
            continue;
        }
        
        // Parse line format: "filename start end" or "- start end"
        // Simple approach: replace multiple spaces/tabs with single space, then split
        wxString cleanLine = line;
        cleanLine.Replace("\t", " ");  // Replace tabs with spaces
        while (cleanLine.Replace("  ", " ")) {  // Replace multiple spaces with single space
            // Keep replacing until no more double spaces
        }
        cleanLine = cleanLine.Trim().Trim(false);  // Trim leading/trailing spaces
        
        // Split on single space
        wxArrayString tokens = wxSplit(cleanLine, ' ', '\0');
        
        if (tokens.GetCount() < 3) {
            logInfo("ParseScriptFile: Not enough tokens (" + std::to_string(tokens.GetCount()) + "), skipping line");
            continue; // Invalid line format
        }
        
        wxString fileToken = tokens[0];
        wxString startToken = tokens[1];
        wxString endToken = tokens[2];
        
        // Parse start and end codes
        unsigned long start, end;
        bool startOk = false, endOk = false;
        
        // Try parsing as hex first (0x format)
        if (startToken.StartsWith("0x") || startToken.StartsWith("0X")) {
            startOk = startToken.Mid(2).ToULong(&start, 16);
        } else if (startToken.StartsWith("U+") || startToken.StartsWith("u+")) {
            startOk = startToken.Mid(2).ToULong(&start, 16);
        } else {
            // Try decimal
            startOk = startToken.ToULong(&start, 10);
            if (!startOk) {
                // Try hex without prefix
                startOk = startToken.ToULong(&start, 16);
            }
        }
        
        if (endToken.StartsWith("0x") || endToken.StartsWith("0X")) {
            endOk = endToken.Mid(2).ToULong(&end, 16);
        } else if (endToken.StartsWith("U+") || endToken.StartsWith("u+")) {
            endOk = endToken.Mid(2).ToULong(&end, 16);
        } else {
            // Try decimal
            endOk = endToken.ToULong(&end, 10);
            if (!endOk) {
                // Try hex without prefix
                endOk = endToken.ToULong(&end, 16);
            }
        }
        
        if (!startOk || !endOk) {
            logInfo(wxString::Format("ParseScriptFile: Invalid number format - startOk=%s, endOk=%s", startOk ? "true" : "false", endOk ? "true" : "false").ToStdString());
            continue; // Invalid number format
        }
        
        // Handle filename
        if (fileToken == "-") {
            // Use previous filename
            if (currentFilename.IsEmpty()) {
                continue; // No previous filename
            }
        } else {
            // New filename - make it relative to script directory if not absolute
            wxFileName fn(fileToken);
            if (fn.IsAbsolute()) {
                currentFilename = fileToken;
            } else {
                currentFilename = scriptDir + wxFileName::GetPathSeparator() + fileToken;
            }
        }
        
        ScriptRange scriptRange;
        scriptRange.filename = currentFilename;
        scriptRange.start = static_cast<uint32_t>(start);
        scriptRange.end = static_cast<uint32_t>(end);
        ranges.push_back(scriptRange);
    }
    
    file.Close();
    return !ranges.empty();
}

// Smart export function that handles single vs multiple ranges
bool FontData::ExportFontAsBitmap(const wxString& baseFilename) const
{
    if (ranges.empty()) {
        logError("No ranges to export");
        return false;
    }
    
    wxFileName fn(baseFilename);
    wxString baseName = fn.GetName();
    wxString extension = fn.GetExt();
    wxString directory = fn.GetPath();
    
    if (ranges.size() == 1) {
        // Single range: add range suffix only if it differs from default 0x0020
        const Range& range = ranges[0];
        wxString finalFilename;
        
        if (range.start != 0x0020) {
            // Non-standard range start, add suffix
            wxString rangeFilename = wxString::Format("%s_0x%04X.%s", 
                                                     baseName, 
                                                     range.start,
                                                     extension);
            if (!directory.IsEmpty()) {
                finalFilename = directory + wxFileName::GetPathSeparator() + rangeFilename;
            } else {
                finalFilename = rangeFilename;
            }
        } else {
            // Standard range start (0x0020), use original filename
            finalFilename = baseFilename;
        }
        
        logInfo(wxString::Format("Exporting single range to: %s", finalFilename).ToStdString());
        return ExportRangeAsBitmap(range, finalFilename);
    } else {
        // Multiple ranges: export all to one bitmap + create script file
        
        // Create combined bitmap with all ranges
        std::vector<const Range*> allRanges;
        for (const auto& range : ranges) {
            allRanges.push_back(&range);
        }
        
        // Calculate total glyphs and grid dimensions
        int totalGlyphs = 0;
        int maxWidth = 0, maxHeight = 0;
        
        for (const auto& range : ranges) {
            totalGlyphs += range.glyphs.size();
            for (const auto& glyph : range.glyphs) {
                maxWidth = std::max(maxWidth, (int)glyph.width);
                maxHeight = std::max(maxHeight, (int)glyph.height);
            }
        }
        
        // Calculate grid layout
        int cols = (int)std::ceil(std::sqrt(totalGlyphs));
        int rows = (totalGlyphs + cols - 1) / cols;
        
        // Cell and separator dimensions
        int cellWidth = maxWidth;
        int cellHeight = maxHeight;
        int separatorWidth = 1;
        
        // Calculate total bitmap size
        int bitmapWidth = cols * cellWidth + (cols + 1) * separatorWidth;
        int bitmapHeight = rows * cellHeight + (rows + 1) * separatorWidth;
        
        // Create the combined bitmap
        wxImage exportImage(bitmapWidth, bitmapHeight);
        exportImage.SetRGB(wxRect(0, 0, bitmapWidth, bitmapHeight), red(SEPARATOR_COLOR), green(SEPARATOR_COLOR), blue(SEPARATOR_COLOR));
        
        // Draw all glyphs from all ranges
        int currentGlyph = 0;
        for (const auto& range : ranges) {
            for (size_t g = 0; g < range.glyphs.size(); ++g) {
                int row = currentGlyph / cols;
                int col = currentGlyph % cols;
                
                // Calculate cell position
                int cellX = separatorWidth + col * (cellWidth + separatorWidth);
                int cellY = separatorWidth + row * (cellHeight + separatorWidth);
                
                // Fill cell background with transparent color
                for (int y = 0; y < cellHeight; ++y) {
                    for (int x = 0; x < cellWidth; ++x) {
                        exportImage.SetRGB(cellX + x, cellY + y, red(TRANSPARENT_COLOR), green(TRANSPARENT_COLOR), blue(TRANSPARENT_COLOR));
                    }
                }
                
                // Render the glyph
                wxImage glyphImage = range.renderGlyph(g);
                if (glyphImage.IsOk()) {
                    // Center the glyph in the cell
                    int offsetX = cellX + (cellWidth - glyphImage.GetWidth()) / 2;
                    int offsetY = cellY + (cellHeight - glyphImage.GetHeight()) / 2;
                    
                    // Copy glyph pixels
                    for (int y = 0; y < glyphImage.GetHeight(); ++y) {
                        for (int x = 0; x < glyphImage.GetWidth(); ++x) {
                            int destX = offsetX + x;
                            int destY = offsetY + y;
                            if (destX >= 0 && destX < bitmapWidth && destY >= 0 && destY < bitmapHeight) {
                                exportImage.SetRGB(destX, destY, 
                                                 glyphImage.GetRed(x, y),
                                                 glyphImage.GetGreen(x, y),
                                                 glyphImage.GetBlue(x, y));
                            }
                        }
                    }
                }
                
                currentGlyph++;
            }
        }
        
        // Save the combined bitmap
        wxString bitmapPath;
        if (!directory.IsEmpty()) {
            bitmapPath = directory + wxFileName::GetPathSeparator() + baseName + "." + extension;
        } else {
            bitmapPath = baseName + "." + extension;
        }
        
        // Detect image format
        wxBitmapType imageType;
        wxString ext = extension.Lower();
        if (ext == "bmp") {
            imageType = wxBITMAP_TYPE_BMP;
        } else if (ext == "png") {
            imageType = wxBITMAP_TYPE_PNG;
        } else if (ext == "tga") {
            imageType = wxBITMAP_TYPE_TGA;
        } else if (ext == "pcx") {
            imageType = wxBITMAP_TYPE_PCX;
        } else {
            imageType = wxBITMAP_TYPE_BMP;
        }
        
        if (!exportImage.SaveFile(bitmapPath, imageType)) {
            logError("Failed to save combined bitmap file: " + bitmapPath.ToStdString());
            return false;
        }
        
        // Create script file
        wxString scriptPath;
        if (!directory.IsEmpty()) {
            scriptPath = directory + wxFileName::GetPathSeparator() + baseName + ".txt";
        } else {
            scriptPath = baseName + ".txt";
        }
        
        wxString bitmapFilename = baseName + "." + extension;
        if (!CreateScriptFile(scriptPath, ranges, bitmapFilename)) {
            logError("Failed to create script file: " + scriptPath.ToStdString());
            return false;
        }
        
        logInfo(wxString::Format("Exported %d ranges to: %s (with script: %s)", 
                                (int)ranges.size(), bitmapPath, scriptPath).ToStdString());
        return true;
    }
}

// Create script file for multiple ranges
bool FontData::CreateScriptFile(const wxString& scriptPath, const std::vector<Range>& ranges, const wxString& bitmapFilename)
{
    // Remove existing file if it exists to ensure we can create a new one
    if (wxFileExists(scriptPath)) {
        wxRemoveFile(scriptPath);
    }
    
    wxTextFile file;
    if (!file.Create(scriptPath)) {
        return false;
    }
    
    file.AddLine("# Font export script");
    file.AddLine("# Format: filename start_code end_code");
    file.AddLine("");
    
    // Add entry for each range
    for (size_t i = 0; i < ranges.size(); ++i) {
        const Range& range = ranges[i];
        
        if (i == 0) {
            // First range uses the bitmap filename
            file.AddLine(wxString::Format("%s 0x%04X 0x%04X", 
                                        bitmapFilename, range.start, range.end));
        } else {
            // Subsequent ranges use "-" to indicate same file
            file.AddLine(wxString::Format("- 0x%04X 0x%04X", 
                                        range.start, range.end));
        }
    }
    
    if (!file.Write()) {
        return false;
    }
    
    file.Close();
    return true;
}

// Add new utility methods
wxString FontData::formatUnicodeRange(uint32_t start, uint32_t end) {
    if (start == end) {
        return wxString::Format("U+%04X", start);
    } else {
        return wxString::Format("U+%04X-U+%04X", start, end);
    }
}

wxString FontData::getColorDepthStringFromMono(int8_t mono) {
    int bitDepth = getBitDepthFromMono(mono);
    return getColorDepthString(bitDepth);
}

wxBitmap FontData::createGlyphDisplayBitmap(const Range& range, uint32_t glyphIndex, int targetSize) {
    // Check if glyph index is valid
    if (glyphIndex >= range.glyphs.size()) {
        // Return empty bitmap for invalid glyphs
        wxBitmap emptyBitmap(targetSize, targetSize);
        wxMemoryDC dc(emptyBitmap);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
        return emptyBitmap;
    }

    // Use the Range::renderGlyph method to get the glyph image
    wxImage glyphImg = range.renderGlyph(glyphIndex);
    
    if (!glyphImg.IsOk() || glyphImg.GetWidth() == 0 || glyphImg.GetHeight() == 0) {
        // Return empty bitmap for invalid glyph images
        wxBitmap emptyBitmap(targetSize, targetSize);
        wxMemoryDC dc(emptyBitmap);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
        return emptyBitmap;
    }
    
    // Scale the image to fit in target size while maintaining aspect ratio
    int scaledWidth = glyphImg.GetWidth();
    int scaledHeight = glyphImg.GetHeight();
    
    // Calculate scaling factor to fit within target size
    double scaleX = (double)targetSize / scaledWidth;
    double scaleY = (double)targetSize / scaledHeight;
    double scale = std::min(scaleX, scaleY);
    
    if (scale < 1.0) {
        scaledWidth = (int)(scaledWidth * scale);
        scaledHeight = (int)(scaledHeight * scale);
        glyphImg = glyphImg.Scale(scaledWidth, scaledHeight, wxIMAGE_QUALITY_NEAREST);
    }
    
    // Create a target size bitmap with white background
    wxBitmap bitmap(targetSize, targetSize);
    wxMemoryDC dc(bitmap);
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    
    // Center the glyph image in the bitmap
    int offsetX = (targetSize - scaledWidth) / 2;
    int offsetY = (targetSize - scaledHeight) / 2;
    
    wxBitmap glyphBitmap(glyphImg);
    dc.DrawBitmap(glyphBitmap, offsetX, offsetY, true); // true for transparency
    
    dc.SelectObject(wxNullBitmap);
    return bitmap;
}

bool FontData::operator==(const FontData& other) const {
    if (typeID != other.typeID) return false;
    if (fontSize != other.fontSize) return false;
    if (glyphCount != other.glyphCount) return false;
    if (ranges.size() != other.ranges.size()) return false;
    
    // Compare ranges
    for (size_t i = 0; i < ranges.size(); ++i) {
        const Range& r1 = ranges[i];
        const Range& r2 = other.ranges[i];
        
        if (r1.mono != r2.mono) return false;
        if (r1.start != r2.start) return false;
        if (r1.end != r2.end) return false;
        if (r1.glyphs.size() != r2.glyphs.size()) return false;
        
        // Compare glyphs
        for (size_t j = 0; j < r1.glyphs.size(); ++j) {
            const Glyph& g1 = r1.glyphs[j];
            const Glyph& g2 = r2.glyphs[j];
            
            if (g1.width != g2.width) return false;
            if (g1.height != g2.height) return false;
            if (g1.data != g2.data) return false;
        }
    }
    
    return true;
}

bool FontData::operator!=(const FontData& other) const {
    return !(*this == other);
}