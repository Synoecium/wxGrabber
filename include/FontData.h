#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <wx/string.h>
#include <wx/font.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include "CommonTypes.h"
#include "wx/colour.h"

const uint32_t FONT_GRX_MAGIC = 0x19590214L;


class FontData {
public:
    struct Glyph {
        uint16_t width;
        uint16_t height;
        std::vector<uint8_t> data;
        
    };
    struct Range {
        int8_t mono;
        uint32_t start;
        uint32_t end;
        std::vector<Glyph> glyphs;
        wxImage renderGlyph(uint32_t index) const;
        
        // Helper method for rendering glyph pixels
        void renderGlyphPixels(const Glyph& glyph, wxImage& img, int offsetX, int offsetY, int fontSize) const;
        int getBitDepth() const;
    };
    struct ScriptRange {
        ScriptRange(wxString filename, uint32_t start, uint32_t end) : filename(filename), start(start), end(end) {};
        ScriptRange() : filename(""), start(0), end(0) {};
        wxString filename;
        uint32_t start;
        uint32_t end;
    };

    FontData();
    bool isValidFormat() const;
    int getGlyphCount() const;
    int getFontSize() const;
    static bool parse(const std::vector<uint8_t>& dataBuffer, ObjectType typeID, FontData& outFont);
    std::vector<uint8_t> serialize() const;
    bool compareWithFile(const std::string& filepath) const;
    bool importFromFile(const std::string& filepath);
    wxString getPreviewCaption() const;
    wxImage getPreviewImage() const;
    wxFont getWxFont() const;
    wxImage renderGlyph(uint32_t codePoint) const;
    int getMaxBitDepth() const;
    void UpdateGlyphCount();
    bool CheckRangeOverlap(const FontData::Range& newRange, std::vector<wxString>& overlappingRanges) const;
    static int getBitDepthFromMono(int8_t mono);
    static wxString getColorDepthString(int bitDepth);
    static wxString formatUnicodeRange(uint32_t start, uint32_t end);
    static wxString getColorDepthStringFromMono(int8_t mono);
    static wxBitmap createGlyphDisplayBitmap(const Range& range, uint32_t glyphIndex, int targetSize = 16);

    // Import/Export functions
    static Range ImportBitmapAsRange(const wxString& filename, uint32_t baseCharacter, int8_t colorFormat);
    static Range ImportFntAsRange(const wxString& filename, uint32_t baseCharacter);
    static bool ImportScriptRanges(std::vector<FontData::ScriptRange> scriptRanges, FontData& fontData);
    static Range CreateRangeFromBitmap(const wxImage& image, uint32_t baseCharacter, int colorFormat);
    static Range CreateRangeFromBiosFnt(const std::vector<uint8_t>& buffer, int charHeight, uint32_t baseCharacter);
    static Range CreateRangeFromGrxFnt(const std::vector<uint8_t>& buffer);
    static std::vector<Glyph> ExtractGlyphsFromBitmap(const wxImage& image, int8_t colorFormat);
    static bool ExportRangeAsBitmap(const Range& range, const wxString& filename);
    static bool ExportRangeAsFnt(const Range& range, const wxString& filename);

    // Range parsing functions
    static bool ParseRangeFromFilename(const wxString& filename, uint32_t& startCode, uint32_t& endCode);
    static bool ParseScriptFile(const wxString& scriptPath, std::vector<ScriptRange>& ranges);
    
    // Smart export functions
    bool ExportFontAsBitmap(const wxString& baseFilename) const;
    static bool CreateScriptFile(const wxString& scriptPath, const std::vector<Range>& ranges, const wxString& bitmapFilename);

    // Font properties
    ObjectType typeID = ObjectType::DAT_FONT;
    int fontSize = 0;
    int glyphCount = 0;
    std::vector<Range> ranges;

    // Equality operator
    bool operator==(const FontData& other) const;
    bool operator!=(const FontData& other) const;

protected:
    wxString getFontFormatString() const;
}; 