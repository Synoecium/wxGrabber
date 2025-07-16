#include "../include/FontEditDialog.h"
#include "../include/log.h"
#include <limits>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/dataview.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/filename.h>
#include <algorithm>

// GlyphBitmapRenderer implementation
GlyphBitmapRenderer::GlyphBitmapRenderer() : wxDataViewCustomRenderer("wxBitmap", wxDATAVIEW_CELL_INERT) {
}

bool GlyphBitmapRenderer::SetValue(const wxVariant& value) {
    if (value.GetType() == "wxBitmap") {
        m_bitmap << value;
        return true;
    }
    return false;
}

bool GlyphBitmapRenderer::GetValue(wxVariant& value) const {
    value << m_bitmap;
    return true;
}

bool GlyphBitmapRenderer::Render(wxRect rect, wxDC* dc, int state) {
    if (m_bitmap.IsOk()) {
        // Center the bitmap in the cell
        int x = rect.x + (rect.width - m_bitmap.GetWidth()) / 2;
        int y = rect.y + (rect.height - m_bitmap.GetHeight()) / 2;
        dc->DrawBitmap(m_bitmap, x, y, true);
    }
    return true;
}

wxSize GlyphBitmapRenderer::GetSize() const {
    if (m_bitmap.IsOk()) {
        return wxSize(m_bitmap.GetWidth() + 4, m_bitmap.GetHeight() + 4);
    }
    return wxSize(20, 20); // Default size
}

// GlyphDataModel implementation
GlyphDataModel::GlyphDataModel() : m_range(nullptr) {
}

void GlyphDataModel::SetRange(const FontData::Range* range) {
    m_range = range;
    Reset(m_range ? (m_range->end - m_range->start + 1) : 0);
}

void GlyphDataModel::Clear() {
    m_range = nullptr;
    Reset(0);
}

unsigned int GlyphDataModel::GetColumnCount() const {
    return 4; // Unicode, Size, Char, Glyph
}

wxString GlyphDataModel::GetColumnType(unsigned int col) const {
    if (col == 3) {
        return "wxBitmap"; // For glyph images
    }
    return "string";
}

void GlyphDataModel::GetValueByRow(wxVariant& variant, unsigned int row, unsigned int col) const {
    if (!m_range || row >= (m_range->end - m_range->start + 1)) {
        variant = wxString();
        return;
    }
    
    uint32_t codepoint = m_range->start + row;
    size_t glyphIndex = row;
    
    switch (col) {
        case 0: // Unicode
            variant = wxString::Format("U+%04X", codepoint);
            break;
        case 1: // Size
            if (glyphIndex < m_range->glyphs.size()) {
                const FontData::Glyph& glyph = m_range->glyphs[glyphIndex];
                variant = wxString::Format("%dx%d", glyph.width, glyph.height);
            } else {
                variant = "8x8";
            }
            break;
        case 2: // Char
            if (codepoint == 0) {
                variant = "NULL";
            } else if (codepoint < 32) {
                variant = wxString::Format("^%c", codepoint + 64);
            } else {
                variant = wxString(wxChar(codepoint));
            }
            break;
        case 3: // Glyph - return bitmap
            {
                wxBitmap glyphBitmap = FontData::createGlyphDisplayBitmap(*m_range, glyphIndex, 16);
                variant << glyphBitmap;
            }
            break;
        default:
            variant = wxString();
    }
}

bool GlyphDataModel::SetValueByRow(const wxVariant& variant, unsigned int row, unsigned int col) {
    // Read-only model
    return false;
}

unsigned int GlyphDataModel::GetCount() const {
    return m_range ? (m_range->end - m_range->start + 1) : 0;
}

// BaseCharacterDialog implementation
wxBEGIN_EVENT_TABLE(BaseCharacterDialog, wxDialog)
    EVT_BUTTON(wxID_OK, BaseCharacterDialog::OnOK)
    EVT_BUTTON(wxID_CANCEL, BaseCharacterDialog::OnCancel)
    EVT_TEXT(ID_TEXT_CTRL, BaseCharacterDialog::OnTextChanged)
    EVT_SPINCTRL(ID_SPIN_CTRL, BaseCharacterDialog::OnSpinChanged)
    EVT_CHOICE(ID_COLOR_FORMAT_CHOICE, BaseCharacterDialog::OnColorFormatChanged)
    EVT_CHAR_HOOK(BaseCharacterDialog::OnKeyDown)
wxEND_EVENT_TABLE()

BaseCharacterDialog::BaseCharacterDialog(wxWindow* parent, const wxString& scriptDescription)
    : wxDialog(parent, wxID_ANY, "Import Range - Base Character", wxDefaultPosition, wxSize(450, 400)),
      m_baseCharacter(32), m_colorFormat(1)  // Default to monochrome
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // If script description is provided, show it first
    if (!scriptDescription.IsEmpty()) {
        wxStaticText* scriptLabel = new wxStaticText(this, wxID_ANY, "Script File Ranges:");
        mainSizer->Add(scriptLabel, 0, wxALL, 10);
        
        wxTextCtrl* scriptText = new wxTextCtrl(this, wxID_ANY, scriptDescription,
                                               wxDefaultPosition, wxSize(-1, 80),
                                               wxTE_MULTILINE | wxTE_READONLY | wxTE_WORDWRAP);
        scriptText->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
        mainSizer->Add(scriptText, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);
        
        // Add separator
        wxStaticLine* separator = new wxStaticLine(this);
        mainSizer->Add(separator, 0, wxLEFT | wxRIGHT | wxEXPAND, 10);
        mainSizer->AddSpacer(5);
    }
    
    // Only show base character input if no script description (ranges will be defined by script)
    if (scriptDescription.IsEmpty()) {
        // Instructions
        wxStaticText* instructions = new wxStaticText(this, wxID_ANY, 
            "Enter the Unicode value for the first character in the bitmap:");
        mainSizer->Add(instructions, 0, wxALL | wxEXPAND, 10);
        
        // Input controls
        wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
        
        // Text input for hex value
        wxStaticText* hexLabel = new wxStaticText(this, wxID_ANY, "Hex (U+):");
        inputSizer->Add(hexLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        
        m_textCtrl = new wxTextCtrl(this, ID_TEXT_CTRL, "0020", wxDefaultPosition, wxSize(80, -1));
        inputSizer->Add(m_textCtrl, 0, wxRIGHT, 10);
        
        // Decimal input
        wxStaticText* decLabel = new wxStaticText(this, wxID_ANY, "Decimal:");
        inputSizer->Add(decLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
        
        m_spinCtrl = new wxSpinCtrl(this, ID_SPIN_CTRL, "32", wxDefaultPosition, wxSize(80, -1), 
                                   wxSP_ARROW_KEYS, 0, 0x10FFFF, 32);
        inputSizer->Add(m_spinCtrl, 0, 0);
        
        mainSizer->Add(inputSizer, 0, wxALL | wxEXPAND, 10);
    } else {
        // Create hidden controls for script mode but don't add them to the layout
        m_textCtrl = new wxTextCtrl(this, ID_TEXT_CTRL, "0020", wxDefaultPosition, wxSize(80, -1));
        m_spinCtrl = new wxSpinCtrl(this, ID_SPIN_CTRL, "32", wxDefaultPosition, wxSize(80, -1), 
                                   wxSP_ARROW_KEYS, 0, 0x10FFFF, 32);
        // Hide the controls
        m_textCtrl->Hide();
        m_spinCtrl->Hide();
        
        // Show instruction for script mode
        wxStaticText* instructions = new wxStaticText(this, wxID_ANY, 
            "Character ranges are defined by the script file. Choose the color format:");
        mainSizer->Add(instructions, 0, wxALL | wxEXPAND, 10);
    }
    
    // Color format selection
    wxStaticText* formatLabel = new wxStaticText(this, wxID_ANY, "Color Format:");
    mainSizer->Add(formatLabel, 0, wxLEFT | wxRIGHT | wxTOP, 10);
    
    m_colorFormatChoice = new wxChoice(this, ID_COLOR_FORMAT_CHOICE);
    m_colorFormatChoice->Append("Monochrome (1-bit)", new wxStringClientData("1"));
    m_colorFormatChoice->Append("Indexed Color (8-bit palette)", new wxStringClientData("0"));
    m_colorFormatChoice->Append("True Color (24-bit RGB)", new wxStringClientData("24"));
    m_colorFormatChoice->Append("True Color with Alpha (32-bit RGBA)", new wxStringClientData("-32"));
    m_colorFormatChoice->SetSelection(0);  // Default to monochrome
    mainSizer->Add(m_colorFormatChoice, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 10);
    
    // Character preview (only show if not using script ranges)
    if (scriptDescription.IsEmpty()) {
        wxStaticText* previewLabel = new wxStaticText(this, wxID_ANY, "Character: ' ' (space)");
        previewLabel->SetName("preview");
        mainSizer->Add(previewLabel, 0, wxALL, 10);
        
        // Store reference for easy access
        m_previewLabel = previewLabel;
    } else {
        // Create hidden preview for script mode
        m_previewLabel = new wxStaticText(this, wxID_ANY, "Character: ' ' (space)");
        m_previewLabel->Hide();
    }
    
    // Add some space before buttons
    mainSizer->AddSpacer(10);
    
    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    
    wxButton* okButton = new wxButton(this, wxID_OK, "OK (Shift+Enter)");
    buttonSizer->Add(okButton, 0, wxRIGHT, 5);
    
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    buttonSizer->Add(cancelButton, 0, 0);
    
    buttonSizer->AddStretchSpacer();
    
    mainSizer->Add(buttonSizer, 0, wxALL | wxEXPAND, 10);
    
    SetSizer(mainSizer);
    
    // Set minimum size to ensure all controls are visible
    if (!scriptDescription.IsEmpty()) {
        SetMinSize(wxSize(450, 350));  // Smaller since no base character input
        SetSize(wxSize(450, 350));
    } else {
        SetMinSize(wxSize(380, 300));
        // Resize dialog to fit content when no script description
        SetSize(wxSize(380, 300));
    }
    
    CentreOnParent();
    
    // Set focus to text control
    m_textCtrl->SetFocus();
    m_textCtrl->SelectAll();
}

void BaseCharacterDialog::OnOK(wxCommandEvent& event)
{
    // Get the final values
    m_baseCharacter = m_spinCtrl->GetValue();
    
    // Get color format from choice selection
    int selection = m_colorFormatChoice->GetSelection();
    if (selection != wxNOT_FOUND) {
        wxStringClientData* data = static_cast<wxStringClientData*>(m_colorFormatChoice->GetClientObject(selection));
        if (data) {
            long format;
            if (data->GetData().ToLong(&format)) {
                m_colorFormat = static_cast<int>(format);
            }
        }
    }
    
    EndModal(wxID_OK);
}

void BaseCharacterDialog::OnCancel(wxCommandEvent& event)
{
    EndModal(wxID_CANCEL);
}

void BaseCharacterDialog::OnColorFormatChanged(wxCommandEvent& event)
{
    // Update the internal color format when selection changes
    int selection = m_colorFormatChoice->GetSelection();
    if (selection != wxNOT_FOUND) {
        wxStringClientData* data = static_cast<wxStringClientData*>(m_colorFormatChoice->GetClientObject(selection));
        if (data) {
            long format;
            if (data->GetData().ToLong(&format)) {
                m_colorFormat = static_cast<int>(format);
            }
        }
    }
}

void BaseCharacterDialog::OnTextChanged(wxCommandEvent& event)
{
    // Parse hex input
    wxString hexStr = m_textCtrl->GetValue();
    unsigned long value;
    if (hexStr.ToULong(&value, 16) && value <= 0x10FFFF) {
        m_spinCtrl->SetValue(value);
        m_baseCharacter = value;
        
        // Update character preview
        if (m_previewLabel) {
            if (m_baseCharacter == 0) {
                m_previewLabel->SetLabel("Character: NULL");
            } else if (m_baseCharacter < 32) {
                m_previewLabel->SetLabel(wxString::Format("Character: ^%c (control)", m_baseCharacter + 64));
            } else if (m_baseCharacter <= 126) {
                m_previewLabel->SetLabel(wxString::Format("Character: '%c'", (char)m_baseCharacter));
            } else {
                m_previewLabel->SetLabel(wxString::Format("Character: U+%04X", m_baseCharacter));
            }
        }
    }
}

void BaseCharacterDialog::OnSpinChanged(wxSpinEvent& event)
{
    // Update hex display
    uint32_t value = m_spinCtrl->GetValue();
    m_textCtrl->SetValue(wxString::Format("%04X", value));
    m_baseCharacter = value;
    
    // Update character preview
    if (m_previewLabel) {
        if (m_baseCharacter == 0) {
            m_previewLabel->SetLabel("Character: NULL");
        } else if (m_baseCharacter < 32) {
            m_previewLabel->SetLabel(wxString::Format("Character: ^%c (control)", m_baseCharacter + 64));
        } else if (m_baseCharacter <= 126) {
            m_previewLabel->SetLabel(wxString::Format("Character: '%c'", (char)m_baseCharacter));
        } else {
            m_previewLabel->SetLabel(wxString::Format("Character: U+%04X", m_baseCharacter));
        }
    }
}

void BaseCharacterDialog::OnKeyDown(wxKeyEvent& event)
{
    if ((event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER) && event.ShiftDown()) {
        // Trigger the OK button on Shift+Enter
        wxCommandEvent okEvent(wxEVT_COMMAND_BUTTON_CLICKED, wxID_OK);
        ProcessEvent(okEvent);
    } else if (event.GetKeyCode() == WXK_ESCAPE) {
        // Trigger the Cancel button
        wxCommandEvent cancelEvent(wxEVT_COMMAND_BUTTON_CLICKED, wxID_CANCEL);
        ProcessEvent(cancelEvent);
    } else {
        // Let other keys be processed normally
        event.Skip();
    }
}

void BaseCharacterDialog::SetBaseCharacter(uint32_t baseChar)
{
    m_baseCharacter = baseChar;
    
    // Update the text control and spin control
    m_textCtrl->SetValue(wxString::Format("%04X", baseChar));
    m_spinCtrl->SetValue(baseChar);
    
    // Update character preview
    if (m_previewLabel) {
        if (m_baseCharacter == 0) {
            m_previewLabel->SetLabel("Character: NULL");
        } else if (m_baseCharacter < 32) {
            m_previewLabel->SetLabel(wxString::Format("Character: ^%c (control)", m_baseCharacter + 64));
        } else if (m_baseCharacter <= 126) {
            m_previewLabel->SetLabel(wxString::Format("Character: '%c'", (char)m_baseCharacter));
        } else {
            m_previewLabel->SetLabel(wxString::Format("Character: U+%04X", m_baseCharacter));
        }
    }
}

wxBEGIN_EVENT_TABLE(FontEditDialog, wxDialog)
    EVT_BUTTON(ID_IMPORT_RANGE, FontEditDialog::OnImportRange)
    EVT_BUTTON(ID_EXPORT_RANGE, FontEditDialog::OnExportRange)
    EVT_BUTTON(ID_DELETE_RANGE, FontEditDialog::OnDeleteRange)
    EVT_BUTTON(ID_DELETE_GLYPH, FontEditDialog::OnDeleteGlyph)
    EVT_BUTTON(ID_EXIT, FontEditDialog::OnExit)
    EVT_LIST_ITEM_SELECTED(ID_RANGE_LIST, FontEditDialog::OnRangeSelected)
    EVT_LIST_ITEM_DESELECTED(ID_RANGE_LIST, FontEditDialog::OnRangeDeselected)
    EVT_LIST_KEY_DOWN(ID_RANGE_LIST, FontEditDialog::OnRangeKeyDown)
    EVT_DATAVIEW_SELECTION_CHANGED(ID_GLYPH_TABLE, FontEditDialog::OnGlyphSelected)
    EVT_CHAR_HOOK(FontEditDialog::OnKeyDown)
wxEND_EVENT_TABLE()

FontEditDialog::FontEditDialog(wxWindow* parent, FontData& fontData)
    : wxDialog(parent, wxID_ANY, "Font Editor", wxDefaultPosition, wxSize(700, 600), 
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_fontData(fontData)
{
    // Create main sizer (vertical layout)
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create top panel with horizontal layout for range list and buttons
    wxPanel* topPanel = new wxPanel(this);
    wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);

    // Create left panel for the range list
    wxPanel* leftPanel = new wxPanel(topPanel);
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);

    // Create the range list control
    m_rangeList = new wxListCtrl(leftPanel, ID_RANGE_LIST, wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL);

    // Add columns to match the screenshot
    m_rangeList->InsertColumn(0, "Range", wxLIST_FORMAT_LEFT, 140);
    m_rangeList->InsertColumn(1, "Size", wxLIST_FORMAT_LEFT, 60);
    m_rangeList->InsertColumn(2, "Start", wxLIST_FORMAT_LEFT, 60);
    m_rangeList->InsertColumn(3, "End", wxLIST_FORMAT_LEFT, 60);
    m_rangeList->InsertColumn(4, "Color", wxLIST_FORMAT_LEFT, 140);

    leftSizer->Add(m_rangeList, 1, wxEXPAND | wxALL, 5);
    leftPanel->SetSizer(leftSizer);

    // Create right panel for buttons
    wxPanel* rightPanel = new wxPanel(topPanel);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);

    // Create buttons to match the screenshot
    m_importButton = new wxButton(rightPanel, ID_IMPORT_RANGE, "Import Range");
    m_exportButton = new wxButton(rightPanel, ID_EXPORT_RANGE, "Export Range");
    m_deleteButton = new wxButton(rightPanel, ID_DELETE_RANGE, "Delete Range");
    m_exitButton = new wxButton(rightPanel, ID_EXIT, "Exit");

    // Set button sizes to match screenshot proportions
    wxSize buttonSize(120, 30);
    m_importButton->SetMinSize(buttonSize);
    m_exportButton->SetMinSize(buttonSize);
    m_deleteButton->SetMinSize(buttonSize);
    m_exitButton->SetMinSize(buttonSize);

    // Add buttons with spacing
    rightSizer->Add(m_importButton, 0, wxEXPAND | wxALL, 5);
    rightSizer->Add(m_exportButton, 0, wxEXPAND | wxALL, 5);
    rightSizer->Add(m_deleteButton, 0, wxEXPAND | wxALL, 5);
    rightSizer->AddSpacer(20); // Add some space before Exit button
    rightSizer->Add(m_exitButton, 0, wxEXPAND | wxALL, 5);
    rightSizer->AddStretchSpacer(); // Push buttons to top

    rightPanel->SetSizer(rightSizer);

    // Add panels to top sizer
    topSizer->Add(leftPanel, 1, wxEXPAND | wxALL, 5);
    topSizer->Add(rightPanel, 0, wxEXPAND | wxALL, 5);
    topPanel->SetSizer(topSizer);

    // Create bottom panel for glyph table
    wxPanel* bottomPanel = new wxPanel(this);
    wxBoxSizer* bottomSizer = new wxBoxSizer(wxVERTICAL);

    // Create a horizontal sizer for glyph table and buttons
    wxBoxSizer* glyphSizer = new wxBoxSizer(wxHORIZONTAL);

    // Create the glyph table control
    m_glyphTable = new wxDataViewCtrl(bottomPanel, ID_GLYPH_TABLE, wxDefaultPosition, wxDefaultSize,
                                     wxDV_SINGLE | wxDV_ROW_LINES | wxDV_HORIZ_RULES | wxDV_VERT_RULES);

    // Create and set the model
    m_glyphModel = new GlyphDataModel();
    m_glyphTable->AssociateModel(m_glyphModel);

    // Create image list for glyph rendering (16x16 images)
    m_glyphImageList = new wxImageList(16, 16, true);

    // Add columns for glyph details
    m_glyphTable->AppendTextColumn("Unicode", 0, wxDATAVIEW_CELL_INERT, 60);
    m_glyphTable->AppendTextColumn("Size", 1, wxDATAVIEW_CELL_INERT, 60);
    m_glyphTable->AppendTextColumn("Char", 2, wxDATAVIEW_CELL_INERT, 50);
    
    // Add custom bitmap column for glyph images
    wxDataViewColumn* glyphColumn = new wxDataViewColumn("Glyph", new GlyphBitmapRenderer(), 3, 60, wxALIGN_LEFT);
    m_glyphTable->AppendColumn(glyphColumn);

    // Bind key event for glyph table
    m_glyphTable->Bind(wxEVT_KEY_DOWN, &FontEditDialog::OnGlyphKeyDown, this);

    // Add glyph table to horizontal sizer
    glyphSizer->Add(m_glyphTable, 1, wxEXPAND | wxALL, 5);

    // Create vertical sizer for glyph buttons
    wxBoxSizer* glyphButtonSizer = new wxBoxSizer(wxVERTICAL);

    // Create delete glyph button
    m_deleteGlyphButton = new wxButton(bottomPanel, ID_DELETE_GLYPH, "Delete Glyph");
    m_deleteGlyphButton->SetMinSize(wxSize(100, 30));
    m_deleteGlyphButton->Enable(false); // Initially disabled

    glyphButtonSizer->Add(m_deleteGlyphButton, 0, wxEXPAND | wxALL, 5);
    glyphButtonSizer->AddStretchSpacer(); // Push button to top

    // Add button sizer to horizontal sizer
    glyphSizer->Add(glyphButtonSizer, 0, wxEXPAND | wxALL, 5);

    // Add horizontal sizer to bottom panel
    bottomSizer->Add(glyphSizer, 1, wxEXPAND);
    bottomPanel->SetSizer(bottomSizer);

    // Add panels to main sizer
    mainSizer->Add(topPanel, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(bottomPanel, 1, wxEXPAND | wxALL, 5);

    SetSizer(mainSizer);

    // Populate the range list
    PopulateRangeList();
    
    // Select the first range by default if available
    if (m_rangeList->GetItemCount() > 0) {
        m_rangeList->SetItemState(0, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
        m_rangeList->EnsureVisible(0);
        // Populate glyph table for the selected range
        PopulateGlyphTable();
    }
    
    // Update button states
    UpdateButtonStates();
    UpdateGlyphButtonStates();

    // Center the dialog
    CentreOnParent();
}

FontEditDialog::~FontEditDialog()
{
    if (m_glyphImageList) {
        delete m_glyphImageList;
        m_glyphImageList = nullptr;
    }
    
    if (m_glyphModel) {
        m_glyphModel->DecRef();
        m_glyphModel = nullptr;
    }
}

void FontEditDialog::PopulateRangeList()
{
    m_rangeList->DeleteAllItems();

    for (size_t i = 0; i < m_fontData.ranges.size(); ++i) {
        const FontData::Range& range = m_fontData.ranges[i];
        
        // Insert new item
        long index = m_rangeList->InsertItem(i, FontData::formatUnicodeRange(range.start, range.end));
        
        // Set the size (number of glyphs in range)
        int glyphCount = range.end - range.start + 1;
        m_rangeList->SetItem(index, 1, wxString::Format("%d", glyphCount));
        
        // Set start character
        m_rangeList->SetItem(index, 2, wxString::Format("0x%04X", range.start));
        
        // Set end character  
        m_rangeList->SetItem(index, 3, wxString::Format("0x%04X", range.end));
        
        // Set color depth
        m_rangeList->SetItem(index, 4, FontData::getColorDepthString(range.mono));
    }
}

void FontEditDialog::UpdateButtonStates()
{
    bool hasSelection = m_rangeList->GetSelectedItemCount() > 0;
    m_exportButton->Enable(hasSelection);
    m_deleteButton->Enable(hasSelection);
}

void FontEditDialog::UpdateGlyphButtonStates()
{
    // Enable delete glyph button only if a glyph is selected
    wxDataViewItem selection = m_glyphTable->GetSelection();
    bool hasGlyphSelection = selection.IsOk();
    m_deleteGlyphButton->Enable(hasGlyphSelection);
}

void FontEditDialog::OnGlyphSelected(wxDataViewEvent& event)
{
    UpdateGlyphButtonStates();
}

void FontEditDialog::OnGlyphDeselected(wxDataViewEvent& event)
{
    UpdateGlyphButtonStates();
}

void FontEditDialog::OnDeleteGlyph(wxCommandEvent& event)
{
    if (DeleteSelectedGlyph()) {
        // Refresh the range list to update counts
        PopulateRangeList();
        
        // Select the same range as before (if it still exists)
        long selectedRangeItem = m_rangeList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selectedRangeItem >= 0 && selectedRangeItem < (long)m_fontData.ranges.size()) {
            PopulateGlyphTable();
            
            // Try to select the same glyph index or the previous one if this was the last
            int glyphCount = m_fontData.ranges[selectedRangeItem].glyphs.size();
            if (glyphCount > 0) {
                // Get the row that was previously selected
                wxDataViewItem selection = m_glyphTable->GetSelection();
                int selectedRow = selection.IsOk() ? m_glyphModel->GetRow(selection) : 0;
                
                // Ensure we select a valid row
                if (selectedRow >= glyphCount) {
                    selectedRow = glyphCount - 1; // Select last glyph if we deleted the last one
                }
                
                // Select the glyph
                if (selectedRow >= 0) {
                    wxDataViewItem newSelection = m_glyphModel->GetItem(selectedRow);
                    if (newSelection.IsOk()) {
                        m_glyphTable->Select(newSelection);
                        m_glyphTable->EnsureVisible(newSelection);
                    }
                }
            }
        } else {
            // Range was deleted, clear glyph table
            m_glyphModel->Clear();
            
            // Select appropriate range if any remain
            if (m_rangeList->GetItemCount() > 0) {
                long newSelection = std::min(selectedRangeItem, (long)(m_rangeList->GetItemCount() - 1));
                m_rangeList->SetItemState(newSelection, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                m_rangeList->EnsureVisible(newSelection);
                PopulateGlyphTable();
            }
        }
        
        UpdateButtonStates();
        UpdateGlyphButtonStates();
    }
}

bool FontEditDialog::DeleteSelectedGlyph()
{
    // Get selected glyph from the data view
    wxDataViewItem selection = m_glyphTable->GetSelection();
    if (!selection.IsOk()) {
        return false; // No selection
    }
    
    // Get the row number from the selection
    int selectedRow = m_glyphModel->GetRow(selection);
    if (selectedRow < 0) {
        return false; // Invalid row
    }
    
    // Get the selected range
    long selectedRangeItem = m_rangeList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selectedRangeItem == -1 || selectedRangeItem >= (long)m_fontData.ranges.size()) {
        return false; // No range selected
    }
    
    FontData::Range& range = m_fontData.ranges[selectedRangeItem];
    
    if (selectedRow >= (int)range.glyphs.size()) {
        return false; // Invalid glyph index
    }
    
    // Get character code for this glyph
    uint32_t characterCode = range.start + selectedRow;
    
    // Confirm deletion
    wxMessageDialog confirmDialog(this, 
        wxString::Format("Are you sure you want to delete glyph at U+%04X?", characterCode),
        "Confirm Delete Glyph", 
        wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
    
    if (confirmDialog.ShowModal() != wxID_YES) {
        return false; // User cancelled
    }

    // Remove the glyph from the range
    range.glyphs.erase(range.glyphs.begin() + selectedRow);
    
    // Update the range end character (since we removed a glyph)
    if (range.glyphs.empty()) {
        // If no glyphs left, remove the entire range
        m_fontData.ranges.erase(m_fontData.ranges.begin() + selectedRangeItem);
        logInfo("Deleted empty range after removing last glyph");
    } else {
        // Adjust the end character
        range.end = range.start + range.glyphs.size() - 1;
        logInfo(wxString::Format("Deleted glyph at U+%04X, range now: U+%04X-U+%04X", 
                                characterCode, range.start, range.end).ToStdString());
    }

    // Update total glyph count
    int totalGlyphs = 0;
    for (const auto& r : m_fontData.ranges) {
        totalGlyphs += r.glyphs.size();
    }
    m_fontData.glyphCount = totalGlyphs;

    return true;
}

// Helper function implementations
bool FontEditDialog::ParseRangeFromFilename(const wxString& filepath, uint32_t& rangeStart, uint32_t& rangeEnd)
{
    rangeStart = 0x20; // Default start value
    rangeEnd = std::numeric_limits<uint32_t>::max();
    
    bool hasRangeInfo = FontData::ParseRangeFromFilename(filepath, rangeStart, rangeEnd);
    if (hasRangeInfo) {
        logInfo("Auto-detected range from filename: U+" + wxString::Format("%04X-U+%04X", rangeStart, rangeEnd).ToStdString());
    } else {
        logInfo("No range information found in filename, using default start: U+0020");
    }
    
    return hasRangeInfo;
}

bool FontEditDialog::ShowBaseCharacterDialog(wxWindow* parent, const wxString& scriptDescription, uint32_t& baseCharacter, int& colorFormat)
{
    BaseCharacterDialog* charDialog = nullptr;
    
    if (!scriptDescription.IsEmpty()) {
        // Show dialog with script ranges info
        charDialog = new BaseCharacterDialog(parent, scriptDescription);
        charDialog->SetBaseCharacter(baseCharacter);
        
        if (charDialog->ShowModal() != wxID_OK) {
            delete charDialog;
            return false; // User cancelled
        }
        
        baseCharacter = charDialog->GetBaseCharacter();
    } else {
        // Show base character dialog for single range
        charDialog = new BaseCharacterDialog(parent, wxEmptyString);
        charDialog->SetBaseCharacter(baseCharacter);
        
        if (charDialog->ShowModal() != wxID_OK) {
            delete charDialog;
            return false; // User cancelled
        }
        
        baseCharacter = charDialog->GetBaseCharacter();
    }
    
    colorFormat = charDialog->GetColorFormat();
    delete charDialog;
    logDebug(wxString::Format("Base character: %04X, color format: %d", baseCharacter, colorFormat).ToStdString());
    
    return true;
}

bool FontEditDialog::ImportFileToRange(const wxString& filepath, uint32_t baseCharacter, int colorFormat, FontData::Range& fullRange)
{
    wxString extension = wxFileName(filepath).GetExt().Lower();
    
    if (extension == "fnt") {
        fullRange = FontData::ImportFntAsRange(filepath, baseCharacter);
    } else {
        fullRange = FontData::ImportBitmapAsRange(filepath, baseCharacter, colorFormat);
    }
    
    if (fullRange.glyphs.empty()) {
        logError("Failed to import file: " + filepath.ToStdString());
        return false;
    }
    
    logDebug(wxString::Format("Imported %d glyphs from %s", 
                        (int)fullRange.glyphs.size(), filepath).ToStdString());
    
    return true;
}

bool FontEditDialog::AddRangesToFontData(wxWindow* parent, FontData& fontData, const std::vector<FontData::Range>& newRanges)
{
    // Check for range overlap
    if (!CheckRangeOverlap(parent, fontData, newRanges)) {
        logWarning("User cancelled due to range overlap");
        return false;
    }
    
    // Add the new ranges to the font data
    for (const auto& range : newRanges) {
        fontData.ranges.push_back(range);
    }
    
    fontData.UpdateGlyphCount();
    
    logInfo(wxString::Format("Successfully added %d ranges to font data", (int)newRanges.size()).ToStdString());
    return true;
}

// Static helper method for font import functionality (reusable by main frame)
bool FontEditDialog::ImportFontRange(wxWindow* parent, FontData& fontData, const wxString& operationName, wxString* outFilename)
{
    // Show file dialog for import
    wxFileDialog fileDialog(parent, wxString::Format("Select file to %s", operationName),
                           "", "", 
                           "Font files (*.fnt)|*.fnt|"
                           "BMP images (*.bmp)|*.bmp|"
                           "PNG images (*.png)|*.png|"
                           "TGA images (*.tga)|*.tga|"
                           "PCX images (*.pcx)|*.pcx|"
                           "All files (*.*)|*.*",
                           wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (fileDialog.ShowModal() != wxID_OK) {
        return false; // User cancelled
    }
    
    wxString filepath = fileDialog.GetPath();
    if (outFilename) {
        *outFilename = filepath;
    }
    
    // Step 1: Try to parse range information from filename
    uint32_t rangeStart, rangeEnd;
    ParseRangeFromFilename(filepath, rangeStart, rangeEnd);
    
    // Step 2: Show base character dialog for single range import
    uint32_t baseCharacter = rangeStart;
    int colorFormat;
    if (!ShowBaseCharacterDialog(parent, wxEmptyString, baseCharacter, colorFormat)) {
        return false; // User cancelled
    }
    
    // Step 3: Import the file to get all glyphs
    FontData::Range fullRange;
    if (!ImportFileToRange(filepath, baseCharacter, colorFormat, fullRange)) {
        return false;
    }
    
    // Update color format from imported range
    colorFormat = fullRange.mono;
    
    // Step 4: Create single range from imported glyphs
    FontData::Range newRange;
    newRange.mono = colorFormat;
    newRange.start = baseCharacter;
    
    // Determine end character based on available glyphs
    if (rangeEnd == std::numeric_limits<uint32_t>::max()) {
        // Use all available glyphs
        newRange.end = baseCharacter + fullRange.glyphs.size() - 1;
        logDebug(wxString::Format("Using all %d glyphs for range U+%04X-U+%04X", 
                            (int)fullRange.glyphs.size(), newRange.start, newRange.end).ToStdString());
    } else {
        // Use specified range, but limit to available glyphs
        uint32_t requestedSize = rangeEnd - baseCharacter + 1;
        if (requestedSize <= fullRange.glyphs.size()) {
            newRange.end = rangeEnd;
            logDebug(wxString::Format("Using requested range U+%04X-U+%04X (%d glyphs)", 
                                newRange.start, newRange.end, requestedSize).ToStdString());
        } else {
            newRange.end = baseCharacter + fullRange.glyphs.size() - 1;
            logWarning(wxString::Format("Requested range U+%04X-U+%04X exceeds available glyphs, using U+%04X-U+%04X", 
                                    baseCharacter, rangeEnd, newRange.start, newRange.end).ToStdString());
        }
    }
    
    // Copy glyphs for the range
    uint32_t rangeSize = newRange.end - newRange.start + 1;
    newRange.glyphs.assign(fullRange.glyphs.begin(), 
                          fullRange.glyphs.begin() + std::min(rangeSize, (uint32_t)fullRange.glyphs.size()));
    
    logDebug(wxString::Format("Created range U+%04X-U+%04X with %d glyphs", 
                        newRange.start, newRange.end, (int)newRange.glyphs.size()).ToStdString());
    
    // Step 5: Add the range to font data and update glyph count
    std::vector<FontData::Range> newRanges = {newRange};
    if (!AddRangesToFontData(parent, fontData, newRanges)) {
        return false;
    }
    
    logInfo(wxString::Format("Successfully imported range U+%04X-U+%04X with %d glyphs", 
                        newRange.start, newRange.end, (int)newRange.glyphs.size()).ToStdString());
    return true;
}

bool FontEditDialog::GrabFontFromFile(wxWindow* parent, FontData& fontData, const wxString& filepath)
{
    wxString extension = wxFileName(filepath).GetExt().Lower();
    
    // Step 1: Check for script file with the same name in the same folder then parse ranges from it if present
    wxString scriptDescription;
    std::vector<FontData::ScriptRange> scriptRanges;
    wxString scriptFile = wxFileName(filepath).GetPathWithSep() + wxFileName(filepath).GetName() + ".txt";
    bool scriptParsed = false;
    
    if (wxFileName::FileExists(scriptFile)) {
        if (FontData::ParseScriptFile(scriptFile, scriptRanges) && !scriptRanges.empty()) {
            logInfo(wxString::Format("Found script file with %d ranges: %s", 
                                (int)scriptRanges.size(), scriptFile).ToStdString());

            // Filter out ranges that don't match the current filename
            wxFileName selectedFile(filepath);
            scriptRanges.erase(std::remove_if(scriptRanges.begin(), scriptRanges.end(), [&](const FontData::ScriptRange& range) {
                wxFileName rangeFilename(range.filename);
                return rangeFilename.GetFullName().CmpNoCase(selectedFile.GetFullName()) != 0;
            }), scriptRanges.end());
            
            if (!scriptRanges.empty()) {
                scriptParsed = true;
                // Build description of ranges found in script file
                scriptDescription = wxString::Format("Found %d range(s) for this file:\n\n", (int)scriptRanges.size());
                for (size_t i = 0; i < scriptRanges.size(); ++i) {
                    uint32_t startCode = scriptRanges[i].start;
                    uint32_t endCode = scriptRanges[i].end;
                    scriptDescription += wxString::Format("Range %zu: U+%04X - U+%04X (%d characters)\n", 
                                                i + 1, startCode, endCode, endCode - startCode + 1);
                }
            }
        }
    }

    // Step 2: If script file is missing or there was a problem with parsing, try to parse 1 range from filename
    // and set default start with 0x20 value if it wasn't parsed from filename
    if (!scriptParsed) {
        uint32_t rangeStart, rangeEnd;
        ParseRangeFromFilename(filepath, rangeStart, rangeEnd);
        
        FontData::ScriptRange scriptRange(filepath, rangeStart, rangeEnd);
        scriptRanges.push_back(scriptRange);
    }
    
    // Step 3: Show base character dialog
    uint32_t baseCharacter = scriptRanges[0].start;
    int colorFormat;
    if (!ShowBaseCharacterDialog(parent, scriptDescription, baseCharacter, colorFormat)) {
        return false; // User cancelled
    }
    
    // Update script range if no script was parsed
    if (!scriptParsed) {
        scriptRanges[0].start = baseCharacter;
    }

    // Step 4: Import the file to get all glyphs
    FontData::Range fullRange;
    if (!ImportFileToRange(filepath, baseCharacter, colorFormat, fullRange)) {
        return false;
    }
    
    // Update color format from imported range
    colorFormat = fullRange.mono;
    
    logDebug(wxString::Format("Splitting into %d ranges", (int)scriptRanges.size()).ToStdString());
    
    // Step 5: Create separate ranges from the imported glyphs
    // Glyph count can be less than planned range - there can be a warning in logs but it still should grab range normally
    std::vector<FontData::Range> newRanges;
    int successCount = 0;
    uint32_t currentGlyphIndex = 0;
    
    for (const auto& scriptRange : scriptRanges) {
        uint32_t startCode = scriptRange.start;
        uint32_t endCode = scriptRange.end;
        
        // Adjust end code if it's the max value (unlimited range)
        if (endCode == std::numeric_limits<uint32_t>::max()) {
            endCode = startCode + fullRange.glyphs.size() - currentGlyphIndex - 1;
            logDebug(wxString::Format("Adjusted end code for range U+%04X-U+%04X to U+%04X", 
                                scriptRange.start, scriptRange.end, endCode).ToStdString());
        }
        
        uint32_t rangeSize = endCode - startCode + 1;
        logDebug(wxString::Format("Range size: %d", rangeSize).ToStdString());
        
        FontData::Range newRange;
        newRange.mono = colorFormat;
        newRange.start = startCode;
        newRange.end = endCode;
        
        // Check if we have enough glyphs left for this range
        if (currentGlyphIndex + rangeSize <= fullRange.glyphs.size()) {
            // Copy the appropriate glyphs for this range
            newRange.glyphs.assign(
                fullRange.glyphs.begin() + currentGlyphIndex,
                fullRange.glyphs.begin() + currentGlyphIndex + rangeSize
            );
            
            newRanges.push_back(newRange);
            successCount++;
            currentGlyphIndex += rangeSize;
            
            logDebug(wxString::Format("Created range U+%04X-U+%04X with %d glyphs", 
                                startCode, endCode, (int)newRange.glyphs.size()).ToStdString());
        } else {
            // Still create the range with available glyphs, but log a warning
            uint32_t availableGlyphs = fullRange.glyphs.size() - currentGlyphIndex;
            if (availableGlyphs > 0) {
                newRange.glyphs.assign(
                    fullRange.glyphs.begin() + currentGlyphIndex,
                    fullRange.glyphs.end()
                );
                newRange.end = startCode + availableGlyphs - 1;
                
                newRanges.push_back(newRange);
                successCount++;
                
                logWarning(wxString::Format("Range U+%04X-U+%04X created with only %d glyphs (requested %d)", 
                                        startCode, newRange.end, availableGlyphs, rangeSize).ToStdString());
            } else {
                logWarning(wxString::Format("Range U+%04X-U+%04X skipped - no glyphs available", 
                                        startCode, endCode).ToStdString());
            }
            currentGlyphIndex = fullRange.glyphs.size(); // Mark all glyphs as used
        }
    }
    
    // Step 6: Add ranges to font data and update glyph count
    if (successCount > 0) {
        if (!AddRangesToFontData(parent, fontData, newRanges)) {
            return false;
        }
        
        logInfo(wxString::Format("Successfully created %d ranges from grabbed file", successCount).ToStdString());
        return true;
    } else {
        logError("No ranges were successfully grabbed from file");
        return false;
    }
}

bool FontEditDialog::CheckRangeOverlap(wxWindow* parent, FontData& fontData, const std::vector<FontData::Range>& ranges)
{
    std::vector<wxString> overlappingRanges;
    for (const auto& range : ranges) {
        fontData.CheckRangeOverlap(range, overlappingRanges);
    }
    
    if (!overlappingRanges.empty()) {
        wxString message;
        for (const auto& range : ranges) {
            message += wxString::Format(
                "The new range %s overlaps with existing range(s):\n\n",
                FontData::formatUnicodeRange(range.start, range.end).ToStdString()
            );
            for (const auto& rangeStr : overlappingRanges) {
                message += "\t" + rangeStr + "\n";
            }
        }
        
        message += "\nThis may cause character conflicts. Do you want to continue anyway?";

        // Show warning dialog
        wxMessageDialog warningDialog(parent, message, "Range Overlap Warning", 
                                    wxYES_NO | wxNO_DEFAULT | wxICON_WARNING);
        
        if (warningDialog.ShowModal() == wxID_YES) {
            logWarning("User chose to import overlapping range: " + 
                      message);
            return true; // User wants to continue
        } else {
            logInfo("User cancelled import due to range overlap");
            return false; // User cancelled
        }
    }
    
    return true; // No overlap, proceed
}

void FontEditDialog::PopulateGlyphTable()
{
    // Clear the image list
    if (m_glyphImageList) {
        m_glyphImageList->RemoveAll();
    }

    // Get the selected range
    long selectedItem = m_rangeList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selectedItem == -1 || selectedItem >= (long)m_fontData.ranges.size()) {
        m_glyphModel->Clear();
        return;
    }

    const FontData::Range& range = m_fontData.ranges[selectedItem];
    
    // Set the range in the model - this will automatically populate the view
    m_glyphModel->SetRange(&range);
}

void FontEditDialog::OnImportRange(wxCommandEvent& event)
{
    // Use the static helper method for consistent import functionality
    if (ImportFontRange(this, m_fontData, "import")) {
        // Refresh the UI after successful import
        PopulateRangeList();
        UpdateButtonStates();
        
        // Select the newly imported range
        if (m_rangeList->GetItemCount() > 0) {
            long lastIndex = m_rangeList->GetItemCount() - 1;
            m_rangeList->SetItemState(lastIndex, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
            m_rangeList->EnsureVisible(lastIndex);
            PopulateGlyphTable();
        }
    }
}

void FontEditDialog::OnExportRange(wxCommandEvent& event)
{
    // Get the selected range
    long selectedItem = m_rangeList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    
    // If no range selected, offer to export all ranges
    if (selectedItem == -1 || selectedItem >= (long)m_fontData.ranges.size()) {
        if (m_fontData.ranges.empty()) {
            wxMessageBox("No ranges available to export.", "No Ranges", wxOK | wxICON_INFORMATION, this);
            return;
        }
        
        // Ask user if they want to export all ranges
        wxMessageDialog choiceDialog(this,
            wxString::Format("No range selected. Do you want to export all %d ranges?\n\n"
                           "• Single range: exports with range suffix in filename\n"
                           "• Multiple ranges: exports to one bitmap + script file",
                           (int)m_fontData.ranges.size()),
            "Export All Ranges?",
            wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
        
        if (choiceDialog.ShowModal() != wxID_YES) {
            return;
        }
        
        // Export entire font using smart export
        wxFileDialog fileDialog(this, "Export font as...",
                               "", "", 
                               "BMP images (*.bmp)|*.bmp|"
                               "PNG images (*.png)|*.png|"
                               "TGA images (*.tga)|*.tga|"
                               "PCX images (*.pcx)|*.pcx|"
                               "All files (*.*)|*.*",
                               wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        
        // Set BMP as default file type (index 0)
        fileDialog.SetFilterIndex(0);
        
        if (fileDialog.ShowModal() != wxID_OK) {
            return; // User cancelled
        }
        
        wxString filename = fileDialog.GetPath();
        
        if (m_fontData.ExportFontAsBitmap(filename)) {
            wxMessageBox("Font exported successfully!", "Export Complete", wxOK | wxICON_INFORMATION, this);
        } else {
            wxMessageBox("Failed to export font. Please check the file path and try again.",
                        "Export Error", wxOK | wxICON_ERROR, this);
        }
        return;
    }
    
    const FontData::Range& selectedRange = m_fontData.ranges[selectedItem];
    
    // Show file dialog with filters for export formats
    wxFileDialog fileDialog(this, "Export font range as...",
                           "", "", 
                           "BMP images (*.bmp)|*.bmp|"
                           "PNG images (*.png)|*.png|"
                           "TGA images (*.tga)|*.tga|"
                           "PCX images (*.pcx)|*.pcx|"
                           "Font files (*.fnt)|*.fnt|"
                           "All files (*.*)|*.*",
                           wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    // Set BMP as default file type (index 0)
    fileDialog.SetFilterIndex(0);
    
    if (fileDialog.ShowModal() != wxID_OK) {
        return; // User cancelled
    }
    
    wxString filename = fileDialog.GetPath();
    wxString extension = wxFileName(filename).GetExt().Lower();
    
    bool exportSuccess = false;
    
    // Determine export format based on extension
    if (extension == "fnt") {
        // Export as .fnt file using FontData static method
        exportSuccess = FontData::ExportRangeAsFnt(selectedRange, filename);
        if (exportSuccess) {
            logInfo("Successfully exported range " + FontData::formatUnicodeRange(selectedRange.start, selectedRange.end).ToStdString() + 
                    " to font file: " + filename.ToStdString());
        } else {
            wxMessageBox("Failed to export range as font file. Please check the file path and try again.",
                        "Export Error", wxOK | wxICON_ERROR, this);
        }
    } else if (extension == "bmp" || extension == "png" || extension == "tga" || extension == "pcx") {
        // Export as image using FontData static method
        exportSuccess = FontData::ExportRangeAsBitmap(selectedRange, filename);
        if (exportSuccess) {
            wxString formatName = extension.Upper();
            logInfo("Successfully exported range " + FontData::formatUnicodeRange(selectedRange.start, selectedRange.end).ToStdString() + 
                    " to " + formatName.ToStdString() + " image: " + filename.ToStdString());
        } else {
            wxMessageBox("Failed to export range as image. Please check the file path and try again.",
                        "Export Error", wxOK | wxICON_ERROR, this);
        }
    } else {
        // Default to bitmap export for unknown extensions using FontData static method
        exportSuccess = FontData::ExportRangeAsBitmap(selectedRange, filename);
        if (exportSuccess) {
            logInfo("Successfully exported range " + FontData::formatUnicodeRange(selectedRange.start, selectedRange.end).ToStdString() + 
                    " to file: " + filename.ToStdString());
        } else {
            wxMessageBox("Failed to export range. Please check the file path and try again.",
                        "Export Error", wxOK | wxICON_ERROR, this);
        }
    }
    
    if (exportSuccess) {
        wxMessageBox("Range exported successfully!", "Export Complete", wxOK | wxICON_INFORMATION, this);
    }
}

void FontEditDialog::OnDeleteRange(wxCommandEvent& event)
{
    long selectedItem = m_rangeList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    if (selectedItem == -1) {
        return;
    }

    // Confirm deletion
    wxMessageDialog confirmDialog(this, 
        "Are you sure you want to delete the selected range?",
        "Confirm Delete", 
        wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
    
    if (confirmDialog.ShowModal() == wxID_YES) {
        // Remove the range from the font data
        if (selectedItem < (long)m_fontData.ranges.size()) {
            m_fontData.ranges.erase(m_fontData.ranges.begin() + selectedItem);
            
            // Update glyph count
            int totalGlyphs = 0;
            for (const auto& range : m_fontData.ranges) {
                totalGlyphs += range.glyphs.size();
            }
            m_fontData.glyphCount = totalGlyphs;
            
            // Refresh the list
            PopulateRangeList();
            
            // Select appropriate range after deletion
            long newSelection = -1;
            if (m_rangeList->GetItemCount() > 0) {
                if (selectedItem < m_rangeList->GetItemCount()) {
                    newSelection = selectedItem; // Select same index if available
                } else {
                    newSelection = m_rangeList->GetItemCount() - 1; // Select last item if deleted was last
                }
                
                if (newSelection >= 0) {
                    m_rangeList->SetItemState(newSelection, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
                    m_rangeList->EnsureVisible(newSelection);
                    PopulateGlyphTable();
                } else {
                    // Clear glyph table if no ranges remain
                    m_glyphModel->Clear();
                }
            } else {
                // Clear glyph table if no ranges remain
                m_glyphModel->Clear();
            }
            
            UpdateButtonStates();
            logInfo("Range deleted successfully");
        }
    }
}

void FontEditDialog::OnExit(wxCommandEvent& event)
{
    EndModal(wxID_OK);
}

void FontEditDialog::OnRangeSelected(wxListEvent& event)
{
    PopulateGlyphTable();
    UpdateButtonStates();
}

void FontEditDialog::OnRangeDeselected(wxListEvent& event)
{
    UpdateButtonStates();
}

void FontEditDialog::OnRangeKeyDown(wxListEvent& event)
{
    if (event.GetKeyCode() == WXK_DELETE) {
        // Trigger the delete range action
        wxCommandEvent deleteEvent(wxEVT_COMMAND_BUTTON_CLICKED, ID_DELETE_RANGE);
        ProcessEvent(deleteEvent);
    } else {
        event.Skip();
    }
}

void FontEditDialog::OnGlyphKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_DELETE) {
        // Trigger the delete glyph action
        wxCommandEvent deleteEvent(wxEVT_COMMAND_BUTTON_CLICKED, ID_DELETE_GLYPH);
        ProcessEvent(deleteEvent);
    } else {
        event.Skip();
    }
}

void FontEditDialog::OnKeyDown(wxKeyEvent& event)
{
    if (event.GetKeyCode() == WXK_ESCAPE) {
        // Trigger the Exit button
        wxCommandEvent exitEvent(wxEVT_COMMAND_BUTTON_CLICKED, ID_EXIT);
        ProcessEvent(exitEvent);
    } else {
        // Let other keys be processed normally
        event.Skip();
    }
} 