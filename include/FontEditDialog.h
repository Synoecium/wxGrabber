#pragma once

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/dataview.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include "FontData.h"

// Custom bitmap renderer for glyph images
class GlyphBitmapRenderer : public wxDataViewCustomRenderer {
public:
    GlyphBitmapRenderer();
    
    virtual bool SetValue(const wxVariant& value) override;
    virtual bool GetValue(wxVariant& value) const override;
    virtual bool Render(wxRect rect, wxDC* dc, int state) override;
    virtual wxSize GetSize() const override;
    
private:
    wxBitmap m_bitmap;
};

// Custom model for glyph data view
class GlyphDataModel : public wxDataViewVirtualListModel {
public:
    GlyphDataModel();
    
    void SetRange(const FontData::Range* range);
    void Clear();
    
    // wxDataViewVirtualListModel overrides
    virtual unsigned int GetColumnCount() const override;
    virtual wxString GetColumnType(unsigned int col) const override;
    virtual void GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const override;
    virtual bool SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) override;
    virtual unsigned int GetCount() const override;
    
private:
    const FontData::Range* m_range;
};

// Dialog for entering base character when importing a range
class BaseCharacterDialog : public wxDialog {
public:
    BaseCharacterDialog(wxWindow* parent, const wxString& scriptDescription = wxEmptyString);
    
    uint32_t GetBaseCharacter() const { return m_baseCharacter; }
    int GetColorFormat() const { return m_colorFormat; }
    
    // Set the base character (useful for auto-detection)
    void SetBaseCharacter(uint32_t baseChar);
    
    // Legacy compatibility method
    bool IsMonochrome() const { return m_colorFormat == 1; }
    
private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnTextChanged(wxCommandEvent& event);
    void OnSpinChanged(wxSpinEvent& event);
    void OnColorFormatChanged(wxCommandEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    
    wxTextCtrl* m_textCtrl;
    wxSpinCtrl* m_spinCtrl;
    wxChoice* m_colorFormatChoice;
    wxStaticText* m_previewLabel;
    
    uint32_t m_baseCharacter;
    int m_colorFormat;  // mono value: 1=monochrome, 0=8-bit indexed, 24=24-bit RGB, -32=32-bit RGBA
    
    enum {
        ID_TEXT_CTRL = wxID_HIGHEST + 100,
        ID_SPIN_CTRL,
        ID_COLOR_FORMAT_CHOICE
    };
    
    wxDECLARE_EVENT_TABLE();
};

class FontEditDialog : public wxDialog {
public:
    FontEditDialog(wxWindow* parent, FontData& fontData);
    virtual ~FontEditDialog();

    // Get the modified font data
    const FontData& GetFontData() const { return m_fontData; }
    
    // Static helper for model to access glyph creation
    static wxBitmap CreateGlyphBitmapStatic(const FontData::Range& range, uint32_t glyphIndex);
    
    // Static helper for font import functionality (reusable by main frame)
    static bool ImportFontRange(wxWindow* parent, FontData& fontData, const wxString& operationName = "import", wxString* outFilename = nullptr);
    static bool GrabFontFromFile(wxWindow* parent, FontData& fontData, const wxString& filepath);

private:
    // Event handlers
    void OnImportRange(wxCommandEvent& event);
    void OnExportRange(wxCommandEvent& event);
    void OnDeleteRange(wxCommandEvent& event);
    void OnDeleteGlyph(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnRangeSelected(wxListEvent& event);
    void OnRangeDeselected(wxListEvent& event);
    void OnRangeKeyDown(wxListEvent& event);
    void OnGlyphSelected(wxDataViewEvent& event);
    void OnGlyphDeselected(wxDataViewEvent& event);
    void OnGlyphKeyDown(wxKeyEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    // Helper methods
    void PopulateRangeList();
    void PopulateGlyphTable();
    void UpdateButtonStates();
    void UpdateGlyphButtonStates();

    static bool CheckRangeOverlap(wxWindow* parent, FontData& fontData, const std::vector<FontData::Range>& ranges);
    
    // Glyph management methods
    bool DeleteSelectedGlyph();

    // UI components
    wxListCtrl* m_rangeList;
    wxDataViewCtrl* m_glyphTable;
    GlyphDataModel* m_glyphModel;
    wxImageList* m_glyphImageList;
    wxButton* m_importButton;
    wxButton* m_exportButton;
    wxButton* m_deleteButton;
    wxButton* m_deleteGlyphButton;
    wxButton* m_exitButton;

    // Data
    FontData& m_fontData;

    // Event IDs
    enum {
        ID_IMPORT_RANGE = wxID_HIGHEST + 1,
        ID_EXPORT_RANGE,
        ID_DELETE_RANGE,
        ID_DELETE_GLYPH,
        ID_EXIT,
        ID_RANGE_LIST,
        ID_GLYPH_TABLE
    };

    // Helper methods for import functionality
    static bool ParseRangeFromFilename(const wxString& filepath, uint32_t& rangeStart, uint32_t& rangeEnd);
    static bool ShowBaseCharacterDialog(wxWindow* parent, const wxString& scriptDescription, uint32_t& baseCharacter, int& colorFormat);
    static bool ImportFileToRange(const wxString& filepath, uint32_t baseCharacter, int colorFormat, FontData::Range& fullRange);
    static bool AddRangesToFontData(wxWindow* parent, FontData& fontData, const std::vector<FontData::Range>& newRanges);

    wxDECLARE_EVENT_TABLE();
}; 