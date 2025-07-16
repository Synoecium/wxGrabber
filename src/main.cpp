#include "../include/main.h"
#include "../include/DataParser.h"
#include "../include/BitmapData.h"
#include "../include/AudioData.h"
#include "../include/VideoData.h"
#include "../include/FontData.h"
#include "../include/GrabberInfo.h"
#include "../include/log.h"
#include "../include/UnitTests.h"
#include "../include/ObjectTraversalUtils.h"
#include "../include/icons.h"
#include "../include/GrabPreviewDialog.h"
#include "../include/BitmapPreviewDialog.h"
#include "../include/CommonTypes.h"
#include "../include/VideoDataPanel.h"
#include "../include/AudioPlaybackControl.h"
#include "../include/FontEditDialog.h"
#include "wx/wx.h"
#include <cstdint>
#include <cctype>
#include <wx/filename.h>
#include <wx/mediactrl.h>
#include <wx/artprov.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <wx/image.h>
#include <wx/stdpaths.h>
#include <wx/timer.h>
#include <wx/progdlg.h>
#include <algorithm>
#include <wx/colour.h>
#include <wx/clrpicker.h>
#include <vector>
#include <wx/propgrid/advprops.h>
#include <wx/textdlg.h>
#include <wx/display.h>
#include <wx/msw/winundef.h>

#if defined(__WXMSW__)
    #include <windows.h>
    #include <mmsystem.h>
    #pragma comment(lib, "winmm.lib")
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
#elif defined(__linux__) || defined(__unix__)
    #include <unistd.h>
    #include <fstream>
#endif

// Add after the includes at the top:
wxDEFINE_EVENT(wxEVT_SHOW_GRID_COMPLETE, wxCommandEvent);

// Helper function to format time in MM:SS.S format
wxString FormatTime(int milliseconds) {
    int totalSeconds = milliseconds / 1000;
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    int tenths = (milliseconds % 1000) / 100;  // Get tenths of a second
    return wxString::Format("%02d:%02d.%d", minutes, seconds, tenths);
}

// Global file stream for logging
static std::ofstream logFile;

// Helper class to store object pointer in tree items
class ObjectTreeData : public wxTreeItemData {
public:
    ObjectTreeData(std::shared_ptr<DataParser::DataObject> obj) : object(obj) {}
    std::shared_ptr<DataParser::DataObject> object;
};

// Add main function for wxWidgets application
int main(int argc, char **argv) {
    return wxEntry(argc, argv);
}

PropertyEditDialog::PropertyEditDialog(wxWindow* parent, const wxString& title, const wxString& propId, const wxString& value)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(400, -1), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      m_originalValue(value)
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create a grid sizer for aligned labels and fields
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 2, 5, 5);  // 2 rows, 2 columns, 5px spacing
    gridSizer->AddGrowableCol(1, 1);  // Make the second column growable
    
    // Add ID label and value with proper alignment
    wxStaticText* idLabel = new wxStaticText(this, wxID_ANY, "ID:");
    wxStaticText* idText = new wxStaticText(this, wxID_ANY, propId);
    gridSizer->Add(idLabel, 0, wxALIGN_LEFT | wxALL, 5);
    gridSizer->Add(idText, 1, wxEXPAND | wxALL, 5);
    
    // Add Value label and text control with proper alignment
    wxStaticText* valueLabel = new wxStaticText(this, wxID_ANY, "Value:");
    m_value = new wxTextCtrl(this, wxID_ANY, value, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    gridSizer->Add(valueLabel, 0, wxALIGN_LEFT | wxALL, 5);
    gridSizer->Add(m_value, 1, wxEXPAND | wxALL, 5);
    
    mainSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 5);
    
    // Add button sizer with OK and Cancel
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(this, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 5);
    
    SetSizer(mainSizer);
    mainSizer->Fit(this);
    
    // Set minimum size to prevent dialog from being too small
    SetMinSize(wxSize(400, -1));
    
    // Center the dialog on the parent window
    CentreOnParent();

    // Bind enter key event to close dialog with OK
    m_value->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) {
        EndModal(wxID_OK);
    });

    // Bind text change event to update title
    m_value->Bind(wxEVT_TEXT, [this](wxCommandEvent&) {
        bool isModified = m_value->GetValue() != m_originalValue;
        wxString currentTitle = GetTitle();
        bool hasAsterisk = currentTitle.EndsWith("*");
        
        if (isModified && !hasAsterisk) {
            SetTitle(currentTitle + "*");
        } else if (!isModified && hasAsterisk) {
            SetTitle(currentTitle.RemoveLast());
        }
    });

    // Set focus to the text control
    m_value->SetFocus();
}

wxIMPLEMENT_APP(MyApp);
 
bool MyApp::InitializeLogging() {
    // Open log file with immediate flush on write
    m_logFile.open("log.txt", std::ios::out | std::ios::trunc);
    if (!m_logFile.is_open()) {
        wxMessageBox("Failed to open log.txt for writing!", "Error", wxOK | wxICON_ERROR);
        return false;
    }

    // Enable automatic flushing for the log file
    m_logFile.setf(std::ios::unitbuf);

    // Store original stream buffers
    m_oldCout = std::cout.rdbuf();
    m_oldCerr = std::cerr.rdbuf();

    // Create tee streams that write to both console and file
    static TeeStream teeCout(std::cout, m_logFile);
    static TeeStream teeCerr(std::cerr, m_logFile);

    // Redirect cout and cerr to the tee streams
    std::cout.rdbuf(teeCout.rdbuf());
    std::cerr.rdbuf(teeCerr.rdbuf());

    // Enable automatic flushing for cout and cerr
    std::cout.setf(std::ios::unitbuf);
    std::cerr.setf(std::ios::unitbuf);

    return true;
}

int MyApp::OnExit() {
    // Flush the streams
    std::cout.flush();
    std::cerr.flush();

    // Restore original stream buffers
    std::cout.rdbuf(m_oldCout);
    std::cerr.rdbuf(m_oldCerr);

    // Close the log file
    if (m_logFile.is_open()) {
        m_logFile.close();
    }

    return wxApp::OnExit();
}
 
bool MyApp::OnInit()
{
    // Parse command line arguments for log level and test mode
    bool runTests = false;
    for (int i = 1; i < argc; i++) {
        std::string arg = wxString(argv[i]).ToStdString();
        if (arg == "-test") {
            runTests = true;
        } else if (arg == "-debug") {
            setLogLevel(Logger::Level::Debug);
        } else if (arg == "-verbose") {
            setLogLevel(Logger::Level::Verbose);
        } else if (arg == "-info") {
            setLogLevel(Logger::Level::Info);
        } else if (arg == "-warning") {
            setLogLevel(Logger::Level::Warning);
        } else if (arg == "-error") {
            setLogLevel(Logger::Level::Error);
        }
    }

    // Initialize logging first (before running tests)
    if (!InitializeLogging()) {
        return false;
    }
    if (!initLog("log.txt")) {
        wxMessageBox("Failed to initialize custom logging system!", "Error", wxOK | wxICON_ERROR);
        return false;
    }

    if (runTests) {
        bool lzssFileDecompressTestPassed = UnitTests::LZSSFileDecompressTest();
        bool lzssTestsPassed = UnitTests::LZSSTests();
        if (lzssFileDecompressTestPassed && lzssTestsPassed) {
            std::cout << "All compression/decompression tests passed!" << std::endl;
        } else {
            std::cout << "Tests failed:" << std::endl;
            if (!lzssFileDecompressTestPassed) std::cout << "- LZSS file decompression test failed" << std::endl;
            if (!lzssTestsPassed) std::cout << "- LZSS compression tests failed" << std::endl;
        }
        std::cout << "Check the log.txt file for detailed output." << std::endl;
        return false;
    }

    // Initialize all image handlers
    wxInitAllImageHandlers();

    MyFrame *frame = new MyFrame();
    frame->Show(true);
    return true;
}
 
MyFrame::MyFrame()
    : wxFrame(nullptr, wxID_ANY, GrabberName, wxDefaultPosition, wxSize(800, 600))
    , m_currentFilePath()
    , m_objects()
    , m_grabberInfo()
    , m_isModified(false)
    , m_AVSlider(nullptr)
    , m_loadedBitmapPath()  // Initialize the loaded bitmap path
    , m_currentPalette(BitmapData::allegro_palette)
    , m_paletteInfoText(nullptr) // Initialize the new member
    , m_videoPanel(nullptr)
    , m_videoPanelContainer(nullptr)
    , m_draggedItem()
    , m_draggedObject(nullptr)
{
    m_showIndices = false;  // Initialize index display state

    // Bind close event handler
    Bind(wxEVT_CLOSE_WINDOW, &MyFrame::OnClose, this);

    // Create the main vertical sizer for the entire window
    wxBoxSizer* verticalSizer = new wxBoxSizer(wxVERTICAL);

    // Create header panel
    wxPanel* headerPanel = new wxPanel(this, wxID_ANY);
    headerPanel->SetBackgroundColour(*wxLIGHT_GREY);
    
    wxBoxSizer* headerSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* AdditionalheaderSizer = new wxBoxSizer(wxVERTICAL);
    
    // Add file info fields
    wxFont monoFont(9, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    // Add editing info
    wxBoxSizer* editingSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* editingLabel = new wxStaticText(headerPanel, wxID_ANY, "Editing:", wxDefaultPosition, wxSize(HeaderLabelsWidth, -1));
    editingLabel->SetFont(monoFont);
    m_editingText = new wxTextCtrl(headerPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxBORDER_NONE);
    m_editingText->SetBackgroundColour(headerPanel->GetBackgroundColour());
    m_editingText->SetFont(monoFont);
    editingSizer->Add(editingLabel, 0, wxALIGN_CENTER_VERTICAL);
    editingSizer->Add(m_editingText, 1, wxALIGN_CENTER_VERTICAL );
    headerSizer->Add(editingSizer, 1, wxEXPAND | wxALL, 5);

    // Add separator line
    wxStaticLine* line = new wxStaticLine(headerPanel);
    AdditionalheaderSizer->Add(line, 0, wxEXPAND | wxALL, 2);

    // Add grid info at the top with centered alignment
    wxBoxSizer* gridSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* xGridLabel = new wxStaticText(headerPanel, wxID_ANY, "X-grid:");
    xGridLabel->SetFont(monoFont);
    m_xGridText = new wxTextCtrl(headerPanel, wxID_ANY, "3000", 
                                wxDefaultPosition, wxSize(60, -1), 
                                wxTE_PROCESS_ENTER);
    m_xGridText->SetFont(monoFont);
    wxStaticText* yGridLabel = new wxStaticText(headerPanel, wxID_ANY, "Y-grid:");
    yGridLabel->SetFont(monoFont);
    m_yGridText = new wxTextCtrl(headerPanel, wxID_ANY, "300", 
                                wxDefaultPosition, wxSize(60, -1), 
                                wxTE_PROCESS_ENTER);
    m_yGridText->SetFont(monoFont);

    // Add spacer for center alignment
    gridSizer->Add(0, 0, 1, wxEXPAND);
    gridSizer->Add(xGridLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
    gridSizer->Add(m_xGridText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
    gridSizer->Add(yGridLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 10);
    gridSizer->Add(m_yGridText, 0, wxALIGN_CENTER_VERTICAL | wxALL, 2);
    gridSizer->Add(0, 0, 1, wxEXPAND);
    AdditionalheaderSizer->Add(gridSizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 5);
    
    // Create a flex grid sizer for aligned labels and fields
    wxFlexGridSizer* fieldsSizer = new wxFlexGridSizer(3, 2, 2, 0);  // 3 rows, 2 columns, vgap, hgap
    fieldsSizer->AddGrowableCol(1, 1);  // Make the second column growable
    
    // First row: Header
    wxStaticText* headerLabel = new wxStaticText(headerPanel, wxID_ANY, "Header:", wxDefaultPosition, wxSize(HeaderLabelsWidth, -1));
    headerLabel->SetFont(monoFont);
    m_headerText = new wxTextCtrl(headerPanel, wxID_ANY, "", 
                                wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_headerText->SetFont(monoFont);
    fieldsSizer->Add(headerLabel, 0, wxALIGN_CENTER_VERTICAL);
    fieldsSizer->Add(m_headerText, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    
    // Second row: Prefix
    wxStaticText* prefixLabel = new wxStaticText(headerPanel, wxID_ANY, "Prefix:", wxDefaultPosition, wxSize(HeaderLabelsWidth, -1));
    prefixLabel->SetFont(monoFont);
    m_prefixText = new wxTextCtrl(headerPanel, wxID_ANY, "", 
                                 wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_prefixText->SetFont(monoFont);
    fieldsSizer->Add(prefixLabel, 0, wxALIGN_CENTER_VERTICAL);
    fieldsSizer->Add(m_prefixText, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);
    
    // Third row: Password
    wxStaticText* passwordLabel = new wxStaticText(headerPanel, wxID_ANY, "Password:", wxDefaultPosition, wxSize(HeaderLabelsWidth, -1));
    passwordLabel->SetFont(monoFont);
    m_passwordText = new wxTextCtrl(headerPanel, wxID_ANY, "", 
                                  wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_passwordText->SetFont(monoFont);
    fieldsSizer->Add(passwordLabel, 0, wxALIGN_CENTER_VERTICAL);
    fieldsSizer->Add(m_passwordText, 1, wxEXPAND | wxALIGN_CENTER_VERTICAL);

    // Create compression radio box
    wxArrayString compressionChoices;
    compressionChoices.Add("No compression");
    compressionChoices.Add("Individual compression");
    compressionChoices.Add("Global compression");
    m_compressionBox = new wxRadioBox(headerPanel, wxID_ANY, "", 
                                    wxDefaultPosition, wxDefaultSize,
                                    compressionChoices, 1, wxRA_SPECIFY_COLS);
    m_compressionBox->SetFont(monoFont);
    m_compressionBox->Bind(wxEVT_RADIOBOX, &MyFrame::OnPackModeChanged, this);

    // Create horizontal sizer for fields and compression box
    wxBoxSizer* headerContentSizer = new wxBoxSizer(wxHORIZONTAL);
    headerContentSizer->Add(fieldsSizer, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    headerContentSizer->Add(m_compressionBox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT | wxBOTTOM, 5);

    // Add the header content sizer to the header sizer
    AdditionalheaderSizer->Add(headerContentSizer, 0, wxEXPAND | wxBOTTOM, 5);

    headerSizer->Add(AdditionalheaderSizer, 0, wxEXPAND | wxALL, 5);
    
    headerPanel->SetSizer(headerSizer);
    verticalSizer->Add(headerPanel, 0, wxEXPAND | wxALL, 5);

    // Create the main horizontal sizer for tree and right panel
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    wxMenuBar* menuBar = new wxMenuBar();
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(wxID_NEW, "&New");
    Bind(wxEVT_MENU, &MyFrame::OnNew, this, wxID_NEW);
    fileMenu->Append(ID_OPEN_FILE, "&Load\tCtrl+L");
    Bind(wxEVT_MENU, &MyFrame::OnLoad, this, ID_OPEN_FILE);
    fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S");
    Bind(wxEVT_MENU, &MyFrame::OnSave, this, wxID_SAVE);
    fileMenu->Append(ID_SAVE_AS, "Save &As...\tCtrl+Shift+S");
    Bind(wxEVT_MENU, &MyFrame::OnSaveAs, this, ID_SAVE_AS);
    fileMenu->Append(ID_SAVE_STRIPPED, "Save S&tripped");
    Bind(wxEVT_MENU, &MyFrame::OnSaveStripped, this, ID_SAVE_STRIPPED);
    fileMenu->Append(ID_MERGE, "&Merge");
    Bind(wxEVT_MENU, &MyFrame::OnMerge, this, ID_MERGE);
    fileMenu->Append(ID_UPDATE, "&Update\tCtrl+U");
    Bind(wxEVT_MENU, &MyFrame::OnUpdate, this, ID_UPDATE);
    fileMenu->Append(ID_UPDATE_SELECTION, "Update S&election");
    Bind(wxEVT_MENU, &MyFrame::OnUpdateSelection, this, ID_UPDATE_SELECTION);
    fileMenu->Append(ID_FORCE_UPDATE, "&Force Update\tCtrl+F");
    Bind(wxEVT_MENU, &MyFrame::OnForceUpdate, this, ID_FORCE_UPDATE);
    fileMenu->Append(ID_FORCE_UPDATE_SELECTION, "F&orce Update Selection");
    Bind(wxEVT_MENU, &MyFrame::OnForceUpdateSelection, this, ID_FORCE_UPDATE_SELECTION);
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_READ_BITMAP, "&Read Bitmap\tCtrl+R");
    Bind(wxEVT_MENU, &MyFrame::OnReadBitmap, this, ID_READ_BITMAP);
    fileMenu->Append(ID_VIEW_BITMAP, "&View Bitmap\tCtrl+V");
    Bind(wxEVT_MENU, &MyFrame::OnViewBitmap, this, ID_VIEW_BITMAP);
    fileMenu->Append(ID_GRAB_FROM_GRID, "&Grab from Grid");
    Bind(wxEVT_MENU, &MyFrame::OnGrabFromGrid, this, ID_GRAB_FROM_GRID);
    fileMenu->Append(ID_READ_ALPHA, "Read &Alpha channel");
    Bind(wxEVT_MENU, &MyFrame::OnReadAlpha, this, ID_READ_ALPHA);
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "&Quit\tCtrl+Q");
    Bind(wxEVT_MENU, &MyFrame::OnQuit, this, wxID_EXIT);
    menuBar->Append(fileMenu, "File");
    
    wxMenu* objectMenu = new wxMenu();
    objectMenu->Append(ID_GRAB, "&Grab\tCtrl+G");
    Bind(wxEVT_MENU, &MyFrame::OnGrab, this, ID_GRAB);
    objectMenu->Append(ID_EXPORT, "&Export\tCtrl+E");
    Bind(wxEVT_MENU, &MyFrame::OnExport, this, ID_EXPORT);
    objectMenu->Append(ID_DELETE, "&Delete\tCtrl+D");
    Bind(wxEVT_MENU, &MyFrame::OnDelete, this, ID_DELETE);

    wxMenu* moveMenu = new wxMenu();
    moveMenu->Append(ID_MOVE_UP, "&Up\tShift+U");
    Bind(wxEVT_MENU, &MyFrame::OnMoveUp, this, ID_MOVE_UP);
    moveMenu->Append(ID_MOVE_DOWN, "&Down\tShift+D");
    Bind(wxEVT_MENU, &MyFrame::OnMoveDown, this, ID_MOVE_DOWN);
    objectMenu->AppendSubMenu(moveMenu, "&Move");
    
    wxMenu* replaceMenu = new wxMenu();
    replaceMenu->Append(ID_REPLACE_BITMAP, "&Bitmap");
    Bind(wxEVT_MENU, &MyFrame::OnReplaceWithBitmap, this, ID_REPLACE_BITMAP);
    replaceMenu->Append(ID_REPLACE_COMPILED_SPRITE, "&Compiled sprite");
    Bind(wxEVT_MENU, &MyFrame::OnReplaceWithCompiledSprite, this, ID_REPLACE_COMPILED_SPRITE);
    replaceMenu->Append(ID_REPLACE_X_SPRITE, "Compiled &X-sprite");
    Bind(wxEVT_MENU, &MyFrame::OnReplaceWithXSprite, this, ID_REPLACE_X_SPRITE);
    replaceMenu->Append(ID_REPLACE_DATAFILE, "&Datafile");
    Bind(wxEVT_MENU, &MyFrame::OnReplaceWithDatafile, this, ID_REPLACE_DATAFILE);
    replaceMenu->Append(ID_REPLACE_FLI, "&FLI/FLC animation");
    Bind(wxEVT_MENU, &MyFrame::OnReplaceWithFLI, this, ID_REPLACE_FLI);
    replaceMenu->Append(ID_REPLACE_FONT, "F&ont");
    Bind(wxEVT_MENU, &MyFrame::OnReplaceWithFont, this, ID_REPLACE_FONT);
    replaceMenu->Append(ID_REPLACE_MIDI, "&MIDI file");
    Bind(wxEVT_MENU, &MyFrame::OnReplaceWithMIDI, this, ID_REPLACE_MIDI);
    replaceMenu->Append(ID_REPLACE_PALETTE, "&Palette");
    Bind(wxEVT_MENU, &MyFrame::OnReplaceWithPalette, this, ID_REPLACE_PALETTE);
    replaceMenu->Append(ID_REPLACE_RLE, "&RLE sprite");
    Bind(wxEVT_MENU, &MyFrame::OnReplaceWithRLE, this, ID_REPLACE_RLE);
    replaceMenu->Append(ID_REPLACE_SAMPLE, "&Sample");
    Bind(wxEVT_MENU, &MyFrame::OnReplaceWithSample, this, ID_REPLACE_SAMPLE);
    replaceMenu->Append(ID_REPLACE_OTHER, "O&ther");
    Bind(wxEVT_MENU, &MyFrame::OnReplaceWithOther, this, ID_REPLACE_OTHER);
    objectMenu->AppendSubMenu(replaceMenu, "&Replace");

    objectMenu->Append(ID_RENAME, "&Rename\tCtrl+N");
    Bind(wxEVT_MENU, &MyFrame::OnRenameObject, this, ID_RENAME);
    objectMenu->Append(ID_SET_PROPERTY, "Set &Property\tCtrl+P");
    Bind(wxEVT_MENU, &MyFrame::OnSetProperty, this, ID_SET_PROPERTY);
    objectMenu->Append(ID_AUTOCROP, "&Autocrop");
    Bind(wxEVT_MENU, &MyFrame::OnAutocrop, this, ID_AUTOCROP);
    objectMenu->Append(ID_BOX_GRAB, "&Box Grab\tCtrl+B");
    Bind(wxEVT_MENU, &MyFrame::OnBoxGrab, this, ID_BOX_GRAB);
    objectMenu->Append(ID_UNGRAB, "&Ungrab");
    Bind(wxEVT_MENU, &MyFrame::OnUngrab, this, ID_UNGRAB);

    wxMenu* alphachannelMenu = new wxMenu();
    alphachannelMenu->Append(ID_VIEW_ALPHA, "&View Alpha");
    Bind(wxEVT_MENU, &MyFrame::OnViewAlpha, this, ID_VIEW_ALPHA);
    alphachannelMenu->Append(ID_IMPORT_ALPHA, "&Import Alpha");
    Bind(wxEVT_MENU, &MyFrame::OnImportAlpha, this, ID_IMPORT_ALPHA);
    alphachannelMenu->Append(ID_EXPORT_ALPHA, "&Export Alpha");
    Bind(wxEVT_MENU, &MyFrame::OnExportAlpha, this, ID_EXPORT_ALPHA);
    alphachannelMenu->Append(ID_DELETE_ALPHA, "&Delete Alpha");
    Bind(wxEVT_MENU, &MyFrame::OnDeleteAlpha, this, ID_DELETE_ALPHA);
    objectMenu->AppendSubMenu(alphachannelMenu, "Alpha &Channel");

    wxMenu* depthMenu = new wxMenu();
    depthMenu->Append(ID_DEPTH_256, "To &256 color palette");
    Bind(wxEVT_MENU, &MyFrame::OnDepth256, this, ID_DEPTH_256);
    depthMenu->Append(ID_DEPTH_15, "To 1&5 hicolor");
    Bind(wxEVT_MENU, &MyFrame::OnDepth15, this, ID_DEPTH_15);
    depthMenu->Append(ID_DEPTH_16, "To 1&6 hicolor");
    Bind(wxEVT_MENU, &MyFrame::OnDepth16, this, ID_DEPTH_16);
    depthMenu->Append(ID_DEPTH_24, "To 2&4 truecolor");
    Bind(wxEVT_MENU, &MyFrame::OnDepth24, this, ID_DEPTH_24);
    depthMenu->Append(ID_DEPTH_32, "To &32 truecolor");
    Bind(wxEVT_MENU, &MyFrame::OnDepth32, this, ID_DEPTH_32);
    objectMenu->AppendSubMenu(depthMenu, "C&hange Depth");

    wxMenu* filenameMenu = new wxMenu();
    filenameMenu->Append(ID_FILENAME_RELATIVE, "To &Relative");
    Bind(wxEVT_MENU, &MyFrame::OnChangeFilenameToRelative, this, ID_FILENAME_RELATIVE);
    filenameMenu->Append(ID_FILENAME_ABSOLUTE, "To &Absolute");
    Bind(wxEVT_MENU, &MyFrame::OnChangeFilenameToAbsolute, this, ID_FILENAME_ABSOLUTE);
    objectMenu->AppendSubMenu(filenameMenu, "Change &Filename");
    
    wxMenu* typeMenu = new wxMenu();
    typeMenu->Append(ID_TYPE_BITMAP, "To &Bitmap");
    Bind(wxEVT_MENU, &MyFrame::OnChangeTypeToBitmap, this, ID_TYPE_BITMAP);
    typeMenu->Append(ID_TYPE_RLE, "To &RLE Sprite");
    Bind(wxEVT_MENU, &MyFrame::OnChangeTypeToRLE, this, ID_TYPE_RLE);
    typeMenu->Append(ID_TYPE_COMPILED, "To &Compiled Sprite");
    Bind(wxEVT_MENU, &MyFrame::OnChangeTypeToCompiled, this, ID_TYPE_COMPILED);
    typeMenu->Append(ID_TYPE_X_COMPILED, "To &X-Compiled Sprite");
    Bind(wxEVT_MENU, &MyFrame::OnChangeTypeToXCompiled, this, ID_TYPE_X_COMPILED);
    objectMenu->AppendSubMenu(typeMenu, "Change &Type");

    objectMenu->Append(ID_SHELL_EDIT, "&Shell Edit\tCtrl+Z");
    Bind(wxEVT_MENU, &MyFrame::OnShellEdit, this, ID_SHELL_EDIT);
    objectMenu->AppendSeparator();
    // Add New submenu
    wxMenu* newMenu = new wxMenu();
    newMenu->Append(ID_NEW_BITMAP, "&Bitmap");
    Bind(wxEVT_MENU, &MyFrame::OnNewBitmap, this, ID_NEW_BITMAP);
    newMenu->Append(ID_NEW_RLE, "RLE sprite");
    Bind(wxEVT_MENU, &MyFrame::OnNewRLE, this, ID_NEW_RLE);
    newMenu->Append(ID_NEW_COMPILED_SPRITE, "Compiled sprite");
    Bind(wxEVT_MENU, &MyFrame::OnNewCompiledSprite, this, ID_NEW_COMPILED_SPRITE);
    newMenu->Append(ID_NEW_X_SPRITE, "Compiled X-sprite");
    Bind(wxEVT_MENU, &MyFrame::OnNewXSprite, this, ID_NEW_X_SPRITE);
    newMenu->Append(ID_NEW_DATAFILE, "Datafile");
    Bind(wxEVT_MENU, &MyFrame::OnNewDatafile, this, ID_NEW_DATAFILE);
    newMenu->Append(ID_NEW_FLI, "FLI/FLC animation");
    Bind(wxEVT_MENU, &MyFrame::OnNewFLI, this, ID_NEW_FLI);
    newMenu->Append(ID_NEW_FONT, "Font");
    Bind(wxEVT_MENU, &MyFrame::OnNewFont, this, ID_NEW_FONT);
    newMenu->Append(ID_NEW_MIDI, "MIDI file");
    Bind(wxEVT_MENU, &MyFrame::OnNewMIDI, this, ID_NEW_MIDI);
    newMenu->Append(ID_NEW_PALETTE, "Palette");
    Bind(wxEVT_MENU, &MyFrame::OnNewPalette, this, ID_NEW_PALETTE);
    newMenu->Append(ID_NEW_SAMPLE, "Sample");
    Bind(wxEVT_MENU, &MyFrame::OnNewSample, this, ID_NEW_SAMPLE);
    newMenu->Append(ID_NEW_OGG_AUDIO, "Ogg audio");
    Bind(wxEVT_MENU, &MyFrame::OnNewOggAudio, this, ID_NEW_OGG_AUDIO);
    newMenu->Append(ID_NEW_OTHER, "Other");
    Bind(wxEVT_MENU, &MyFrame::OnNewOther, this, ID_NEW_OTHER);
    objectMenu->AppendSubMenu(newMenu, "&New");
    menuBar->Append(objectMenu, "&Object");
    
    wxMenu* optionsMenu = new wxMenu();
    optionsMenu->AppendCheckItem(ID_BACKUP_DATAFILES, "&Backup Datafiles");
    optionsMenu->Check(ID_BACKUP_DATAFILES, true);
    optionsMenu->AppendCheckItem(ID_INDEX_OBJECTS, "&Index Objects");
    Bind(wxEVT_MENU, &MyFrame::OnIndexObjects, this, ID_INDEX_OBJECTS);
    optionsMenu->AppendCheckItem(ID_SORT_OBJECTS, "&Sort Objects");
    Bind(wxEVT_MENU, &MyFrame::OnSortObjects, this, ID_SORT_OBJECTS);
    optionsMenu->AppendCheckItem(ID_STORE_RELATIVE, "Store &Relative Filenames");
    Bind(wxEVT_MENU, &MyFrame::OnStoreRelativeFilenames, this, ID_STORE_RELATIVE);
    optionsMenu->AppendCheckItem(ID_DITHER_IMAGES, "&Dither Images");
    Bind(wxEVT_MENU, &MyFrame::OnDitherImages, this, ID_DITHER_IMAGES);
    optionsMenu->AppendCheckItem(ID_PRESERVE_TRANSPARENCY, "Preserve &Transparency");
    Bind(wxEVT_MENU, &MyFrame::OnPreserveTransparency, this, ID_PRESERVE_TRANSPARENCY);
    menuBar->Append(optionsMenu, "O&ptions");
    
    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(wxID_HELP, "&Help\tF1");
    Bind(wxEVT_MENU, &MyFrame::OnHelp, this, wxID_HELP);
    helpMenu->Append(ID_HELP_SYSTEM, "&System");
    Bind(wxEVT_MENU, &MyFrame::OnHelpSystem, this, ID_HELP_SYSTEM);
    helpMenu->Append(ID_HELP_WORMS, "&Worms");
    Bind(wxEVT_MENU, &MyFrame::OnNotImplemented, this, ID_HELP_WORMS);
    helpMenu->Append(wxID_ABOUT, "&About");
    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
    menuBar->Append(helpMenu, "&Help");
    
    SetMenuBar(menuBar);
    
    // Create tree control with specific ID
    m_tree = new wxTreeCtrl(this, ID_TREE_CTRL, wxDefaultPosition, wxDefaultSize,
                           wxTR_DEFAULT_STYLE | wxTR_MULTIPLE);
    
    // Bind right-click context menu event
    m_tree->Bind(wxEVT_TREE_ITEM_MENU, &MyFrame::OnTreeItemMenu, this);
    
    // Create root item only
    wxTreeItemId rootId = m_tree->AddRoot("<root>");
    m_tree->SelectItem(rootId);
    
    // No sample data - tree will be populated when a file is loaded
    
    // Bind tree selection event with specific ID
    m_tree->Bind(wxEVT_TREE_SEL_CHANGED, &MyFrame::OnTreeSelectionChanged, this);
    
    // Add handler for tree item activation (double-click)
    m_tree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &MyFrame::OnTreeItemActivated, this);
    
    // Add drag and drop support
    m_tree->Bind(wxEVT_TREE_BEGIN_DRAG, &MyFrame::OnTreeBeginDrag, this);
    m_tree->Bind(wxEVT_TREE_END_DRAG, &MyFrame::OnTreeEndDrag, this);
    
    wxBoxSizer* treeSizer = new wxBoxSizer(wxVERTICAL);
    treeSizer->Add(m_tree, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(treeSizer, 1, wxEXPAND | wxALL);
    
    wxPanel* rightPanel = new wxPanel(this);
    wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create list control with specific ID
    m_details = new wxListCtrl(rightPanel, ID_LIST_CTRL, wxDefaultPosition, wxDefaultSize,
                             wxLC_REPORT | wxLC_SINGLE_SEL);
    m_details->InsertColumn(0, "Property");
    m_details->InsertColumn(1, "Value");
    m_details->SetColumnWidth(0, 100);
    m_details->SetColumnWidth(1, 300);
    rightSizer->Add(m_details, 0, wxEXPAND | wxALL);
    m_details->Hide();  // Initially hide property list
    
    // Bind list control event with specific ID
    m_details->Bind(wxEVT_LIST_ITEM_ACTIVATED, &MyFrame::OnPropertyActivated, this);
    
    // Add info text control
    m_infoText = new wxStaticText(rightPanel, wxID_ANY, "");
    wxFont infoMonoFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Consolas");
    m_infoText->SetFont(infoMonoFont);
    rightSizer->Add(m_infoText, 0, wxEXPAND | wxALL, 5);
    m_infoText->Hide();  // Initially hide info text
    
    // Create preview panel
    wxPanel* previewPanel = new wxPanel(rightPanel, wxID_ANY);
    wxBoxSizer* previewSizer = new wxBoxSizer(wxVERTICAL);

    // Create image preview panel
    m_imagePreviewPanel = new wxScrolledWindow(previewPanel, wxID_ANY);
    m_imagePreviewPanel->SetScrollRate(10, 10); // Enable scrolling with 10 pixel steps
    m_imagePreview = new wxStaticBitmap(m_imagePreviewPanel, wxID_ANY, wxNullBitmap);
    wxBoxSizer* imageSizer = new wxBoxSizer(wxVERTICAL);
    imageSizer->Add(m_imagePreview, 1, wxALL, 5);
    m_imagePreviewPanel->SetSizer(imageSizer);

    // Create the palette info text (initially hidden)
    m_paletteInfoText = new wxStaticText(previewPanel, wxID_ANY, 
        "A different palette is currently in use.\nTo select this one, double-click on it in the item list.", 
        wxDefaultPosition, wxDefaultSize, wxALIGN_LEFT); //wxST_NO_AUTORESIZE
    m_paletteInfoText->SetFont(infoMonoFont);
    m_paletteInfoText->Wrap(200); // Set initial wrap width
    m_paletteInfoText->Hide();

    // Create a horizontal sizer for the image preview and palette text
    wxBoxSizer* imageAreaSizer = new wxBoxSizer(wxHORIZONTAL);
    imageAreaSizer->Add(m_imagePreviewPanel, 1, wxEXPAND | wxALL, 5);
    imageAreaSizer->Add(m_paletteInfoText, 0, wxALL | wxALIGN_CENTER_VERTICAL | wxALIGN_LEFT, 5);

    // Add the image area sizer to the preview sizer
    previewSizer->Add(imageAreaSizer, 1, wxEXPAND);

    // Create zoom controls
    wxBoxSizer* zoomSizer = new wxBoxSizer(wxHORIZONTAL);
    m_zoomLabel = new wxStaticText(previewPanel, wxID_ANY, "Zoom: 1.0x");
    m_zoomSlider = new wxSlider(previewPanel, wxID_ANY, 10, 5, 30);
    zoomSizer->Add(m_zoomLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    zoomSizer->Add(m_zoomSlider, 1, wxEXPAND);
    
    // Add zoom sizer to preview sizer
    previewSizer->Add(zoomSizer, 0, wxEXPAND | wxALL, 5);

    // Create video preview and add it above the audio panel
    m_videoPanelContainer = new wxScrolledWindow(previewPanel, wxID_ANY);
    m_videoPanelContainer->SetScrollRate(10, 10);
    m_videoPanel = new VideoDataPanel(m_videoPanelContainer, [this](int a, int b) { this->AVPositionUpdateCallback(a, b); });
    m_videoPanel->SetMinSize(wxSize(320, 240));
    wxBoxSizer* videoSizer = new wxBoxSizer(wxVERTICAL);
    videoSizer->Add(m_videoPanel, 1, wxEXPAND | wxALL, 5);
    m_videoPanelContainer->SetSizer(videoSizer);
    previewSizer->Add(m_videoPanelContainer, 1, wxEXPAND | wxALL, 5);
    m_videoPanelContainer->Hide();

    // Create audio controls panel
    m_audioPanel = new wxPanel(previewPanel, wxID_ANY);
    wxBoxSizer* audioSizer = new wxBoxSizer(wxHORIZONTAL);
    m_audioControl = std::make_unique<AudioPlaybackControl>([this](int a, int b) { this->AVPositionUpdateCallback(a, b); });
    
    // Play button - fixed size, centered vertically
    m_playButton = new wxBitmapButton(m_audioPanel, ID_PLAY_AUDIOVIDEO, GetPlayIcon(), 
        wxDefaultPosition, wxSize(40, 40));
    audioSizer->Add(m_playButton, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    // Slider - expands horizontally
    m_AVSlider = new wxSlider(m_audioPanel, ID_AUDIOVIDEO_POSITION, 0, 0, 100, 
        wxDefaultPosition, wxSize(300, -1));
    audioSizer->Add(m_AVSlider, 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    // Time label - fixed size, centered vertically
    m_timeLabel = new wxStaticText(m_audioPanel, wxID_ANY, "00:00.0 / 00:00.0");
    audioSizer->Add(m_timeLabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
    
    m_audioPanel->SetSizer(audioSizer);
    
    // Add a stretch spacer to push audio panel to the bottom
    previewSizer->Add(m_audioPanel, 0, wxEXPAND | wxALL, 5);

    // Set the preview panel's sizer
    previewPanel->SetSizer(previewSizer);
    
    // Add preview panel to rightSizer
    rightSizer->Add(previewPanel, 1, wxEXPAND | wxALL, 5);
    
    // Initially hide audio controls and zoom controls
    m_audioPanel->Hide();
    m_zoomSlider->Hide();
    m_zoomLabel->Hide();

    // Bind event for zoom change
    m_zoomSlider->Bind(wxEVT_SLIDER, &MyFrame::OnZoomChange, this);

    // Bind events for grid value changes
    m_xGridText->Bind(wxEVT_TEXT_ENTER, &MyFrame::OnGridChange, this);
    m_yGridText->Bind(wxEVT_TEXT_ENTER, &MyFrame::OnGridChange, this);

    // Bind events for header panel text controls
    m_headerText->Bind(wxEVT_TEXT, &MyFrame::OnHeaderChange, this);
    m_prefixText->Bind(wxEVT_TEXT, &MyFrame::OnHeaderChange, this);
    m_passwordText->Bind(wxEVT_TEXT, &MyFrame::OnPasswordChange, this);
    
    rightPanel->SetSizer(rightSizer);
    mainSizer->Add(rightPanel, 2, wxEXPAND | wxALL, 5);
    
    verticalSizer->Add(mainSizer, 1, wxEXPAND);
    SetSizer(verticalSizer);

    CreateStatusBar();
    SetStatusText(GrabberName + " status");

    m_currentObject = nullptr;

    // Load settings from allegro.cfg
    m_grabberInfo.LoadSettings("allegro.cfg");
    UpdateUIFromGrabberInfo();

    // Bind audio/video events
    Bind(wxEVT_BUTTON, &MyFrame::OnPlayPauseAV, this, ID_PLAY_AUDIOVIDEO);
    Bind(wxEVT_SCROLL_THUMBTRACK, &MyFrame::OnSliderDrag, this, ID_AUDIOVIDEO_POSITION);
    Bind(wxEVT_SCROLL_THUMBRELEASE, &MyFrame::OnSliderRelease, this, ID_AUDIOVIDEO_POSITION);

    // Bind key events
    Bind(wxEVT_KEY_DOWN, &MyFrame::OnKeyDown, this);
    m_tree->Bind(wxEVT_KEY_DOWN, &MyFrame::OnKeyDown, this);

    // In class member variables (in main.h):
    // wxButton* m_toggleHeaderButton;
    // bool m_showAdditionalHeader = true;
    // ... existing code ...

    // In the header panel setup:
    // ...
    // Create arrow bitmaps for toggle button
    auto makeArrowBitmap = [](bool down) -> wxBitmap {
        wxBitmap bmp(16, 16);
        wxMemoryDC dc(bmp);
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
        wxPoint pts[3];
        if (down) {
            pts[0] = wxPoint(4, 6);
            pts[1] = wxPoint(12, 6);
            pts[2] = wxPoint(8, 11);
        } else {
            pts[0] = wxPoint(4, 11);
            pts[1] = wxPoint(12, 11);
            pts[2] = wxPoint(8, 6);
        }
        dc.SetBrush(*wxBLACK_BRUSH);
        dc.SetPen(*wxBLACK_PEN);
        dc.DrawPolygon(3, pts);
        dc.SelectObject(wxNullBitmap);
        return bmp;
    };
    m_arrowDownBmp = makeArrowBitmap(true);
    m_arrowUpBmp = makeArrowBitmap(false);

    m_showAdditionalHeader = false;
    m_toggleHeaderButton = new wxButton(headerPanel, wxID_ANY, "", wxDefaultPosition, wxSize(24, 24));
    m_toggleHeaderButton->SetBitmap(m_arrowDownBmp);
    editingSizer->Add(m_toggleHeaderButton, 0, wxLEFT, 5);
    m_toggleHeaderButton->Bind(wxEVT_BUTTON, [this, AdditionalheaderSizer](wxCommandEvent&) {
        m_showAdditionalHeader = !m_showAdditionalHeader;
        AdditionalheaderSizer->Show(m_showAdditionalHeader);
        m_toggleHeaderButton->SetBitmap(m_showAdditionalHeader ? m_arrowUpBmp : m_arrowDownBmp);
        this->Layout();
    });
    AdditionalheaderSizer->Show(m_showAdditionalHeader);
    // ... existing code ...
}

void MyFrame::UpdateUIFromGrabberInfo() {
    // Update grid values
    m_xGridText->SetValue(wxString::Format("%d", m_grabberInfo.GetXGrid()));
    m_yGridText->SetValue(wxString::Format("%d", m_grabberInfo.GetYGrid()));
    
    // Update password field
    m_passwordText->SetValue(wxString::FromUTF8(m_grabberInfo.GetPassword()));
    
    // Update menu options
    wxMenuBar* menuBar = GetMenuBar();
    if (menuBar) {
        wxMenu* optionsMenu = menuBar->GetMenu(menuBar->FindMenu("&Options"));
        if (optionsMenu) {
            optionsMenu->Check(ID_BACKUP_DATAFILES, m_grabberInfo.GetBackup());
            optionsMenu->Check(ID_DITHER_IMAGES, m_grabberInfo.GetDither());
            optionsMenu->Check(ID_SORT_OBJECTS, m_grabberInfo.GetSort());
            optionsMenu->Check(ID_STORE_RELATIVE, m_grabberInfo.GetRelativeFilenames());
            optionsMenu->Check(ID_PRESERVE_TRANSPARENCY, m_grabberInfo.GetTransparency());
        }
    }

    // Update compression mode
    if (m_compressionBox) {
        switch (m_grabberInfo.GetPack()) {
            case GrabberInfo::CompressionMode::None:
                m_compressionBox->SetSelection(0);
                break;
            case GrabberInfo::CompressionMode::Individual:
                m_compressionBox->SetSelection(1);
                break;
            case GrabberInfo::CompressionMode::Global:
                m_compressionBox->SetSelection(2);
                break;
        }
    }
}
 
void MyFrame::OnQuit(wxCommandEvent& event)
{
    Close(true);
}
 
void MyFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("Allegro Datafile Editor, version " + GrabberVersion + ", " + wxVERSION_STRING + "\n\nMIT license (c) 2025 Synoecium",
                 "About Allegro Datafile Editor", wxOK | wxICON_INFORMATION);
}
 
void MyFrame::UpdatePropertyList(std::shared_ptr<DataParser::DataObject> obj)
{
    // First hide the controls
    m_details->Hide();
    m_infoText->Hide();
    
    if (!obj) {
        return;
    }
    
    // Clear the list control first
    m_details->DeleteAllItems();
    
    // Show the controls
    m_details->Show();
    m_infoText->Show();
    
    // Add properties to list in order
    long idx = 0;
    for (const auto& [propId, value] : obj->getOrderedProperties()) {
        std::string propName = DataParser::ConvertIDToString(propId);
        idx = m_details->InsertItem(idx, propName);
        if (idx != -1) {  // Check if item was inserted successfully
            m_details->SetItem(idx, 1, value);
        }
    }

    // Only resize if we have items
    if (m_details->GetItemCount() > 0) {
        // Resize the list control to fit its content
        wxRect itemRect;
        if (m_details->GetItemRect(0, itemRect)) {  // Check if we can get the rect
            int itemHeight = itemRect.GetHeight();
            int headerHeight = itemRect.GetTop();
            int totalHeight = headerHeight + (itemHeight * m_details->GetItemCount()) + 10;
            m_details->SetMinSize(wxSize(-1, totalHeight));
            m_details->SetSize(-1, totalHeight);
            Layout();
        }
    }
}

void MyFrame::OnPropertyActivated(wxListEvent& event)
{
    if (!m_currentObject) return;
    
    long item = event.GetIndex();
    wxString propName = m_details->GetItemText(item, 0);
    wxString propValue = m_details->GetItemText(item, 1);
    
    PropertyEditDialog dialog(this, "Edit Property", propName, propValue);
    if (dialog.ShowModal() == wxID_OK) {
        wxString newValue = dialog.GetValue();
        logInfo("Property " + propName + " changed from " + propValue + " to " + newValue);
        if (newValue != propValue) {  // Only mark as modified if value actually changed
            m_details->SetItem(item, 1, newValue);
            
            // Update the property in the object using string-based setProperty
            m_currentObject->setProperty(propName.ToStdString(), newValue.ToStdString());
            
            // Update the property list to reflect any changes
            UpdatePropertyList(m_currentObject);

            // If this was the NAME property, refresh the tree display
            if (propName == "NAME") {
                RefreshTreeDisplay();
            }

            // Mark as modified
            SetModified(true);
        }
    }
}

void MyFrame::UpdateImageDisplay(const wxImage& image, double zoomLevel, bool addBorder) {
    if (!image.IsOk()) {
        return;
    }

    try {
        // Scale image according to zoom level
        wxImage scaledImage = image.Scale(image.GetWidth() * zoomLevel, image.GetHeight() * zoomLevel, wxIMAGE_QUALITY_NEAREST);
        if (!scaledImage.IsOk()) {
            return;
        }

        wxImage finalImage;
        if (addBorder) {
            // Add a 1-pixel black border to the scaled image
            finalImage = wxImage(scaledImage.GetWidth() + 2, scaledImage.GetHeight() + 2);
            finalImage.Clear(0);  // Fill with black
            finalImage.Paste(scaledImage, 1, 1);  // Place original image inside border
        } else {
            finalImage = scaledImage;
        }

        m_imagePreview->SetBitmap(finalImage);
        
        // Set virtual size to match the image size
        m_imagePreviewPanel->SetVirtualSize(finalImage.GetWidth(), finalImage.GetHeight());
        
        // Ensure scrollbars update
        m_imagePreviewPanel->FitInside();
        m_imagePreviewPanel->Refresh();
        
        Layout();
    } catch (const std::exception&) {
        return;
    }
}



void MyFrame::OnTreeSelectionChanged(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk() || item == m_tree->GetRootItem()) {
        m_currentObject = nullptr;
        UpdateObjectPreview();
        return;
    }

    // Get the object pointer from tree item data
    ObjectTreeData* data = static_cast<ObjectTreeData*>(m_tree->GetItemData(item));
    if (!data || !data->object) {
        logDebug("Tree selection changed to invalid item");
        UpdatePreviewControls(nullptr);
        m_currentObject = nullptr;
        return;
    }

    m_currentObject = data->object;
    UpdateObjectPreview();
}

void MyFrame::UpdatePreviewControls(std::shared_ptr<DataParser::DataObject> obj) {
    // Hide all by default
    m_infoText->Hide();
    m_audioPanel->Hide();
    m_imagePreviewPanel->Hide();
    m_zoomLabel->Hide();
    m_zoomSlider->Hide();
    m_paletteInfoText->Hide();
    m_videoPanelContainer->Hide();
    ResetAVPlayback();

    if (!obj) {
        // No object selected, all already hidden
        Layout(); // Refresh the layout
        return;
    }

    if (obj->isAudio() || obj->isVideo()) {
        // Audio or Video object selected - show audio controls, hide image controls
        logDebug(wxString(obj->isAudio() ? "audio" : "video") + " object selected");
        m_audioPanel->Show();
        int duration = obj->isAudio() ? obj->getAudio().getDurationMs() : obj->getVideo().getDurationMs();
        m_playbackDuration = duration;
        m_timeLabel->SetLabel(wxString::Format("00:00.0 / %s", FormatTime(duration)));
        wxString caption = obj->isAudio() ? obj->getAudio().getPreviewCaption() : obj->getVideo().getPreviewCaption();
        m_infoText->SetLabel(caption);
        m_infoText->Show();

        // Disable Play button for MIDI - make it grayed out and non-functional
        if (obj->typeID == ObjectType::DAT_MIDI) {
            m_playButton->Disable();
            m_playButton->SetToolTip("MIDI files cannot be played as audio");
            m_playButton->SetCanFocus(false);
        } else {
            m_playButton->Enable();
            m_playButton->SetToolTip("Play/Pause");
            m_playButton->SetCanFocus(true);
        }

        if (obj->isVideo()) {
            m_videoPanelContainer->Show();
            m_videoPanel->SetVideoData(obj);
        } else if (obj->isAudio()) {
            m_audioControl->SetAudioData(obj);
        }
    }
    else if (obj->isBitmap()) {
        // Bitmap object selected - show image controls, hide audio controls
        m_imagePreviewPanel->Show();
        m_zoomLabel->Show();
        m_zoomSlider->Show();
        m_infoText->Show();

        // Update bitmap display
        wxImage image;
        if (obj->getBitmap().toWxImage(image, m_currentPalette)) {
            UpdateImageDisplay(image, m_zoomSlider->GetValue() / 10.0, true);  // Add border for bitmaps
        }
        
        // Update info text with image dimensions
        const BitmapData& bmpData = obj->getBitmap();
        m_infoText->SetLabel(bmpData.getPreviewCaption());

        if (obj->getBitmap().isPalette() && obj->getBitmap().data != m_currentPalette) {
            m_paletteInfoText->Show();
        }
    }
    else if (obj->isFont()) {
        // Font object selected - show info text, preview image, and zoom controls
        wxString fontCaption = obj->getFont().getPreviewCaption();
        fontCaption += "\n\nDouble-click to edit this font";
        m_infoText->SetLabel(fontCaption);
        m_infoText->Show();
        m_imagePreviewPanel->Show();
        m_zoomLabel->Show();
        m_zoomSlider->Show();
        
        // Generate and show font preview image with current zoom
        double zoomLevel = m_zoomSlider->GetValue() / 10.0;
        wxImage fontImg = obj->getFont().getPreviewImage();
        if (fontImg.IsOk()) {
            logDebug("FontData::getPreviewImage: showing preview image");
            UpdateImageDisplay(fontImg, zoomLevel, false);  // No border for fonts
        } else {
            // Clear the preview display for empty fonts
            logDebug("FontData::getPreviewImage: clearing preview for empty font");
            wxImage emptyImg(1, 1);  // Create minimal empty image
            emptyImg.SetRGB(0, 0, 255, 255, 255);  // White pixel
            UpdateImageDisplay(emptyImg, 1.0, false);  // Display empty image to clear previous preview
        }
    }
    else if (obj->isNested()) {
        // Datafile object selected - show info text only
        m_infoText->SetLabel("Datafile\n\nDouble-click in the item list to (un)fold it");
        m_infoText->Show();
    }
    else if (obj->isRawData()) {
        // Raw/binary data object - show info and hex dump preview
        const std::vector<uint8_t>& data = obj->getRawData();
        
        wxString info = wxString::Format("binary data (%d bytes)\n\n", (int)data.size());
        
        // Show first 256 bytes as hex dump with ASCII representation
        size_t previewSize = std::min(data.size(), size_t(256));
        if (previewSize > 0) {
            info += "Data preview (first " + wxString::Format("%d", (int)previewSize) + " bytes):\n\n";
            
            // Format as hex dump: offset | hex bytes | ASCII
            for (size_t i = 0; i < previewSize; i += 16) {
                // Offset
                info += wxString::Format("%08X  ", (unsigned int)i);
                
                // Hex bytes (16 per line)
                wxString hexPart, asciiPart;
                for (size_t j = 0; j < 16; ++j) {
                    if (i + j < previewSize) {
                        uint8_t byte = data[i + j];
                        hexPart += wxString::Format("%02X ", byte);
                        
                        // ASCII representation (printable chars only)
                        if (byte >= 32 && byte <= 126) {
                            asciiPart += wxChar(byte);
                        } else {
                            asciiPart += '.';
                        }
                    } else {
                        hexPart += "   ";  // 3 spaces for missing bytes
                        asciiPart += ' ';
                    }
                    
                    // Add extra space after 8 bytes for readability
                    if (j == 7) {
                        hexPart += " ";
                    }
                }
                
                info += hexPart + " |" + asciiPart + "|\n";
            }
            
            if (data.size() > previewSize) {
                info += "\n... (" + wxString::Format("%d", (int)(data.size() - previewSize)) + " more bytes)";
            }
        } else {
            info += "Empty data";
        }
        
        m_infoText->SetLabel(info);
        m_infoText->Show();
    }
    
    Layout(); // Refresh the layout
}

void MyFrame::GenerateObjectNames(std::vector<std::shared_ptr<DataParser::DataObject>>& objects) {
    static int index = 0;
    for (auto& obj : objects) {
        // Only generate name if NAME property doesn't exist
        if (obj->getProperty('NAME').empty()) {
            // Format index with leading zeros (001, 002, etc)
            std::string indexStr = std::to_string(index);
            indexStr = std::string(3 - indexStr.length(), '0') + indexStr;
            
            // Get type ID as string
            std::string typeStr = DataParser::ConvertIDToString(obj->typeID);
            // Remove spaces at the start and end of the type string
            typeStr.erase(0, typeStr.find_first_not_of(" "));
            typeStr.erase(typeStr.find_last_not_of(" ") + 1);
            
            // Set the name property
            obj->setProperty('NAME', indexStr + "_" + typeStr);
            index++;
        }
        
        // Process nested objects if any
        if (obj->isNested()) {
            GenerateObjectNames(obj->getNestedObjects());
        }
    }
}

void MyFrame::OnLoad(wxCommandEvent& event)
{
    // Check for unsaved changes
    if (HasUnsavedChanges()) {
        wxMessageDialog dialog(this, 
            "Do you want to save changes to the current file?",
            "Save Changes?",
            wxYES_NO | wxCANCEL | wxICON_QUESTION);
        
        int result = dialog.ShowModal();
        
        if (result == wxID_CANCEL) {
            return;  // User cancelled
        }
        
        if (result == wxID_YES) {
            // Try to save current file
            wxCommandEvent saveEvent;
            OnSave(saveEvent);
            
            // If save failed, don't proceed with load
            if (HasUnsavedChanges()) {
                return;
            }
        }
    }

    // Stop any ongoing playback before loading new file
    ResetAVPlayback();

    {
        wxFileDialog openFileDialog(this, "Open DAT file", "", "",
                                   "DAT files (*.dat)|*.dat", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

        if (openFileDialog.ShowModal() == wxID_CANCEL) {
            return;
        }

        wxString path = openFileDialog.GetPath();
        std::string pathStr = path.ToStdString();
        
        logInfo("Loading file: " + pathStr);
        
        // Store the file path
        m_currentFilePath = pathStr;
        
        // Clear existing data
        m_objects.clear();
        m_tree->DeleteAllItems();
        m_details->DeleteAllItems();
        m_imagePreview->SetBitmap(wxNullBitmap);
        
        // Try to load the file
        std::string password = m_grabberInfo.GetPassword();
        auto [success, isCompressed] = DataParser::LoadPackfile(pathStr, m_objects, password);
        if (success) {
            // Check for info object and create default one if not present
            // Parse existing info object
            m_grabberInfo.ParseDataObject(m_objects);

            logInfo("Successfully loaded file: " + pathStr + " with " + std::to_string(m_objects.size()) + " objects");
            
            // Set window title to show loaded file
            SetTitle(GrabberName + " - " + path);
            
            // Update editing text with current file path
            m_editingText->SetValue(wxString::FromUTF8(m_currentFilePath));

            // Generate names for objects that don't have a NAME property
            GenerateObjectNames(m_objects);

            // Update UI with info object data (whether default or parsed)
            UpdateUIFromGrabberInfo();
            
            // Update tree display
            RefreshTreeDisplay();
            
            // Select root element and update preview
            wxTreeItemId rootId = m_tree->GetRootItem();
            if (rootId.IsOk()) {
                m_tree->SelectItem(rootId);
            }

            // Set modified flag to false since we just loaded a file
            SetModified(false);
        } else {
            // Load failed - show error message to user
            logError("Failed to load file: " + pathStr);
            wxMessageBox("Failed to load file: " + path + "\n\nPlease check that the file exists and is not corrupted, or try a different password.", 
                        "Load Error", wxOK | wxICON_ERROR, this);
            
            // Reset file path since load failed
            m_currentFilePath.clear();
            m_editingText->SetValue("");
            SetTitle(GrabberName);
        }
    } // Dialog is automatically destroyed here

    // Force a layout update after dialog is closed
    Layout();
    Refresh();
}

void MyFrame::OnSave(wxCommandEvent& event)
{
    if (m_currentFilePath.empty()) {
        // If no file is opened, prompt for save location
        wxCommandEvent saveAsEvent;
        OnSaveAs(saveAsEvent);
        return;
    }

    // Update GrabberInfo from UI controls
    long xGrid = 3000, yGrid = 300;
    m_xGridText->GetValue().ToLong(&xGrid);
    m_yGridText->GetValue().ToLong(&yGrid);
    m_grabberInfo.SetXGrid(xGrid);
    m_grabberInfo.SetYGrid(yGrid);

    // Get options from menu
    wxMenuBar* menuBar = GetMenuBar();
    wxMenu* optionsMenu = menuBar->GetMenu(menuBar->FindMenu("O&ptions"));
    m_grabberInfo.SetBackup(optionsMenu->IsChecked(ID_BACKUP_DATAFILES));
    m_grabberInfo.SetDither(optionsMenu->IsChecked(ID_DITHER_IMAGES));
    m_grabberInfo.SetSort(optionsMenu->IsChecked(ID_SORT_OBJECTS));
    m_grabberInfo.SetRelativeFilenames(optionsMenu->IsChecked(ID_STORE_RELATIVE));
    m_grabberInfo.SetTransparency(optionsMenu->IsChecked(ID_PRESERVE_TRANSPARENCY));

    logInfo("Saving file: " + m_currentFilePath + 
            " (Backup: " + (m_grabberInfo.GetBackup() ? "enabled" : "disabled") + 
            ", Compression: " + (m_grabberInfo.GetPack() != GrabberInfo::CompressionMode::None ? "enabled" : "disabled") + 
            ", Objects: " + std::to_string(m_objects.size()) + ")");

    // Create temp objects array with info object
    std::vector<std::shared_ptr<DataParser::DataObject>> objectsWithInfo = m_objects;
    std::shared_ptr<DataParser::DataObject> infoObj = std::make_shared<DataParser::DataObject>(m_grabberInfo.SerializeToDataObject());
    objectsWithInfo.push_back(infoObj);

    std::string password = m_grabberInfo.GetPassword();
    if (DataParser::SavePackfile(m_currentFilePath, objectsWithInfo, m_grabberInfo.GetBackup(), 
                               m_grabberInfo.GetPack() != GrabberInfo::CompressionMode::None, password)) {
        wxString path = wxString::FromUTF8(m_currentFilePath);
        SetStatusText("File saved successfully: " + path);
        logInfo("Successfully saved file: " + m_currentFilePath);
        
        // Generate header file if header text is not empty
        if (!GenerateHeaderFile(m_currentFilePath)) {
            wxMessageBox("Warning: Failed to generate header file", "Warning", wxOK | wxICON_WARNING, this);
        }
        
        // Clear modified flag
        SetModified(false);
    } else {
        wxString path = wxString::FromUTF8(m_currentFilePath);
        logError("Failed to save file: " + m_currentFilePath);
        wxMessageBox("Failed to save file: " + path, "Error", wxOK | wxICON_ERROR, this);
    }
}

void MyFrame::OnSaveAs(wxCommandEvent& event)
{
    wxFileDialog saveFileDialog(this, "Save DAT file", "", "",
                              "DAT files (*.dat)|*.dat", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;

    // Store the new file path
    m_currentFilePath = saveFileDialog.GetPath().ToStdString();
    
    // Update window title and editing text
    SetTitle(GrabberName + " - " + saveFileDialog.GetPath());
    m_editingText->SetValue(wxString::FromUTF8(m_currentFilePath));
    
    // Call OnSave to perform the actual save operation
    wxCommandEvent saveEvent;
    OnSave(saveEvent);
}

void MyFrame::OnSaveStripped(wxCommandEvent& event)
{
    // Create and show the Save Stripped dialog
    wxDialog* dialog = new wxDialog(this, wxID_ANY, "Save Stripped",
                                  wxDefaultPosition, wxDefaultSize,
                                  wxDEFAULT_DIALOG_STYLE);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create radio buttons for stripping options
    wxRadioBox* stripOptions = new wxRadioBox(dialog, wxID_ANY, "",
                                             wxDefaultPosition, wxDefaultSize,
                                             wxArrayString{
                                                 "Save everything",
                                                 "Strip grabber information",
                                                 "Strip all object properties"
                                             },
                                             1, wxRA_SPECIFY_COLS);

    mainSizer->Add(stripOptions, 0, wxEXPAND | wxALL, 5);

    // Add OK and Cancel buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(dialog, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(dialog, wxID_CANCEL, "Cancel");
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    dialog->SetSizer(mainSizer);
    mainSizer->Fit(dialog);
    dialog->Center();

    // Show dialog and handle result
    if (dialog->ShowModal() == wxID_OK) {
        int selection = stripOptions->GetSelection();
        
        // Create a copy of objects for stripping
        std::vector<std::shared_ptr<DataParser::DataObject>> strippedObjects = m_objects;
        
        switch (selection) {
            case 1: // Strip grabber information, already done
                // Iterate through all objects and remove their properties except NAME
                for (auto& obj : strippedObjects) {
                    // Save NAME property if it exists
                    auto nameIt = obj->properties.find('NAME');
                    std::string nameProp;
                    if (nameIt != obj->properties.end()) {
                        nameProp = nameIt->second;
                    }
                    
                    // Clear all properties
                    obj->properties.clear();
                    obj->propertyOrder.clear();
                    
                    // Restore NAME property if it existed
                    if (!nameProp.empty()) {
                        obj->properties['NAME'] = nameProp;
                        obj->propertyOrder.push_back('NAME');
                    }
                }
                break;
                
            case 2: // Strip all object properties
                // Iterate through all objects and remove their properties
                for (auto& obj : strippedObjects) {
                    obj->properties.clear();
                }
                break;
                
            case 0: // Save everything
                // Add info object to objects array
                strippedObjects.push_back(std::make_shared<DataParser::DataObject>(m_grabberInfo.SerializeToDataObject()));
                break;
            default:
                // No stripping needed
                break;
        }
        
        // Show save file dialog
        wxFileDialog saveFileDialog(this, "Save Stripped DAT file", "", "",
                                  "DAT files (*.dat)|*.dat", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (saveFileDialog.ShowModal() == wxID_OK) {
            std::string strippedPath = saveFileDialog.GetPath().ToStdString();
            
            // Save the stripped file
            std::string password = m_grabberInfo.GetPassword();
            if (DataParser::SavePackfile(strippedPath, strippedObjects, false, 
                m_grabberInfo.GetPack() != GrabberInfo::CompressionMode::None, password)) {
                SetStatusText("Stripped file saved successfully: " + saveFileDialog.GetPath());
                logInfo("Successfully saved stripped file: " + strippedPath);

                // Generate header file if header text is not empty
                if (!GenerateHeaderFile(strippedPath)) {
                    wxMessageBox("Warning: Failed to generate header file", "Warning", wxOK | wxICON_WARNING, this);
                }

                // Update current file path and UI
                m_currentFilePath = strippedPath;
                SetTitle(GrabberName + " - " + saveFileDialog.GetPath());
                m_editingText->SetValue(wxString::FromUTF8(m_currentFilePath));
                             
                // Remove info object from strippedObjects
                GrabberInfo strippedInfoObj;
                strippedInfoObj.ParseDataObject(strippedObjects);
                // Update our working objects with the stripped version
                m_objects = std::move(strippedObjects);
                
                // If we stripped all properties, generate new names
                if (selection == 2) {
                    GenerateObjectNames(m_objects);
                }
                
                // Update tree display with stripped objects
                RefreshTreeDisplay();
            } else {
                wxMessageBox("Failed to save stripped file: " + saveFileDialog.GetPath(), 
                           "Error", wxOK | wxICON_ERROR, this);
                logError("Failed to save stripped file: " + strippedPath);
            }
        }
    }

    dialog->Destroy();
}

void MyFrame::OnNotImplemented(wxCommandEvent& event)
{
    wxMessageBox("This feature is under construction.", "Not Implemented", 
                wxOK | wxICON_INFORMATION, this);
}

// Helper function to validate object selection
bool MyFrame::ValidateObjectSelection(const wxString& operation, bool requireBitmap, bool requireAudio, bool requireFont, bool requireVideo) {
    // Check if there's a selected object
    if (!m_currentObject) {
        wxMessageBox("No object selected. Please select an object first.", 
                    "No Selection", wxOK | wxICON_INFORMATION);
        return false;
    }

    // If no specific type requirements, just check that object exists
    if (!requireBitmap && !requireAudio && !requireFont && !requireVideo) {
        return true;
    }

    // Check if the object matches any of the required types (OR logic, not AND)
    bool isValidType = false;
    wxString validTypes;
    
    if (requireBitmap) {
        if (m_currentObject->isBitmap()) {
            isValidType = true;
        }
        validTypes += "bitmap";
    }
    
    if (requireAudio) {
        if (m_currentObject->isAudio()) {
            isValidType = true;
        }
        if (!validTypes.empty()) validTypes += ", ";
        validTypes += "audio";
    }
    
    if (requireFont) {
        if (m_currentObject->isFont()) {
            isValidType = true;
        }
        if (!validTypes.empty()) validTypes += ", ";
        validTypes += "font";
    }
    
    if (requireVideo) {
        if (m_currentObject->isVideo()) {
            isValidType = true;
        }
        if (!validTypes.empty()) validTypes += ", ";
        validTypes += "video";
    }

    if (!isValidType) {
        wxMessageBox(wxString::Format("Selected object is not a valid type for %s operation.\nSupported types: %s", 
                                    operation, validTypes), 
                    "Invalid Object Type", wxOK | wxICON_ERROR);
        return false;
    }

    return true;
}

void MyFrame::UpdateObjectPreview() {
    UpdatePropertyList(m_currentObject);
    UpdatePreviewControls(m_currentObject);
}

// Helper: Get all selected objects in the tree
std::vector<std::shared_ptr<DataParser::DataObject>> MyFrame::GetSelectedObjects() {
    std::vector<std::shared_ptr<DataParser::DataObject>> result;
    wxArrayTreeItemIds selectedItems;
    m_tree->GetSelections(selectedItems);
    for (auto& item : selectedItems) {
        ObjectTreeData* data = static_cast<ObjectTreeData*>(m_tree->GetItemData(item));
        if (data && data->object) {
            result.push_back(data->object);
        }
    }
    return result;
}

// Update OnDelete to support multiple selection
void MyFrame::OnDelete(wxCommandEvent& event)
{
    auto selectedObjs = GetSelectedObjects();
    if (selectedObjs.empty()) {
        wxMessageBox("No objects selected for deletion.", "Delete", wxOK | wxICON_INFORMATION);
        return;
    }
    wxString message;
    if (selectedObjs.size() == 1) {
        message = wxString::Format("Are you sure you want to delete the object '%s'?", wxString::FromUTF8(selectedObjs[0]->name));
    } else {
        message = wxString::Format("Are you sure you want to delete %zu objects?", selectedObjs.size());
    }
    wxMessageDialog dialog(this, message, "Confirm Delete", wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION);
    if (dialog.ShowModal() != wxID_YES) {
        return;
    }
    int deletedCount = 0;
    for (auto& obj : selectedObjs) {
        bool deleted = ObjectTraversalUtils::ForEachObjectRecursive(m_objects, [this, &obj](std::shared_ptr<DataParser::DataObject>& candidate) -> bool {
            if (candidate == obj) {
                auto* parentVector = ObjectTraversalUtils::FindParentVector(m_objects, candidate);
                if (parentVector) {
                    ObjectTraversalUtils::FindAndRemoveObject(*parentVector, candidate);
                    return true;
                }
            }
            return false;
        });
        if (deleted) ++deletedCount;
    }
    m_currentObject = nullptr;
    if (deletedCount > 0) {
        SetModified(true);
        RefreshTreeDisplay();
        SetStatusText(wxString::Format("%d object(s) deleted", deletedCount));
    } else {
        wxMessageBox("Failed to delete selected objects.", "Delete Error", wxOK | wxICON_ERROR);
    }
}

// Add event handler for window close
void MyFrame::OnClose(wxCloseEvent& event)
{
    // Save settings before closing
    logInfo("Saving settings to allegro.cfg");
    m_grabberInfo.SaveSettings("allegro.cfg");
    event.Skip();
}

bool MyFrame::HasUnsavedChanges() const {
    return m_isModified;
}

void MyFrame::ResetToDefaults() {
    // Clear current file path
    m_currentFilePath.clear();
    
    // Clear data objects
    m_objects.clear();
    
    // Clear UI
    m_tree->DeleteAllItems();
    m_tree->AddRoot("<root>");
    m_tree->SelectItem(m_tree->GetRootItem());
    m_details->DeleteAllItems();
    m_infoText->SetLabel("");
    m_imagePreview->SetBitmap(wxNullBitmap);
    m_editingText->SetValue("");
    m_headerText->SetValue("");
    m_prefixText->SetValue("");
    // Note: Password field is not cleared - it's a global setting that persists across new files
    
    // Update UI with default values
    UpdateUIFromGrabberInfo();
    
    // Reset window title
    SetTitle(GrabberName);
    
    // Hide preview controls
    m_zoomSlider->Hide();
    m_zoomLabel->Hide();
    m_audioPanel->Hide();
    m_imagePreviewPanel->Hide();
    
    // Stop any ongoing playback
    ResetAVPlayback();
    
    // Clear current object
    m_currentObject = nullptr;
    
    // Clear modified flag
    SetModified(false);
    
    // Update status
    SetStatusText("New file");
    
    // Refresh layout
    Layout();
}

void MyFrame::OnNew(wxCommandEvent& event)
{
    // Check for unsaved changes
    if (HasUnsavedChanges()) {
        wxMessageDialog dialog(this, 
            "Do you want to save changes to the current file?",
            "Save Changes?",
            wxYES_NO | wxCANCEL | wxICON_QUESTION);
        
        int result = dialog.ShowModal();
        
        if (result == wxID_CANCEL) {
            return;  // User cancelled
        }
        
        if (result == wxID_YES) {
            // Try to save current file
            wxCommandEvent saveEvent;
            OnSave(saveEvent);
            
            // If save failed, don't proceed with new
            if (HasUnsavedChanges()) {
                return;
            }
        }
    }
    
    // Reset everything to defaults
    ResetToDefaults();
    
    // Set modified flag to false since we just created a new file
    SetModified(false);
}

void MyFrame::SetModified(bool modified) {
    if (m_isModified != modified) {
        m_isModified = modified;
        wxString title = GrabberName;
        if (!m_currentFilePath.empty()) {
            title += " - " + wxString::FromUTF8(m_currentFilePath);
        }
        if (modified) {
            title += "*";
        }
        SetTitle(title);
    }
}

void MyFrame::StopAVPlayback() {
    m_audioControl->Stop();
    m_videoPanel->Stop();
}

void MyFrame::ResetAVPlayback() {
    StopAVPlayback();
    m_playButton->SetBitmap(GetPlayIcon());
    m_videoPanel->SetVideoData(nullptr);
    m_audioControl->SetAudioData(nullptr);
    m_videoPanel->SetCurrentPositionMs(0);
    m_audioControl->SetCurrentPositionMs(0);
    m_AVSlider->SetValue(0);
    m_timeLabel->SetLabel("00:00.0 / " + FormatTime(m_playbackDuration));
}

void MyFrame::OnPlayPauseAV(wxCommandEvent& event) {
    if (!m_currentObject) return;
    
    // Don't process if button is disabled (e.g., for MIDI objects)
    if (!m_playButton->IsEnabled()) {
        return;
    }
    
    if (m_currentObject->isAudio()) {
        m_audioControl->PlayPause();
    } else if (m_currentObject->isVideo()) {
        m_videoPanel->PlayPause();
    } else {
        logWarning("Object to play is not audio or video");
    }
}

wxBitmap MyFrame::GetPlayIcon() {
    // Create bitmap directly from XPM data
    wxBitmap playBitmap(play_xpm);
    if (playBitmap.IsOk()) {
        return playBitmap;
    }
    
    // Fallback to default icon if XPM loading fails
    return wxArtProvider::GetBitmap("wxART_GO_FORWARD", wxART_BUTTON, wxSize(32, 32));
}

wxBitmap MyFrame::GetPauseIcon() {
    // Create bitmap directly from XPM data
    wxBitmap pauseBitmap(pause_xpm);
    if (pauseBitmap.IsOk()) {
        return pauseBitmap;
    }
    
    // Fallback to default icon if XPM loading fails
    return wxArtProvider::GetBitmap("wxART_STOP", wxART_BUTTON, wxSize(32, 32));
}

void MyFrame::UpdatePlayButtonIcon(bool isPlaying) {
    wxBitmap icon = isPlaying ? GetPauseIcon() : GetPlayIcon();
    m_playButton->SetBitmapLabel(icon);
    m_playButton->SetBitmapPressed(icon);
    m_playButton->SetBitmapHover(icon);
    m_playButton->SetBitmapDisabled(icon);
    m_playButton->SetBitmapCurrent(icon);
}

void MyFrame::UpdateSliderPosition() {
    /*if (!m_isPlaying) {
        ResetAudioPlayback();
        return;
    }

    wxTimeSpan elapsed = wxDateTime::Now() - m_playbackStartTime;
    int elapsedMs = elapsed.GetMilliseconds().ToLong();
    
    // Calculate position as percentage (0-100)
    // round up to the nearest integer
    int position = static_cast<int>(std::ceil(static_cast<double>(elapsedMs) * 100.0 / m_playbackDuration));
    position = std::clamp(position, 0, 100);
    
    m_audioSlider->SetValue(position);
    
    // Update time label
    m_timeLabel->SetLabel(FormatTime(elapsedMs) + " / " + FormatTime(m_playbackDuration));

    // Stop playback if we've reached the end
    if (position >= 100) {
        StopAVPlayback();
        UpdatePlayButtonIcon(false);  // Update to play icon when playback ends
        m_sliderTimer->Stop();
        // Set time label to total duration at the end
        m_timeLabel->SetLabel(FormatTime(m_playbackDuration) + " / " + FormatTime(m_playbackDuration));
    }*/
}

void MyFrame::OnSliderDrag(wxScrollEvent& event) {
    // Update time label while dragging
    int position = (m_AVSlider->GetValue() * m_playbackDuration) / 100;
    m_timeLabel->SetLabel(FormatTime(position) + " / " + FormatTime(m_playbackDuration));
}

void MyFrame::OnSliderRelease(wxScrollEvent& event) {
    if (!m_currentObject) {
        logInfo("No current object");
        return;
    }
    if (m_currentObject->isAudio()) {
        int position = (m_AVSlider->GetValue() * m_playbackDuration) / 100;
        logInfo("Slider released at position " + std::to_string(m_AVSlider->GetValue()/100.0));
        m_audioControl->SetCurrentPositionMs(position);
    } else if (m_currentObject->isVideo()) {
        int position = (m_AVSlider->GetValue() * m_playbackDuration) / 100;
        logInfo("Slider released at position " + std::to_string(m_AVSlider->GetValue()/100.0));
        m_videoPanel->SetCurrentPositionMs(position);
    } else {
        logWarning("Object to play is not audio or video");
    }
}

void MyFrame::OnGridChange(wxCommandEvent& event)
{
    // Get values from text controls
    long xGrid = 3000, yGrid = 300;
    if (!m_xGridText->GetValue().ToLong(&xGrid) || xGrid <= 0) {
        m_xGridText->SetValue("3000");
        xGrid = 3000;
    }
    if (!m_yGridText->GetValue().ToLong(&yGrid) || yGrid <= 0) {
        m_yGridText->SetValue("300");
        yGrid = 300;
    }

    // Update grid values
    m_xGridText->SetValue(wxString::Format("%ld", xGrid));
    m_yGridText->SetValue(wxString::Format("%ld", yGrid));

    // Mark as modified when grid values change
    SetModified(true);
}

void MyFrame::OnHeaderChange(wxCommandEvent& event)
{
    // Mark as modified when header, prefix, or password changes
    SetModified(true);
}

void MyFrame::OnPasswordChange(wxCommandEvent& event)
{
    // Update password in GrabberInfo from UI
    std::string newPassword = m_passwordText->GetValue().ToStdString();
    m_grabberInfo.SetPassword(newPassword);
    
    // Password changes don't set the modified flag since they don't affect the saved data
}

void MyFrame::OnIndexObjects(wxCommandEvent& event)
{
    m_showIndices = event.IsChecked();
    RefreshTreeDisplay();
}

void MyFrame::RefreshTreeDisplay()
{
    // Store currently selected object pointers
    std::vector<std::shared_ptr<DataParser::DataObject>> selectedObjs;
    wxArrayTreeItemIds selectedItems;
    m_tree->GetSelections(selectedItems);
    for (auto& item : selectedItems) {
        ObjectTreeData* data = static_cast<ObjectTreeData*>(m_tree->GetItemData(item));
        if (data && data->object) {
            selectedObjs.push_back(data->object);
        }
    }
    m_tree->DeleteAllItems();
    wxTreeItemId rootId = m_tree->AddRoot("<root>");
    logDebug("root id: " + std::to_string((uint64_t)rootId.GetID()));

    // If sorting is enabled, sort objects by name (recursively)
    if (m_grabberInfo.GetSort()) {
        this->SortObjectsByName(m_objects);
    }

    // If there are objects, add them to the tree
    if (!m_objects.empty()) {
        // First, count total objects to determine max index width
        int totalObjects = ObjectTraversalUtils::CountObjects(m_objects);
        logDebug("totalObjects: " + std::to_string(totalObjects));

        // Calculate width needed for index numbers
        int indexWidth = static_cast<int>(std::to_string(totalObjects).length());
        if (m_showIndices) {
            logDebug("indexWidth: " + std::to_string(indexWidth));
        }

        // Helper function to recursively add objects to tree
        std::function<void(const std::vector<std::shared_ptr<DataParser::DataObject>>&, wxTreeItemId, int&)> addObjectsToTree;
        addObjectsToTree = [&](const std::vector<std::shared_ptr<DataParser::DataObject>>& objects, wxTreeItemId parent, int& index) {
            for (const auto& obj : objects) {
                std::string objType = DataParser::ConvertIDToString(obj->typeID);
                std::string objName = obj->getProperty('NAME');
                std::string itemText;
                if (m_showIndices) {
                    std::string indexStr = std::to_string(index);
                    std::string padding(indexWidth - indexStr.length(), ' ');
                    itemText = wxString::Format("[%s%s] %s - %s", padding, indexStr, objType, objName);
                } else {
                    itemText = objType + " - " + objName;
                }
                wxTreeItemId item = m_tree->AppendItem(parent, itemText);
                m_tree->SetItemData(item, new ObjectTreeData(obj));
                index++;
                if (obj->isNested()) {
                    auto& nestedObjects = obj->getNestedObjects();
                    addObjectsToTree(nestedObjects, item, index);
                }
            }
        };
        int startIndex = 1; // Start indexing from 1
        addObjectsToTree(m_objects, rootId, startIndex);
        m_tree->ExpandAll();

        // Restore selection if we had selected objects
        if (!selectedObjs.empty()) {
            // Helper function to recursively search tree items
            std::function<void(wxTreeItemId)> findAndSelectObjects;
            findAndSelectObjects = [&](wxTreeItemId parent) {
                wxTreeItemIdValue cookie;
                wxTreeItemId item = m_tree->GetFirstChild(parent, cookie);
                while (item.IsOk()) {
                    ObjectTreeData* data = static_cast<ObjectTreeData*>(m_tree->GetItemData(item));
                    if (data && data->object) {
                        for (const auto& selObj : selectedObjs) {
                            if (data->object == selObj) {
                                m_tree->SelectItem(item);
                                break;
                            }
                        }
                    }
                    if (m_tree->ItemHasChildren(item)) {
                        findAndSelectObjects(item);
                    }
                    item = m_tree->GetNextSibling(item);
                }
            };
            findAndSelectObjects(rootId);
        }
    }
    // Always ensure root is visible and selected when no other selection exists
    wxArrayTreeItemIds afterSelects;
    m_tree->GetSelections(afterSelects);
    if (afterSelects.IsEmpty()) {
        m_tree->SelectItem(rootId);
    }
}

void MyFrame::OnHelp(wxCommandEvent& event)
{
    wxMessageBox("Help is under construction.", "Help", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnZoomChange(wxCommandEvent& event) {
    if (!m_currentObject || (!m_currentObject->isBitmap() && !m_currentObject->isFont())) {
        return;
    }

    double zoomLevel = m_zoomSlider->GetValue() / 10.0;  // Convert 5-30 to 0.5-3.0
    m_zoomLabel->SetLabel(wxString::Format("Zoom: %.1fx", zoomLevel));
    
    if (m_currentObject->isBitmap()) {
        const BitmapData& bmpData = m_currentObject->getBitmap();
        wxImage image;
        if (bmpData.toWxImage(image, m_currentPalette)) {
            UpdateImageDisplay(image, zoomLevel, true);  // Add border for bitmaps
        }
    }
    else if (m_currentObject->isFont()) {
        // Handle font zoom
        wxImage fontImg = m_currentObject->getFont().getPreviewImage();
        if (fontImg.IsOk()) {
            UpdateImageDisplay(fontImg, zoomLevel, false);  // No border for fonts
        }
    }
}

void MyFrame::OnPackModeChanged(wxCommandEvent& event) {
    int selection = m_compressionBox->GetSelection();
    switch (selection) {
        case 0: // No compression
            logInfo("Setting pack mode to None");
            m_grabberInfo.SetPack(GrabberInfo::CompressionMode::None);
            break;
        case 1: // Individual compression
            logInfo("Setting pack mode to Individual");
            m_grabberInfo.SetPack(GrabberInfo::CompressionMode::Individual);
            break;
        case 2: // Global compression
            logInfo("Setting pack mode to Global");
            m_grabberInfo.SetPack(GrabberInfo::CompressionMode::Global);
            break;
    }
    SetModified(true);
}

void MyFrame::OnMerge(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, "Open DAT file to merge", "", "",
                               "DAT files (*.dat)|*.dat", wxFD_OPEN|wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxString path = openFileDialog.GetPath();
    std::string pathStr = path.ToStdString();
    
    logInfo("Merging file: " + pathStr);
    
    // Load objects from the file to merge
    std::vector<std::shared_ptr<DataParser::DataObject>> mergeObjects;
    std::string password = m_grabberInfo.GetPassword();
    auto [success, isCompressed] = DataParser::LoadPackfile(pathStr, mergeObjects, password);
    
    if (success) {
        // Parse and remove info object from merge file
        GrabberInfo mergeInfoObj;
        mergeInfoObj.ParseDataObject(mergeObjects);
        
        // Add all objects from merge file to current objects
        m_objects.insert(m_objects.end(), mergeObjects.begin(), mergeObjects.end());
        
        // Generate names for newly added objects
        GenerateObjectNames(m_objects);
        
        // Update tree display
        RefreshTreeDisplay();
        
        // Mark as modified
        SetModified(true);
        
        logInfo("Successfully merged " + std::to_string(mergeObjects.size()) + " objects from " + pathStr);
        SetStatusText("Successfully merged " + std::to_string(mergeObjects.size()) + " objects");
    } else {
        wxMessageBox("Failed to load file: " + path, "Error", wxOK | wxICON_ERROR, this);
        logError("Failed to load file for merging: " + pathStr);
    }
}

void MyFrame::UpdateObjects(std::vector<std::shared_ptr<DataParser::DataObject>>& objects, bool ForceUpdate) {
    // Create a dialog to show update results
    wxDialog* dialog = new wxDialog(this, wxID_ANY, "Update Results", 
                                  wxDefaultPosition, wxSize(800, 600));
    
    // Create a text control to display results
    wxTextCtrl* textCtrl = new wxTextCtrl(dialog, wxID_ANY, "", 
                                         wxDefaultPosition, wxDefaultSize,
                                         wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP | wxHSCROLL);
    
    // Use monospace font for better alignment
    wxFont monoFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    textCtrl->SetFont(monoFont);

    // Create sizer for dialog
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(textCtrl, 1, wxEXPAND | wxALL, 5);

    // Add OK button at the bottom
    wxButton* okButton = new wxButton(dialog, wxID_OK, "OK");
    sizer->Add(okButton, 0, wxALIGN_CENTER | wxBOTTOM, 5);

    dialog->SetSizer(sizer);

    // Process all objects using utility function
    bool anyUpdated = false;
    std::vector<std::string> updateResults;

    ObjectTraversalUtils::ForEachObjectRecursive(objects, [&](const std::shared_ptr<DataParser::DataObject>& obj) -> bool {
        // Try to update the object
        std::string errorMsg;
        if (obj->update(errorMsg, ForceUpdate, &m_currentPalette, m_grabberInfo.GetDither())) {
            anyUpdated = true;
            // Get the path of the object
            std::string originalPath = obj->getProperty('ORIG');
            updateResults.push_back("Updating " + originalPath + " -> " + obj->name);
        } else {
            if (!errorMsg.empty()) {
                updateResults.push_back(errorMsg);
            }
            else {
                updateResults.push_back(obj->name + " - skipped");
            }
        }
        return false; // Continue processing
    });

    // Add "Done!" at the end
    updateResults.push_back("Done!");

    // Update text control with results
    wxString text;
    for (const auto& result : updateResults) {
        text += result + "\n";
    }
    textCtrl->SetValue(text);

    // If any objects were updated, mark as modified
    if (anyUpdated) {
        SetModified(true);
        RefreshTreeDisplay();
    }

    // Show dialog
    dialog->ShowModal();
    dialog->Destroy();
}

void MyFrame::OnUpdate(wxCommandEvent& event) {
    UpdateObjects(m_objects);
    UpdateObjectPreview();
}

void MyFrame::OnUpdateSelection(wxCommandEvent& event) {
    if (m_currentObject == nullptr) {
        // No object selected
        return;
    }

    // Create a temporary vector with just the selected object
    std::vector<std::shared_ptr<DataParser::DataObject>> selectedObjects;
    selectedObjects.push_back(m_currentObject);

    // Update the selected object
    UpdateObjects(selectedObjects);
    m_currentObject = selectedObjects[0];

    UpdateObjectPreview();
}

void MyFrame::OnForceUpdate(wxCommandEvent& event) {
    UpdateObjects(m_objects, true);
    UpdateObjectPreview();
}

void MyFrame::OnForceUpdateSelection(wxCommandEvent& event) {
    if (m_currentObject == nullptr) {
        // No object selected
        return;
    }

    // Create a temporary vector with just the selected object
    std::vector<std::shared_ptr<DataParser::DataObject>> selectedObjects;
    selectedObjects.push_back(m_currentObject);

    // Update the selected object with force update
    UpdateObjects(selectedObjects, true);
    m_currentObject = selectedObjects[0];

    UpdateObjectPreview();
}

MyFrame::~MyFrame() {
}

void MyFrame::OnGrab(wxCommandEvent& event) {
    // Check if there's a selected object (allow all types for grab operation)
    if (!ValidateObjectSelection("grab")) {
        return;
    }

    wxString path;
    std::string objectType;

    // Determine object type and handle accordingly
    if (m_currentObject->isBitmap()) {
        objectType = "bitmap";
        if (!GrabBitmap(path)) return;
    } 
    else if (m_currentObject->isAudio()) {
        objectType = "audio";
        if (!GrabAudio(path)) return;
    }
    else if (m_currentObject->isFont()) {
        objectType = "font";
        if (!GrabFont(path)) return;
    }
    else if (m_currentObject->isVideo()) {
        objectType = "video";
        if (!GrabVideo(path)) return;
    }
    else if (m_currentObject->isRawData()) {
        objectType = "raw binary data";
        if (!GrabRawBinary(path)) return;
    }
    else if (m_currentObject->isNested()) {
        objectType = "datafile";
        if (!GrabNested(path)) return;
    }

    // Log the grab operation
    logInfo("Successfully grabbed " + objectType + " from " + path.ToStdString() + " into " + m_currentObject->name);
}

void MyFrame::UpdateObjectAfterGrab(const std::string& path, const std::string& objectType) {
    // Get a non-const reference to the current object
    DataParser::DataObject& obj = const_cast<DataParser::DataObject&>(*m_currentObject);
    
    // Update ORIG property with correct format based on current setting
    SetOrigPropertyWithFormat(m_currentObject, path);
    
    // Update date property
    obj.updateDateProperty();

    // Mark as modified
    SetModified(true);

    // Update preview and property list
    UpdateObjectPreview();

    // Show success message
    SetStatusText(wxString::Format("Successfully loaded %s: %s", objectType, wxString::FromUTF8(path)));
}

bool MyFrame::GrabBitmap(wxString& path) {
    wxImage image;

    // Check if we have a previously loaded image
    if (m_loadedImage && m_loadedImage->IsOk()) {
        // Show preview dialog for region selection
        long xGrid = 0, yGrid = 0;
        m_xGridText->GetValue().ToLong(&xGrid);
        m_yGridText->GetValue().ToLong(&yGrid);
        GrabPreviewDialog dialog(this, *m_loadedImage, "Select Region to Grab", xGrid, yGrid);
        if (dialog.ShowModal() == wxID_OK) {
            wxRect selection = dialog.GetSelection();
            if (selection.width > 0 && selection.height > 0) {
                // Create a sub-image from the selection
                image = m_loadedImage->GetSubImage(selection);
                logInfo("Using selected region from loaded image: " + path.ToStdString() + 
                       " (" + std::to_string(selection.x) + "," + std::to_string(selection.y) + 
                       "," + std::to_string(selection.width) + "," + std::to_string(selection.height) + ")");
            } else {
                // No valid selection, use the whole image
                image = *m_loadedImage;
                logInfo("No region selected, using whole image: " + path.ToStdString());
            }
            path = m_loadedBitmapPath;
        } else {
            return false; // User cancelled
        }
    } else {
        // Create file dialog to select bitmap file
        wxFileDialog openFileDialog(this, "Open Bitmap File", "", "",
                                   "Bitmap files (*.bmp;*.png;*.jpg;*.jpeg;*.tga;*.pcx)|*.bmp;*.png;*.jpg;*.jpeg;*.tga;*.pcx",
                                   wxFD_OPEN|wxFD_FILE_MUST_EXIST);

        if (openFileDialog.ShowModal() == wxID_CANCEL)
            return false;

        path = openFileDialog.GetPath();
        
        // Load the image
        if (!BitmapData::readFileToWxImage(path, image)) {
            wxMessageBox("Failed to load image file: " + path, "Error", wxOK | wxICON_ERROR);
            return false;
        }
    }

    // Get the current bitmap data to determine bits and type
    const BitmapData& currentBmp = m_currentObject->getBitmap();
    int bits = currentBmp.bits;
    
    // Create new bitmap data
    BitmapData newBmp;
    newBmp.typeID = currentBmp.typeID;  // Keep the same type ID

    // Load the image into the bitmap data
    if (!newBmp.loadFromWxImage(image, bits, &m_currentPalette, m_grabberInfo.GetDither(), m_grabberInfo.GetTransparency())) {
        wxMessageBox("Failed to convert image to bitmap format.", "Error", wxOK | wxICON_ERROR);
        return false;
    }

    // Get a non-const reference to the current object and update data
    DataParser::DataObject& obj = const_cast<DataParser::DataObject&>(*m_currentObject);
    obj.data = newBmp;
    
    UpdateObjectAfterGrab(path.ToStdString(), "bitmap");
    return true;
}

bool MyFrame::GrabAudio(wxString& path) {
    // Determine file filter based on audio object type
    wxString filter;
    wxString dialogTitle;
    
    if (m_currentObject->typeID == ObjectType::DAT_SAMP) {
        filter = "WAV files (*.wav)|*.wav";
        dialogTitle = "Open WAV File";
    } else if (m_currentObject->typeID == ObjectType::DAT_MIDI) {
        filter = "MIDI files (*.mid)|*.mid";
        dialogTitle = "Open MIDI File";
    } else {
        filter = "OGG files (*.ogg)|*.ogg";
        dialogTitle = "Open OGG File";
    }
    
    // Create file dialog to select audio file
    wxFileDialog openFileDialog(this, dialogTitle, "", "", filter, wxFD_OPEN|wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return false;

    path = openFileDialog.GetPath();
    
    // Get a non-const reference to the current object and update data
    DataParser::DataObject& obj = const_cast<DataParser::DataObject&>(*m_currentObject);
    
    // Create new audio data
    AudioData audioData = obj.getAudio();
    // Try to load the audio file
    if (!audioData.importFromFile(path.ToStdString())) {
        wxMessageBox("Failed to load audio file: " + path, "Error", wxOK | wxICON_ERROR);
        return false;
    }
    
    obj.data = audioData;
    UpdateObjectAfterGrab(path.ToStdString(), "audio");
    return true;
}

bool MyFrame::GrabFont(wxString& path) {
    // Show file dialog with filters for BMP, PCX, TGA, and FNT
    wxFileDialog openFileDialog(this, "Select file to import as font range",
                           "", "", 
                           "Font and bitmap files (*.bmp;*.pcx;*.tga;*.fnt)|*.bmp;*.pcx;*.tga;*.fnt|"
                           "Allegro or BIOS font files (*.fnt)|*.fnt|"
                           "Bitmap files (*.bmp;*.pcx;*.tga)|*.bmp;*.pcx;*.tga|"
                           "BMP files (*.bmp)|*.bmp|"
                           "PCX files (*.pcx)|*.pcx|"
                           "TGA files (*.tga)|*.tga|"
                           "All files (*.*)|*.*",
                           wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (openFileDialog.ShowModal() != wxID_OK) {
        return false; // User cancelled
    }
    
    path = openFileDialog.GetPath();

    // Get a non-const reference to the current object and update data
    DataParser::DataObject& obj = const_cast<DataParser::DataObject&>(*m_currentObject);

    // Create new font data
    FontData fontData;
    fontData.typeID = obj.typeID;
    // Try to load the font file
    if (!FontEditDialog::GrabFontFromFile(this, fontData, path)) {
        wxMessageBox("Failed to load font file: " + path, "Error", wxOK | wxICON_ERROR);
        return false;
    }

    obj.data = fontData;
    
    UpdateObjectAfterGrab(path.ToStdString(), "font");
    return true;
}

bool MyFrame::GrabVideo(wxString& path) {
    // Create file dialog to select video file
    wxFileDialog openFileDialog(this, "Open Video File", "", "",
                               "Video files (*.fli;*.flc)|*.fli;*.flc",
                               wxFD_OPEN|wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return false;

    path = openFileDialog.GetPath();
    
    // Get a non-const reference to the current object and update data
    DataParser::DataObject& obj = const_cast<DataParser::DataObject&>(*m_currentObject);

    // Create new video data
    VideoData videoData;
    videoData.typeID = obj.typeID;
    // Try to load the video file
    if (!videoData.importFromFile(path.ToStdString())) {
        wxMessageBox("Failed to load video file: " + path, "Error", wxOK | wxICON_ERROR);
        return false;
    }

    obj.data = videoData;
    
    UpdateObjectAfterGrab(path.ToStdString(), "video");
    return true;
}

bool MyFrame::GrabRawBinary(wxString& path) {
    // Create file dialog to select any file (no extension filter)
    wxFileDialog openFileDialog(this, "Open Binary File", "", "",
                               "All files (*.*)|*.*",
                               wxFD_OPEN|wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return false;

    path = openFileDialog.GetPath();
    
    // Read the binary file data
    std::vector<uint8_t> binaryData;
    std::ifstream file(path.ToStdString(), std::ios::binary);
    if (!file) {
        wxMessageBox("Failed to open file: " + path, "Error", wxOK | wxICON_ERROR);
        return false;
    }

    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read file data
    binaryData.resize(fileSize);
    file.read(reinterpret_cast<char*>(binaryData.data()), fileSize);
    file.close();

    if (!file.good() && !file.eof()) {
        wxMessageBox("Failed to read file: " + path, "Error", wxOK | wxICON_ERROR);
        return false;
    }
    
    // Get a non-const reference to the current object and update data
    DataParser::DataObject& obj = const_cast<DataParser::DataObject&>(*m_currentObject);
    obj.data = binaryData;
    
    UpdateObjectAfterGrab(path.ToStdString(), "raw binary data");
    return true;
}

void MyFrame::ShowImageDialog(const wxString& title) {
    // Create and show the custom dialog
    BitmapPreviewDialog dialog(this, title, *m_loadedImage);
    dialog.ShowModal();
}

void MyFrame::OnReadBitmap(wxCommandEvent& event) {
    // Create file dialog to select bitmap file
    wxFileDialog openFileDialog(this, "Open Bitmap File", "", "",
                               "Bitmap files (*.bmp;*.png;*.jpg;*.jpeg;*.tga;*.pcx)|*.bmp;*.png;*.jpg;*.jpeg;*.tga;*.pcx",
                               wxFD_OPEN|wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxString path = openFileDialog.GetPath();
    m_loadedBitmapPath = path;  // Store the loaded bitmap path
    
    // Create a new shared pointer for the image
    m_loadedImage = std::make_shared<wxImage>();
    bool success = false;
    
    // Check if it's a PCX file
    success = BitmapData::readFileToWxImage(path, *m_loadedImage);
    
    if (!success) {
        wxMessageBox("Failed to load image file: " + path, "Error", wxOK | wxICON_ERROR);
        m_loadedImage.reset();  // Clear the shared pointer on failure
        return;
    }

    // Show the image in a dialog
    ShowImageDialog("Bitmap Preview");
}

void MyFrame::OnViewBitmap(wxCommandEvent& event) {
    // Check if we have a previously loaded image
    if (!m_loadedImage) {
        wxMessageBox("No bitmap has been loaded yet. Please use 'Read Bitmap' first.", 
                    "No Bitmap Loaded", wxOK | wxICON_INFORMATION);
        return;
    }

    // Show the image in a dialog
    ShowImageDialog("Bitmap Preview");
}

void MyFrame::OnGrabFromGrid(wxCommandEvent& event) {
    // Create dialog
    wxDialog* dialog = new wxDialog(this, wxID_ANY, "Grab from Grid",
                                  wxDefaultPosition, wxDefaultSize,
                                  wxDEFAULT_DIALOG_STYLE);

    // Create main sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create grid type box
    wxStaticBox* gridTypeBox = new wxStaticBox(dialog, wxID_ANY, "Grid Type");
    wxStaticBoxSizer* gridTypeSizer = new wxStaticBoxSizer(gridTypeBox, wxVERTICAL);

    // Create radio buttons and grid controls
    wxBoxSizer* gridOptionsSizer = new wxBoxSizer(wxVERTICAL);
    
    // Add "Use col" option with color picker
    wxRadioButton* useColRadio = new wxRadioButton(gridTypeBox, wxID_ANY, "Use color grid", 
                                                  wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    gridOptionsSizer->Add(useColRadio, 0, wxALL, 5);

    wxBoxSizer* colorPickerSizer = new wxBoxSizer(wxHORIZONTAL);
    colorPickerSizer->AddSpacer(15);
    wxStaticText* colorLabel = new wxStaticText(gridTypeBox, wxID_ANY, "Grid Color:");
    wxColourPickerCtrl* colorPicker = new wxColourPickerCtrl(gridTypeBox, wxID_ANY, wxColour(255, 255, 255));
    colorPickerSizer->Add(colorLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    colorPickerSizer->Add(colorPicker, 0, wxALIGN_CENTER_VERTICAL);
    gridOptionsSizer->Add(colorPickerSizer, 0, wxLEFT | wxBOTTOM, 5);

    wxRadioButton* regularGridRadio = new wxRadioButton(gridTypeBox, wxID_ANY, "Regular grid");
    gridOptionsSizer->Add(regularGridRadio, 0, wxALL, 5);

    wxBoxSizer* gridValuesSizer = new wxBoxSizer(wxHORIZONTAL);
    gridValuesSizer->AddSpacer(15);
    
    wxStaticText* xGridLabel = new wxStaticText(gridTypeBox, wxID_ANY, "X-grid:");
    wxTextCtrl* xGridText = new wxTextCtrl(gridTypeBox, wxID_ANY, wxString::Format("%d", m_grabberInfo.GetGriddleXGrid()),
                                          wxDefaultPosition, wxSize(40, -1));
    wxStaticText* yGridLabel = new wxStaticText(gridTypeBox, wxID_ANY, "Y-grid:");
    wxTextCtrl* yGridText = new wxTextCtrl(gridTypeBox, wxID_ANY, wxString::Format("%d", m_grabberInfo.GetGriddleYGrid()),
                                          wxDefaultPosition, wxSize(40, -1));
    
    gridValuesSizer->Add(xGridLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    gridValuesSizer->Add(xGridText, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    gridValuesSizer->Add(yGridLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    gridValuesSizer->Add(yGridText, 0, wxALIGN_CENTER_VERTICAL);
    
    gridOptionsSizer->Add(gridValuesSizer, 0, wxLEFT | wxBOTTOM, 5);
    gridTypeSizer->Add(gridOptionsSizer, 0, wxALL, 5);
    mainSizer->Add(gridTypeSizer, 0, wxALL | wxEXPAND, 5);

    // Skip empties
    wxCheckBox* skipEmptiesCheck = new wxCheckBox(dialog, wxID_ANY, "Skip empties");
    skipEmptiesCheck->SetValue(m_grabberInfo.GetGriddleEmpties());
    mainSizer->Add(skipEmptiesCheck, 0, wxALL, 5);

    // Autocrop
    wxCheckBox* autocropCheck = new wxCheckBox(dialog, wxID_ANY, "Autocrop");
    autocropCheck->SetValue(m_grabberInfo.GetGriddleAutocrop());
    mainSizer->Add(autocropCheck, 0, wxALL, 5);

    // Name
    wxBoxSizer* nameSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* nameLabel = new wxStaticText(dialog, wxID_ANY, "Name:");
    wxTextCtrl* nameText = new wxTextCtrl(dialog, wxID_ANY, "",
                                         wxDefaultPosition, wxSize(200, -1));
    nameSizer->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    nameSizer->Add(nameText, 1, wxEXPAND);
    mainSizer->Add(nameSizer, 0, wxALL | wxEXPAND, 5);

    // Type selection
    wxStaticBox* typeBox = new wxStaticBox(dialog, wxID_ANY, "Type:");
    wxStaticBoxSizer* typeBoxSizer = new wxStaticBoxSizer(typeBox, wxVERTICAL);
    
    wxListBox* typeList = new wxListBox(typeBox, wxID_ANY, wxDefaultPosition, wxSize(200, 100));
    typeList->Append("Bitmap");
    typeList->Append("RLE Sprite");
    typeList->Append("Compiled Sprite");
    typeList->Append("Mode-X Compiled Sprite");
    typeList->SetSelection(m_grabberInfo.GetGriddleType());
    
    typeBoxSizer->Add(typeList, 0, wxALL | wxEXPAND, 5);
    mainSizer->Add(typeBoxSizer, 0, wxALL | wxEXPAND, 5);

    // Color mode selection
    wxStaticBox* colorBox = new wxStaticBox(dialog, wxID_ANY, "Colors:");
    wxStaticBoxSizer* colorBoxSizer = new wxStaticBoxSizer(colorBox, wxVERTICAL);
    
    wxListBox* colorList = new wxListBox(colorBox, wxID_ANY, wxDefaultPosition, wxSize(200, 100));
    colorList->Append("256 color palette");
    colorList->Append("15 bit hicolor");
    colorList->Append("16 bit hicolor");
    colorList->Append("24 bit truecolor");
    colorList->Append("32 bit truecolor");
    colorList->SetSelection(3);  // Default to 24 bit truecolor
    
    // Add autogenerate palette checkbox (initially hidden)
    wxCheckBox* autogeneratePaletteCheck = new wxCheckBox(colorBox, wxID_ANY, "Autogenerate palette");
    autogeneratePaletteCheck->SetValue(true);  // Enabled by default
    autogeneratePaletteCheck->Hide();  // Initially hidden
    autogeneratePaletteCheck->SetToolTip("Generate a new optimal palette for the image if the current one doesn't match well enough. This ensures better color quality for 256-color images.");
    
    colorBoxSizer->Add(colorList, 0, wxALL | wxEXPAND, 5);
    colorBoxSizer->Add(autogeneratePaletteCheck, 0, wxALL, 5);
    mainSizer->Add(colorBoxSizer, 0, wxALL | wxEXPAND, 5);

    // Bind color list selection event to show/hide autogenerate checkbox
    colorList->Bind(wxEVT_LISTBOX, [autogeneratePaletteCheck](wxCommandEvent& event) {
        autogeneratePaletteCheck->Show(event.GetSelection() == 0);  // Show only for 256 color mode
        autogeneratePaletteCheck->GetParent()->Layout();
    });

    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(dialog, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(dialog, wxID_CANCEL, "Cancel");
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    dialog->SetSizer(mainSizer);
    mainSizer->Fit(dialog);
    dialog->Center();

    // Set initial state
    regularGridRadio->SetValue(m_grabberInfo.GetGriddleMode());
    useColRadio->SetValue(!m_grabberInfo.GetGriddleMode());
    xGridText->Enable(m_grabberInfo.GetGriddleMode());
    yGridText->Enable(m_grabberInfo.GetGriddleMode());

    // Set initial color from settings
    uint32_t savedColor = m_grabberInfo.GetGriddleColor();
    wxColour initialColor(
        (savedColor >> 16) & 0xFF,  // Red
        (savedColor >> 8) & 0xFF,   // Green
        savedColor & 0xFF           // Blue
    );
    colorPicker->SetColour(initialColor);

    // Bind radio button events
    useColRadio->Bind(wxEVT_RADIOBUTTON, [xGridText, yGridText](wxCommandEvent&) {
        xGridText->Enable(false);
        yGridText->Enable(false);
    });
    
    regularGridRadio->Bind(wxEVT_RADIOBUTTON, [xGridText, yGridText](wxCommandEvent&) {
        xGridText->Enable(true);
        yGridText->Enable(true);
    });

    // Show dialog
    if (dialog->ShowModal() == wxID_OK) {
        // Get values from dialog controls
        bool useCol255 = useColRadio->GetValue();  // True if "Use col #255" is selected
        bool useRegularGrid = regularGridRadio->GetValue();
        bool skipEmpties = skipEmptiesCheck->GetValue();
        bool autoCrop = autocropCheck->GetValue();
        wxString name = nameText->GetValue();
        int typeIndex = typeList->GetSelection();
        int colorIndex = colorList->GetSelection();
        wxColour gridColor = colorPicker->GetColour();
        bool autogeneratePalette = autogeneratePaletteCheck->GetValue();

        // Save settings back to GrabberInfo
        long xGridSize = 0, yGridSize = 0;
        if (xGridText->GetValue().ToLong(&xGridSize) && yGridText->GetValue().ToLong(&yGridSize)) {
            m_grabberInfo.SetGriddleXGrid(xGridSize);
            m_grabberInfo.SetGriddleYGrid(yGridSize);
        }
        m_grabberInfo.SetGriddleMode(useRegularGrid);
        m_grabberInfo.SetGriddleEmpties(skipEmpties);
        m_grabberInfo.SetGriddleAutocrop(autoCrop);
        m_grabberInfo.SetGriddleType(typeIndex);

        // Save grid color
        uint32_t colorValue = ((uint32_t)gridColor.Red() << 16) |
                            ((uint32_t)gridColor.Green() << 8) |
                            (uint32_t)gridColor.Blue();
        m_grabberInfo.SetGriddleColor(colorValue);

        // Check if we have a loaded image
        if (!m_loadedImage || !m_loadedImage->IsOk()) {
            wxMessageBox("No bitmap has been loaded. Please use 'Read Bitmap' first.", 
                        "No Bitmap", wxOK | wxICON_ERROR);
            dialog->Destroy();
            return;
        }

        // Get grid dimensions (reuse xGridSize and yGridSize from above)
        if (useRegularGrid && (xGridSize <= 0 || yGridSize <= 0)) {
            wxMessageBox("Invalid grid dimensions", "Error", wxOK | wxICON_ERROR);
            dialog->Destroy();
            return;
        }

        // Get selected type and color mode
        if (typeIndex == wxNOT_FOUND || colorIndex == wxNOT_FOUND) {
            wxMessageBox("Please select both type and color mode", "Error", wxOK | wxICON_ERROR);
            dialog->Destroy();
            return;
        }

        int bits = 0;
        switch (colorIndex) {
            case 0: bits = 8; break;  // 8 bit
            case 1: bits = 15; break; // 15 bit hicolor
            case 2: bits = 16; break; // 16 bit hicolor
            case 3: bits = 24; break; // 24 bit truecolor
            case 4: bits = 32; break; // 32 bit truecolor
        }
        // Store palette object temporarily if using 256 colors
        std::optional<DataParser::DataObject> paletteObj;
        if (bits == 8) {
            // 256 color palette
            // check current palette match
            double currentPaletteMatch = BitmapData::calculatePaletteMatch(*m_loadedImage, m_currentPalette);
            logInfo("Current palette match: " + std::to_string(currentPaletteMatch));
            if (autogeneratePalette) {
                if (currentPaletteMatch < 0.998) {
                    logInfo("Generating optimal palette");
                    if (!BitmapData::generateOptimalPalette(*m_loadedImage, m_currentPalette)) {
                        logWarning("Failed to generate optimal palette, using default palette");
                        m_currentPalette = BitmapData::allegro_palette;
                    }
                    paletteObj = DataParser::DataObject();
                    if (!DataParser::createFromPalette(m_currentPalette, paletteObj.value())) {
                        logWarning("Failed to create palette object, skipping");
                    }
                    SetOrigPropertyWithFormat(std::make_shared<DataParser::DataObject>(paletteObj.value()), m_loadedBitmapPath.ToStdString());
                } else {
                    logInfo("Palette match is good enough, skipping palette generation");
                }
            }
        }
        
        // process alpha channel
        if (bits == 32 && (typeIndex == 0 || typeIndex == 1) && !m_currentAlphaChannel.empty()) {
            bits = -32; // set to truecolor with alpha
            logInfo("Alpha channel is available, using it");
            // set alpha channel to the loaded image
            m_loadedImage->SetAlpha();
            std::copy(m_currentAlphaChannel.begin(), m_currentAlphaChannel.end(), m_loadedImage->GetAlpha());

        }

        std::vector<BitmapData::GridCell> gridCells;
        if (useRegularGrid) {
            gridCells = BitmapData::gridBySize(*m_loadedImage, xGridSize, yGridSize);
        } else if (useCol255) {
            // If using color 255 mode, detect the bounding box
            gridCells = BitmapData::gridByColor(*m_loadedImage, gridColor);
        }

        // Create progress dialog
        wxProgressDialog progress("Processing Grid", 
                                "Processing grid cells...",
                                gridCells.size(),
                                dialog,
                                wxPD_AUTO_HIDE | wxPD_APP_MODAL | wxPD_ELAPSED_TIME);

        int objectCount = 0;
        for (const auto& cell : gridCells) {
            BitmapData tempBmp;
            switch (typeIndex) {
                case 0: tempBmp.typeID = ObjectType::DAT_BITMAP; break; // Bitmap
                case 1: tempBmp.typeID = ObjectType::DAT_RLE_SPRITE; break; // RLE Sprite
                case 2: tempBmp.typeID = ObjectType::DAT_C_SPRITE; break; // Compiled Sprite
                case 3: tempBmp.typeID = ObjectType::DAT_XC_SPRITE; break; // Mode-X Compiled
            }
            logDebug("Loading cell image at position " + std::to_string(cell.x) + "," + std::to_string(cell.y));
            if (!tempBmp.loadFromWxImage(cell.image, bits, &m_currentPalette, m_grabberInfo.GetDither(), m_grabberInfo.GetTransparency())) {
                logWarning("Failed to load cell image at position " + std::to_string(cell.x) + "," + std::to_string(cell.y));
                continue;
            }

            // Skip empty cells if requested
            if (skipEmpties) {
                if (tempBmp.isMonocolor()) {
                    continue;
                }
            }

            // Auto-crop if requested
            int minX = 0, minY = 0;
            bool wasCropped = false;
            if (autoCrop) {
                // Create a temporary BitmapData object for cropping
                if (tempBmp.autoCrop(minX, minY)) {
                    wasCropped = true;
                }
            }

            // Create DataObject
            DataParser::DataObject obj;
            obj.typeID = tempBmp.typeID;

            // Set object properties
            wxString objName;
            if (name.IsEmpty()) {
                objName = wxString::Format("%03d", objectCount);
            } else {
                objName = wxString::Format("%s%03d", name, objectCount);
            }
            obj.setProperty('NAME', objName.ToStdString());
            
            // Set ORIG property to the loaded bitmap path
            SetOrigPropertyWithFormat(std::make_shared<DataParser::DataObject>(obj), m_loadedBitmapPath.ToStdString());
            
            // Set additional properties
            obj.updateDateProperty();
            obj.setProperty('XPOS', std::to_string(cell.x));
            obj.setProperty('YPOS', std::to_string(cell.y));
            obj.setProperty('XSIZ', std::to_string(cell.width));
            obj.setProperty('YSIZ', std::to_string(cell.height));

            // If the image was cropped, add crop offset properties
            if (wasCropped) {
                obj.setProperty('XCRP', std::to_string(minX));
                obj.setProperty('YCRP', std::to_string(minY));
            }
            
            obj.data = tempBmp;
            // Add object to list
            m_objects.push_back(std::make_shared<DataParser::DataObject>(std::move(obj)));
            objectCount++;

            // Update progress
            progress.Update(objectCount);
        }

        // Add palette object as the last object if using 256 colors
        if (bits == 8 && paletteObj.has_value()) {
            m_objects.push_back(std::make_shared<DataParser::DataObject>(std::move(paletteObj.value())));
        }

        // Update the tree display
        RefreshTreeDisplay();
        SetModified(true);

        // Update preview controls if an object is selected
        if (m_currentObject) {
            UpdateObjectPreview();
        }
    }

    dialog->Destroy();
}

void MyFrame::OnTreeItemActivated(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk() || item == m_tree->GetRootItem()) {
        return;
    }

    // If the item has children, toggle its expanded/collapsed state
    if (m_tree->ItemHasChildren(item)) {
        if (m_tree->IsExpanded(item)) {
            m_tree->Collapse(item);
        } else {
            m_tree->Expand(item);
        }
    }

    // Get the object pointer from tree item data
    ObjectTreeData* data = static_cast<ObjectTreeData*>(m_tree->GetItemData(item));
    if (!data || !data->object) {
        return;
    }

    // If this is a font object, open the font editor dialog
    if (data->object->isFont()) {
        // Get a non-const reference to the font data for editing
        DataParser::DataObject& obj = const_cast<DataParser::DataObject&>(*data->object);
        FontData& fontData = const_cast<FontData&>(obj.getFont());
        
        FontEditDialog dialog(this, fontData);
        if (dialog.ShowModal() == wxID_OK) {
            // Font data was modified in the dialog
            SetModified(true);
            
            // Update the preview if this is the currently selected object
            if (m_currentObject == data->object) {
                UpdateObjectPreview();
            }
            
            SetStatusText("Font edited: " + wxString::FromUTF8(data->object->name));
            logInfo("Font edited: " + data->object->name);
        }
        return;
    }

    // If this is a palette object, set it as the current palette
    if (data->object->isBitmap() && data->object->getBitmap().isPalette()) {
        const BitmapData& bmpData = data->object->getBitmap();
        m_currentPalette = bmpData.data;
        SetStatusText("Palette set from " + wxString::FromUTF8(data->object->name));
        UpdatePreviewControls(data->object);  // Keep this separate since we're not updating the property list
    }
}

void MyFrame::OnReadAlpha(wxCommandEvent& event) {
    // Create file dialog to select image file
    wxFileDialog openFileDialog(this, "Open Image File", "", "",
                               "Image files (*.bmp;*.png;*.jpg;*.jpeg;*.tga;*.pcx)|*.bmp;*.png;*.jpg;*.jpeg;*.tga;*.pcx",
                               wxFD_OPEN|wxFD_FILE_MUST_EXIST);

    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxString path = openFileDialog.GetPath();
    
    // Load the image
    wxImage image;
    if (!BitmapData::readFileToWxImage(path, image)) {
        wxMessageBox("Failed to load image file: " + path, "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Extract alpha channel using the new method
    if (!BitmapData::extractAlphaChannel(image, m_currentAlphaChannel)) {
        wxMessageBox("Failed to extract alpha channel from image: " + path, "Error", wxOK | wxICON_ERROR);
        return;
    }

    // Show success message
    wxMessageBox(wxString::Format("Successfully loaded alpha channel (%dx%d)", image.GetWidth(), image.GetHeight()),
                "Alpha Channel Loaded", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnExport(wxCommandEvent& event) {
    // Check if there's a selected object
    if (!ValidateObjectSelection("export")) {
        return;
    }

    // Determine file type and extension based on the object type
    wxString defaultExt;
    wxString filter;
    
    if (m_currentObject->isBitmap()) {
        defaultExt = "png";
        filter = "PNG files (*.png)|*.png|BMP files (*.bmp)|*.bmp|JPEG files (*.jpg)|*.jpg|PCX files (*.pcx)|*.pcx|All files (*.*)|*.*";
    }
    else if (m_currentObject->isFont()) {
        defaultExt = "bmp";
        filter = "BMP images (*.bmp)|*.bmp|PNG images (*.png)|*.png|TGA images (*.tga)|*.tga|PCX images (*.pcx)|*.pcx|Font files (*.fnt)|*.fnt|All files (*.*)|*.*";
    }
    else if (m_currentObject->isAudio()) {
        // Determine extension and filter based on audio object type
        if (m_currentObject->typeID == ObjectType::DAT_OGG) {
            defaultExt = "ogg";
            filter = "OGG files (*.ogg)|*.ogg|All files (*.*)|*.*";
        }
        else if (m_currentObject->typeID == ObjectType::DAT_SAMP) {
            defaultExt = "wav";
            filter = "WAV files (*.wav)|*.wav|All files (*.*)|*.*";
        }
        else if (m_currentObject->typeID == ObjectType::DAT_MIDI) {
            defaultExt = "mid";
            filter = "MIDI files (*.mid)|*.mid|All files (*.*)|*.*";
        }
        else {
            // Default to OGG for unknown audio types
            defaultExt = "ogg";
            filter = "OGG files (*.ogg)|*.ogg|All files (*.*)|*.*";
        }
    }
    else if (m_currentObject->isVideo()) {
        defaultExt = "fli";
        filter = "FLI files (*.fli)|*.fli|FLC files (*.flc)|*.flc|All files (*.*)|*.*";
    }
    else {
        defaultExt = "";  // No default extension for raw binary data
        filter = "All files (*.*)|*.*";
    }

    // Get default path and filename for export
    wxString defaultPath = "";
    wxString defaultName;
    
    // Try to use ORIG property if it exists
    std::string origPath = m_currentObject->getProperty('ORIG');
    if (!origPath.empty()) {
        // Extract directory and filename from the path
        wxFileName fileName(wxString::FromUTF8(origPath));
        defaultPath = fileName.GetPath();
        defaultName = fileName.GetFullName();
        
        // Also use the extension from the ORIG property if available
        wxString origExt = fileName.GetExt().Lower();
        if (!origExt.IsEmpty()) {
            // Check if the extension is valid for the current object type
            if (m_currentObject->isBitmap() && 
                (origExt == "png" || origExt == "bmp" || origExt == "jpg" || 
                 origExt == "jpeg" || origExt == "pcx")) {
                defaultExt = origExt;
            }
            else if (m_currentObject->isFont() && 
                (origExt == "bmp" || origExt == "fnt" || origExt == "png" || 
                 origExt == "tga" || origExt == "pcx")) {
                defaultExt = origExt;
            }
            else if (m_currentObject->isAudio() && 
                (origExt == "ogg" || origExt == "wav" || origExt == "mid")) {
                defaultExt = origExt;
            }
            else if (m_currentObject->isVideo() && 
                (origExt == "fli" || origExt == "flc")) {
                defaultExt = origExt;
            }
        }
    }
    
    // If no ORIG property or it's empty, use object name
    if (defaultName.IsEmpty()) {
        defaultName = wxString::FromUTF8(m_currentObject->name);
        // If the name is still empty, use "export"
        if (defaultName.IsEmpty()) {
            defaultName = "export";
        }
    }
    
    // Make sure we have the correct extension, replace it if the filename already has one
    wxFileName fileName(defaultName);
    if (!defaultExt.IsEmpty()) {
        fileName.SetExt(defaultExt);
    }
    // For raw binary data (empty defaultExt), don't modify the filename
    defaultName = fileName.GetFullName();

    // Determine the correct filter index based on the extension
    int filterIndex = 0;
    if (m_currentObject->isBitmap()) {
        if (defaultExt == "png") filterIndex = 0;
        else if (defaultExt == "bmp") filterIndex = 1;
        else if (defaultExt == "jpg" || defaultExt == "jpeg") filterIndex = 2;
        else if (defaultExt == "pcx") filterIndex = 3;
        else filterIndex = 0;
    } else if (m_currentObject->isFont()) {
        if (defaultExt == "bmp") filterIndex = 0;
        else if (defaultExt == "png") filterIndex = 1;
        else if (defaultExt == "tga") filterIndex = 2;
        else if (defaultExt == "pcx") filterIndex = 3;
        else if (defaultExt == "fnt") filterIndex = 4;
        else filterIndex = 0;
    } else if (m_currentObject->isAudio()) {
        if (defaultExt == "ogg") filterIndex = 0;
        else if (defaultExt == "wav") filterIndex = 0;
        else if (defaultExt == "mid") filterIndex = 0;
        else filterIndex = 1;
    } else if (m_currentObject->isVideo()) {
        if (defaultExt == "fli") filterIndex = 0;
        else if (defaultExt == "flc") filterIndex = 1;
        else filterIndex = 0;
    } else {
        // For raw binary data (no default extension), always use index 0 (All files)
        filterIndex = 0;
    }

    // Create file dialog with filter index
    wxFileDialog saveFileDialog(this, "Export Object", defaultPath, defaultName,
                               filter, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    saveFileDialog.SetFilterIndex(filterIndex);

    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;

    wxString path = saveFileDialog.GetPath();
    
    bool success = false;
    
    // Export based on object type
    if (m_currentObject->isBitmap()) {
        const BitmapData& bmpData = m_currentObject->getBitmap();
        wxImage image;
        
        if (bmpData.toWxImage(image, m_currentPalette)) {
            // Determine file format based on extension
            wxString ext = path.AfterLast('.').Lower();
            
            if (ext == "png") {
                success = image.SaveFile(path, wxBITMAP_TYPE_PNG);
            } 
            else if (ext == "bmp") {
                success = image.SaveFile(path, wxBITMAP_TYPE_BMP);
            }
            else if (ext == "jpg" || ext == "jpeg") {
                success = image.SaveFile(path, wxBITMAP_TYPE_JPEG);
            }
            else if (ext == "pcx") {
                // PCX export is not natively supported by wxWidgets, implement custom PCX export
                success = BitmapData::WritePCXFile(path, image);
            }
            else {
                // Default to PNG for other extensions
                success = image.SaveFile(path, wxBITMAP_TYPE_PNG);
            }
        }
    }
    else if (m_currentObject->isFont()) {
        // Handle font export - export all ranges
        const FontData& fontData = m_currentObject->getFont();
        wxString ext = path.AfterLast('.').Lower();
        
        if (fontData.ranges.empty()) {
            logError("Font has no ranges to export");
            success = false;
        }
        else if (ext == "fnt") {
            // Export all ranges as .fnt files
            if (fontData.ranges.size() == 1) {
                // Single range - export directly to the specified path
                success = FontData::ExportRangeAsFnt(fontData.ranges[0], path);
            } else {
                // Multiple ranges - export each to a separate file
                wxFileName baseName(path);
                wxString nameWithoutExt = baseName.GetName();
                wxString dir = baseName.GetPath();
                bool allSuccess = true;
                
                for (size_t i = 0; i < fontData.ranges.size(); ++i) {
                    const auto& range = fontData.ranges[i];
                    wxString rangeFileName = wxString::Format("%s_range%zu_U%04X-U%04X.fnt", 
                                                             nameWithoutExt, i + 1, range.start, range.end);
                    wxString fullPath = wxFileName(dir, rangeFileName).GetFullPath();
                    
                    if (!FontData::ExportRangeAsFnt(range, fullPath)) {
                        allSuccess = false;
                        logError("Failed to export range " + std::to_string(i + 1) + " to " + fullPath.ToStdString());
                    } else {
                        logInfo("Exported range " + std::to_string(i + 1) + " to " + fullPath.ToStdString());
                    }
                }
                success = allSuccess;
            }
        }
        else if (ext == "bmp" || ext == "png" || ext == "tga" || ext == "pcx") {
            // Export font using smart export (single file for single range, combined file + script for multiple ranges)
            success = fontData.ExportFontAsBitmap(path);
        }
        else {
            // Default to .fnt format for unknown extensions
            if (fontData.ranges.size() == 1) {
                // Single range - export directly
                success = FontData::ExportRangeAsFnt(fontData.ranges[0], path);
            } else {
                // Multiple ranges - export each to a separate .fnt file
                wxFileName baseName(path);
                wxString nameWithoutExt = baseName.GetName();
                wxString dir = baseName.GetPath();
                bool allSuccess = true;
                
                for (size_t i = 0; i < fontData.ranges.size(); ++i) {
                    const auto& range = fontData.ranges[i];
                    wxString rangeFileName = wxString::Format("%s_range%zu_U%04X-U%04X.fnt", 
                                                             nameWithoutExt, i + 1, range.start, range.end);
                    wxString fullPath = wxFileName(dir, rangeFileName).GetFullPath();
                    
                    if (!FontData::ExportRangeAsFnt(range, fullPath)) {
                        allSuccess = false;
                        logError("Failed to export range " + std::to_string(i + 1) + " to " + fullPath.ToStdString());
                    } else {
                        logInfo("Exported range " + std::to_string(i + 1) + " to " + fullPath.ToStdString());
                    }
                }
                success = allSuccess;
            }
        }
    }
    else if (m_currentObject->isAudio()) {
        // Handle audio export based on object type
        const AudioData& audioData = m_currentObject->getAudio();
        
        if (m_currentObject->typeID == ObjectType::DAT_SAMP) {
            // Export SAMP as WAV file using getWavData()
            std::vector<uint8_t> wavData = audioData.getWavData();
            if (!wavData.empty()) {
                std::ofstream outFile(path.ToStdString(), std::ios::binary);
                if (outFile) {
                    outFile.write(reinterpret_cast<const char*>(wavData.data()), wavData.size());
                    success = outFile.good();
                    outFile.close();
                }
            }
        }
        else if (m_currentObject->typeID == ObjectType::DAT_OGG) {
            // Export OGG as raw OGG data
            const std::vector<uint8_t>& data = audioData.data;
            std::ofstream outFile(path.ToStdString(), std::ios::binary);
            if (outFile) {
                outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
                success = outFile.good();
                outFile.close();
            }
        }
        else if (m_currentObject->typeID == ObjectType::DAT_MIDI) {
            // Export MIDI as raw MIDI data
            const std::vector<uint8_t>& data = audioData.getMidiData();
            std::ofstream outFile(path.ToStdString(), std::ios::binary);
            if (outFile) {
                outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
                success = outFile.good();
                outFile.close();
            }
        }
        else {
            // Default: export as raw binary data
            const std::vector<uint8_t>& data = audioData.data;
            std::ofstream outFile(path.ToStdString(), std::ios::binary);
            if (outFile) {
                outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
                success = outFile.good();
                outFile.close();
            }
        }
    }
    else if (m_currentObject->isVideo()) {
        // Handle video export
        const VideoData& videoData = m_currentObject->getVideo();
        const std::vector<uint8_t>& data = videoData.data;
        
        // Write binary data to file (FLI/FLC format)
        std::ofstream outFile(path.ToStdString(), std::ios::binary);
        if (outFile) {
            outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
            success = outFile.good();
            outFile.close();
        }
    }
    else if (m_currentObject->isRawData()) {
        const std::vector<uint8_t>& data = m_currentObject->getRawData();
        
        // Write binary data to file
        std::ofstream outFile(path.ToStdString(), std::ios::binary);
        if (outFile) {
            outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
            success = outFile.good();
            outFile.close();
        }
    }
    
    if (success) {
        SetStatusText("Successfully exported: " + path);
        logInfo("Exported object " + m_currentObject->name + " to " + path.ToStdString());
    } else {
        wxMessageBox("Failed to export to file: " + path, "Export Error", wxOK | wxICON_ERROR);
        logError("Failed to export object " + m_currentObject->name + " to " + path.ToStdString());
    }
}

void MyFrame::OnMoveUp(wxCommandEvent& event) {
    if (!ValidateObjectSelection("move up")) {
        return;
    }

    // Store the current object's UI ID before moving
    uint32_t currentUID = m_currentObject->ui_id;

    // Use utility function to find and move object
    bool moved = ObjectTraversalUtils::ForEachObjectRecursive(m_objects, [this](std::shared_ptr<DataParser::DataObject>& obj) -> bool {
        if (obj == m_currentObject) {
            // Find parent vector and move object up
            auto* parentVector = ObjectTraversalUtils::FindParentVector(m_objects, obj);
            if (parentVector) {
                return ObjectTraversalUtils::MoveObjectUp(*parentVector, obj);
            }
        }
        return false;
    });

    if (!moved) {
        // Already at the top, silently do nothing
        return;
    }

    // Mark as modified
    SetModified(true);

    // Refresh the tree display to reflect the changes
    RefreshTreeDisplay();

    // Find and select the moved object in the tree by UI ID
    wxTreeItemId rootId = m_tree->GetRootItem();
    
    // Helper function to recursively search tree items
    std::function<wxTreeItemId(wxTreeItemId)> findObjectInTree;
    findObjectInTree = [&](wxTreeItemId parent) -> wxTreeItemId {
        wxTreeItemIdValue cookie;
        wxTreeItemId item = m_tree->GetFirstChild(parent, cookie);
        while (item.IsOk()) {
            ObjectTreeData* data = static_cast<ObjectTreeData*>(m_tree->GetItemData(item));
            if (data && data->object && data->object->ui_id == currentUID) {
                return item;
            }
            
            // Check children recursively
            if (m_tree->ItemHasChildren(item)) {
                wxTreeItemId found = findObjectInTree(item);
                if (found.IsOk()) {
                    return found;
                }
            }
            
            item = m_tree->GetNextSibling(item);
        }
        return wxTreeItemId();
    };

    wxTreeItemId foundItem = findObjectInTree(rootId);
    if (foundItem.IsOk()) {
        ObjectTreeData* foundData = static_cast<ObjectTreeData*>(m_tree->GetItemData(foundItem));
        if (foundData && foundData->object) {
            m_tree->SelectItem(foundItem);
            m_currentObject = foundData->object;
        } else {
            // Moved object found in tree but has invalid data pointer.
        }
    } else {
        // Moved object not found in tree after move operation.
        m_currentObject = nullptr;
    }

    // Update status
    SetStatusText("Object moved successfully");
}

void MyFrame::OnMoveDown(wxCommandEvent& event) {
    if (!ValidateObjectSelection("move down")) {
        return;
    }

    // Store the current object's UI ID before moving
    uint32_t currentUID = m_currentObject->ui_id;

    // Use utility function to find and move object
    bool moved = ObjectTraversalUtils::ForEachObjectRecursive(m_objects, [this](std::shared_ptr<DataParser::DataObject>& obj) -> bool {
        if (obj == m_currentObject) {
            // Find parent vector and move object down
            auto* parentVector = ObjectTraversalUtils::FindParentVector(m_objects, obj);
            if (parentVector) {
                return ObjectTraversalUtils::MoveObjectDown(*parentVector, obj);
            }
        }
        return false;
    });

    if (!moved) {
        // Already at the bottom, silently do nothing
        return;
    }

    // Mark as modified
    SetModified(true);

    // Refresh the tree display to reflect the changes
    RefreshTreeDisplay();

    // Find and select the moved object in the tree by UI ID
    wxTreeItemId rootId = m_tree->GetRootItem();
    
    // Helper function to recursively search tree items
    std::function<wxTreeItemId(wxTreeItemId)> findObjectInTree;
    findObjectInTree = [&](wxTreeItemId parent) -> wxTreeItemId {
        wxTreeItemIdValue cookie;
        wxTreeItemId item = m_tree->GetFirstChild(parent, cookie);
        while (item.IsOk()) {
            ObjectTreeData* data = static_cast<ObjectTreeData*>(m_tree->GetItemData(item));
            if (data && data->object && data->object->ui_id == currentUID) {
                return item;
            }
            
            // Check children recursively
            if (m_tree->ItemHasChildren(item)) {
                wxTreeItemId found = findObjectInTree(item);
                if (found.IsOk()) {
                    return found;
                }
            }
            
            item = m_tree->GetNextSibling(item);
        }
        return wxTreeItemId();
    };

    wxTreeItemId foundItem = findObjectInTree(rootId);
    if (foundItem.IsOk()) {
        ObjectTreeData* foundData = static_cast<ObjectTreeData*>(m_tree->GetItemData(foundItem));
        if (foundData && foundData->object) {
            m_tree->SelectItem(foundItem);
            m_currentObject = foundData->object;
        } else {
            // Moved object found in tree but has invalid data pointer.
        }
    } else {
        // Moved object not found in tree after move operation.
        m_currentObject = nullptr;
    }

    // Update status
    SetStatusText("Object moved successfully");
}

// Helper to add a new object to the root or to the selected datafile's nested objects
void MyFrame::addObjectToCurrentOrRoot(std::shared_ptr<DataParser::DataObject> obj) {
    if (m_currentObject && m_currentObject->isNested()) {
        auto& nested = m_currentObject->getNestedObjects();
        nested.push_back(obj);
    } else {
        m_objects.push_back(obj);
    }
}

void MyFrame::OnNewBitmap(wxCommandEvent& event)
{
    DataParser::DataObject obj;
    obj.typeID = ObjectType::DAT_BITMAP;
    obj.data = BitmapData::createSampleBitmap(ObjectType::DAT_BITMAP);
    obj.setProperty('NAME', "Bitmap");
    obj.updateDateProperty();
    addObjectToCurrentOrRoot(std::make_shared<DataParser::DataObject>(std::move(obj)));
    RefreshTreeDisplay();
    SetModified(true);
}

void MyFrame::OnNewRLE(wxCommandEvent& event)
{
    addObjectToCurrentOrRoot(std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_RLE_SPRITE)));
    RefreshTreeDisplay();
    SetModified(true);
}

void MyFrame::OnNewCompiledSprite(wxCommandEvent& event)
{
    addObjectToCurrentOrRoot(std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_C_SPRITE)));
    RefreshTreeDisplay();
    SetModified(true);
}

void MyFrame::OnNewXSprite(wxCommandEvent& event)
{
    addObjectToCurrentOrRoot(std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_XC_SPRITE)));
    RefreshTreeDisplay();
    SetModified(true);
}

void MyFrame::OnNewDatafile(wxCommandEvent& event)
{
    addObjectToCurrentOrRoot(std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_FILE)));
    RefreshTreeDisplay();
    SetModified(true);
}

void MyFrame::OnNewFLI(wxCommandEvent& event)
{
    addObjectToCurrentOrRoot(std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_FLI)));
    RefreshTreeDisplay();
    SetModified(true);
}

void MyFrame::OnNewSample(wxCommandEvent& event)
{
    addObjectToCurrentOrRoot(std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_SAMP)));
    RefreshTreeDisplay();
    SetModified(true);
}

void MyFrame::OnNewPalette(wxCommandEvent& event)
{
    DataParser::DataObject paletteObj;
    if (DataParser::createFromPalette(m_currentPalette, paletteObj)) {
        addObjectToCurrentOrRoot(std::make_shared<DataParser::DataObject>(std::move(paletteObj)));
        RefreshTreeDisplay();
        SetModified(true);
    } else {
        wxMessageBox("Failed to create palette object.", "Error", wxOK | wxICON_ERROR);
    }
}

// Handler for Object->New->Other - shows dialog for custom object creation
void MyFrame::OnNewOther(wxCommandEvent& event)
{
    // Use the reusable custom object dialog
    auto result = ShowCustomObjectDialog("DATA", "", "New Object");
    wxString typeStr = result.first;
    wxString name = result.second;
    
    if (typeStr.IsEmpty() || name.IsEmpty()) {
        return; // User cancelled or input was empty
    }
    
    // Convert type string to uint32_t (big endian)
    uint32_t typeID = ((unsigned char)typeStr[0] << 24) |
                      ((unsigned char)typeStr[1] << 16) |
                      ((unsigned char)typeStr[2] << 8)  |
                      ((unsigned char)typeStr[3]);
    
    // Use createSampleObject which will default unknown types to binary data
    DataParser::DataObject obj = DataParser::createSampleObject(static_cast<ObjectType>(typeID));
    obj.setProperty('NAME', name.ToStdString());
    obj.updateDateProperty();
    addObjectToCurrentOrRoot(std::make_shared<DataParser::DataObject>(std::move(obj)));
    RefreshTreeDisplay();
    SetModified(true);
}

void MyFrame::AVPositionUpdateCallback(int currentPositionMs, int totalDurationMs) {
    int sliderValue = (totalDurationMs > 0) ? currentPositionMs * 100 / totalDurationMs : 0;
    m_AVSlider->SetValue(sliderValue);
    m_timeLabel->SetLabel(FormatTime(currentPositionMs) + " / " + FormatTime(totalDurationMs));
    if (!m_currentObject) {
        //do nothing
    }
    else if (m_currentObject->isVideo()) {
        UpdatePlayButtonIcon(m_videoPanel->GetIsPlaying());
    } else if (m_currentObject->isAudio()) {
        UpdatePlayButtonIcon(m_audioControl->GetIsPlaying());
    }
}

void MyFrame::OnNewFont(wxCommandEvent& event) {
    // Create a new font object using the improved createSampleObject function
    addObjectToCurrentOrRoot(std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_FONT)));
    RefreshTreeDisplay();
    SetModified(true);
}

void MyFrame::OnNewMIDI(wxCommandEvent& event) {
    addObjectToCurrentOrRoot(std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_MIDI)));
    RefreshTreeDisplay();
    SetModified(true);
}

void MyFrame::OnTreeBeginDrag(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk() || item == m_tree->GetRootItem()) {
        // Don't allow dragging the root item
        event.Veto();
        return;
    }

    // Get the object pointer from tree item data
    ObjectTreeData* data = static_cast<ObjectTreeData*>(m_tree->GetItemData(item));
    if (!data || !data->object) {
        event.Veto();
        return;
    }

    // Store the dragged item for later use
    m_draggedItem = item;
    m_draggedObject = data->object;

    // Allow the drag operation
    event.Allow();
}

void MyFrame::OnTreeEndDrag(wxTreeEvent& event) {
    wxTreeItemId sourceItem = m_draggedItem;
    wxTreeItemId targetItem = event.GetItem();

    // Reset dragged item
    m_draggedItem = wxTreeItemId();
    m_draggedObject = nullptr;

    if (!sourceItem.IsOk() || !targetItem.IsOk() || sourceItem == targetItem) {
        return;
    }

    // Get the source object data
    ObjectTreeData* sourceData = static_cast<ObjectTreeData*>(m_tree->GetItemData(sourceItem));
    if (!sourceData || !sourceData->object) {
        return;
    }

    // Store the UID before modifying the tree, to avoid use-after-free
    uint32_t movedUID = sourceData->object->ui_id;
    
    std::shared_ptr<DataParser::DataObject> targetObj = nullptr;
    if (targetItem != m_tree->GetRootItem()) {
        ObjectTreeData* targetData = static_cast<ObjectTreeData*>(m_tree->GetItemData(targetItem));
        if (!targetData || !targetData->object) {
            return;
        }

        // Don't allow dropping an object into itself or its own subtree
        if (IsItemInSubtree(targetItem, sourceItem)) {
            wxMessageBox("Cannot drop an object into itself or its own subtree.", "Invalid Drop", wxOK | wxICON_WARNING);
            return;
        }
        targetObj = targetData->object;
    }

    // Perform the move operation
    if (MoveObjectInTree(sourceData->object, targetObj)) {
        // Mark as modified
        SetModified(true);

        // Clear current object pointer before refreshing tree
        m_currentObject = nullptr;

        // Refresh the tree display
        RefreshTreeDisplay();

        // Find and select the moved object using the stored UID
        wxTreeItemId rootId = m_tree->GetRootItem();
        
        // Helper function to recursively search tree items
        std::function<wxTreeItemId(wxTreeItemId)> findObjectInTree;
        findObjectInTree = [&](wxTreeItemId parent) -> wxTreeItemId {
            wxTreeItemIdValue cookie;
            wxTreeItemId item = m_tree->GetFirstChild(parent, cookie);
            while (item.IsOk()) {
                ObjectTreeData* data = static_cast<ObjectTreeData*>(m_tree->GetItemData(item));
                if (data && data->object && data->object->ui_id == movedUID) {
                    return item;
                }
                
                // Check children recursively
                if (m_tree->ItemHasChildren(item)) {
                    wxTreeItemId found = findObjectInTree(item);
                    if (found.IsOk()) {
                        return found;
                    }
                }
                
                item = m_tree->GetNextSibling(item);
            }
            return wxTreeItemId();
        };

        wxTreeItemId foundItem = findObjectInTree(rootId);
        if (foundItem.IsOk()) {
            ObjectTreeData* foundData = static_cast<ObjectTreeData*>(m_tree->GetItemData(foundItem));
            if (foundData && foundData->object) {
                m_tree->SelectItem(foundItem);
                m_currentObject = foundData->object;
            } else {
                // Moved object found in tree but has invalid data pointer.
            }
        } else {
            // Moved object not found in tree after move operation.
            m_currentObject = nullptr;
        }

        SetStatusText("Object moved successfully");
    } else {
        wxMessageBox("Failed to move object.", "Move Error", wxOK | wxICON_ERROR);
    }
}

bool MyFrame::IsItemInSubtree(wxTreeItemId targetItem, wxTreeItemId sourceItem) {
    // Check if targetItem is a descendant of sourceItem
    wxTreeItemId current = targetItem;
    while (current.IsOk()) {
        if (current == sourceItem) {
            return true;
        }
        current = m_tree->GetItemParent(current);
    }
    return false;
}

bool MyFrame::MoveObjectInTree(std::shared_ptr<DataParser::DataObject> sourceObj, std::shared_ptr<DataParser::DataObject> targetObj) {
    // Remove the source object from its current location and get its parent vector
    auto* parentVector = ObjectTraversalUtils::FindParentVector(m_objects, sourceObj);
    if (!parentVector) {
        // Failed to remove source object from its parent.
        return false;
    }

    // Remove the object from its current location
    if (!ObjectTraversalUtils::FindAndRemoveObject(*parentVector, sourceObj)) {
        return false;
    }

    // Add the source object to the target location
    if (!targetObj) {
        // If target is null, move to the end of the root list
        m_objects.push_back(sourceObj);
    } else if (targetObj->isNested()) {
        // If target is a datafile, add to its nested objects
        auto& nested = targetObj->getNestedObjects();
        nested.push_back(sourceObj);
    } else {
        // If target is not a datafile, insert as a sibling after the target
        if (!ObjectTraversalUtils::InsertAfterTarget(m_objects, sourceObj, targetObj)) {
            m_objects.push_back(sourceObj);
        }
    }

    return true;
}

void MyFrame::OnKeyDown(wxKeyEvent& event) {
    // Handle key events
    if (event.GetKeyCode() == WXK_DELETE) {
        // Trigger the delete action using ProcessEvent with wxEVT_MENU
        wxCommandEvent deleteEvent(wxEVT_MENU, ID_DELETE);
        ProcessEvent(deleteEvent);
        event.Skip(false); // Don't propagate the event
    } else {
        event.Skip(); // Let other handlers process the event
    }
}

// Reusable dialog for MoveTo operations
std::pair<wxString, wxString> MyFrame::ShowMoveToDialog(const ObjectType objectType, const wxString& currentName, bool typeEditable) {
    if (typeEditable) {
        // Use the reusable custom object dialog for editable type
        wxString initialType = DataParser::ConvertIDToString(static_cast<uint32_t>(objectType));
        return ShowCustomObjectDialog(initialType, currentName, "Move Object");
    } else {
        // Use the simple dialog for read-only type
        // Create a dialog to get the new name
        wxDialog dialog(this, wxID_ANY, "New Object", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        // Grid sizer for labels and fields
        wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 2, 8, 8);
        gridSizer->AddGrowableCol(1, 1);

        // Type label and value (read-only)
        wxStaticText* typeLabel = new wxStaticText(&dialog, wxID_ANY, "Type:");
        wxString typeString = DataParser::ConvertIDToString(static_cast<uint32_t>(objectType));
        wxStaticText* typeValue = new wxStaticText(&dialog, wxID_ANY, typeString);
        gridSizer->Add(typeLabel, 0, wxALIGN_LEFT | wxALL, 5);
        gridSizer->Add(typeValue, 1, wxEXPAND | wxALL, 5);

        // Name label and text field
        wxStaticText* nameLabel = new wxStaticText(&dialog, wxID_ANY, "Name:");
        wxTextCtrl* nameText = new wxTextCtrl(&dialog, wxID_ANY, currentName, wxDefaultPosition, wxSize(180, -1));
        gridSizer->Add(nameLabel, 0, wxALIGN_LEFT | wxALL, 5);
        gridSizer->Add(nameText, 1, wxEXPAND | wxALL, 5);

        mainSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);

        // Button sizer
        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        wxButton* okButton = new wxButton(&dialog, wxID_OK, "OK");
        wxButton* cancelButton = new wxButton(&dialog, wxID_CANCEL, "Cancel");
        buttonSizer->Add(okButton, 0, wxALL, 5);
        buttonSizer->Add(cancelButton, 0, wxALL, 5);
        mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);

        dialog.SetSizer(mainSizer);
        mainSizer->Fit(&dialog);
        dialog.CentreOnParent();

        nameText->SetFocus();

        if (dialog.ShowModal() == wxID_OK) {
            wxString newName = nameText->GetValue();
            if (newName.IsEmpty()) {
                wxMessageBox("Name cannot be empty.", "Error", wxOK | wxICON_ERROR);
                return std::make_pair(wxEmptyString, wxEmptyString);
            }
            return std::make_pair(typeString, newName);
        }
        
        return std::make_pair(wxEmptyString, wxEmptyString);
    }
}

// Reusable dialog for custom object creation/editing
std::pair<wxString, wxString> MyFrame::ShowCustomObjectDialog(const wxString& initialType, const wxString& initialName, const wxString& dialogTitle) {
    // Create dialog
    wxDialog dialog(this, wxID_ANY, dialogTitle, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Grid sizer for labels and fields
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 2, 8, 8);
    gridSizer->AddGrowableCol(1, 1);

    // Type label and text field
    wxStaticText* typeLabel = new wxStaticText(&dialog, wxID_ANY, "Type:");
    wxTextCtrl* typeText = new wxTextCtrl(&dialog, wxID_ANY, initialType, wxDefaultPosition, wxSize(60, -1));
    gridSizer->Add(typeLabel, 0, wxALIGN_LEFT | wxALL, 5);
    gridSizer->Add(typeText, 1, wxEXPAND | wxALL, 5);

    // Name label and text field
    wxStaticText* nameLabel = new wxStaticText(&dialog, wxID_ANY, "Name:");
    wxTextCtrl* nameText = new wxTextCtrl(&dialog, wxID_ANY, initialName, wxDefaultPosition, wxSize(180, -1));
    gridSizer->Add(nameLabel, 0, wxALIGN_LEFT | wxALL, 5);
    gridSizer->Add(nameText, 1, wxEXPAND | wxALL, 5);

    mainSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);

    // Button sizer
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(&dialog, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(&dialog, wxID_CANCEL, "Cancel");
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);

    dialog.SetSizer(mainSizer);
    mainSizer->Fit(&dialog);
    dialog.CentreOnParent();

    // Focus name field
    nameText->SetFocus();

    // Real-time validation for type field
    typeText->Bind(wxEVT_TEXT, [typeText, okButton](wxCommandEvent& evt) {
        wxString value = typeText->GetValue();
        wxString filtered;
        bool valid = true;
        for (int i = 0; i < value.Length(); ++i) {
            wxChar c = value[i];
            if (c >= 32 && c <= 126) {
                if (filtered.Length() < 4)
                    filtered += c;
            } else {
                valid = false;
            }
        }
        if (filtered != value) {
            int pos = typeText->GetInsertionPoint();
            typeText->ChangeValue(filtered); // ChangeValue does not send another event
            typeText->SetInsertionPoint(pos > filtered.Length() ? filtered.Length() : pos);
        }
        // Valid if exactly 4 chars, all printable
        if (filtered.Length() == 4) {
            typeText->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
            okButton->Enable(true);
        } else {
            typeText->SetBackgroundColour(wxColour(255, 200, 200));
            okButton->Enable(false);
        }
        typeText->Refresh();
    });
    
    // Initial validation state
    if (typeText->GetValue().Length() != 4) {
        typeText->SetBackgroundColour(wxColour(255, 200, 200));
        okButton->Enable(false);
    }

    if (dialog.ShowModal() == wxID_OK) {
        wxString name = nameText->GetValue();
        wxString typeStr = typeText->GetValue();
        if (name.IsEmpty()) {
            wxMessageBox("Name cannot be empty.", "Error", wxOK | wxICON_ERROR, this);
            return std::make_pair(wxEmptyString, wxEmptyString);
        }
        return std::make_pair(typeStr, name);
    }
    
    return std::make_pair(wxEmptyString, wxEmptyString);
}

void MyFrame::OnReplaceWithBitmap(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("replace with bitmap")) {
        return;
    }

    // Use the reusable dialog to get the new name
    auto result = ShowMoveToDialog(ObjectType::DAT_BITMAP, wxString::FromUTF8(m_currentObject->name));
    wxString newName = result.second;
    if (newName.IsEmpty()) {
        return; // User cancelled or name was empty
    }

    // Create a new bitmap object using the improved createSampleObject function
    auto newObj = std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_BITMAP));
    newObj->setProperty('NAME', newName.ToStdString());
    newObj->updateDateProperty();

    // Use utility function to find and replace object
    bool replaced = ObjectTraversalUtils::FindAndReplaceObject(m_objects, m_currentObject, newObj);

    if (replaced) {
        m_currentObject = newObj;
        SetModified(true);
        RefreshTreeDisplay();
        UpdateObjectPreview();
        SetStatusText("Object changed to Bitmap.");
    } else {
        wxMessageBox("Failed to find and replace object.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnReplaceWithFont(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("replace with font")) {
        return;
    }

    // Use the reusable dialog to get the new name
    auto result = ShowMoveToDialog(ObjectType::DAT_FONT, wxString::FromUTF8(m_currentObject->name));
    wxString newName = result.second;
    if (newName.IsEmpty()) {
        return; // User cancelled or name was empty
    }

    // Create a new font object using the improved createSampleObject function
    auto newObj = std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_FONT));
    newObj->setProperty('NAME', newName.ToStdString());
    newObj->updateDateProperty();

    // Use utility function to find and replace object
    bool replaced = ObjectTraversalUtils::FindAndReplaceObject(m_objects, m_currentObject, newObj);

    if (replaced) {
        m_currentObject = newObj;
        SetModified(true);
        RefreshTreeDisplay();
        UpdateObjectPreview();
        SetStatusText("Object changed to Font.");
    } else {
        wxMessageBox("Failed to find and replace object.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnReplaceWithCompiledSprite(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("replace with compiled sprite")) {
        return;
    }

    // Use the reusable dialog to get the new name
    auto result = ShowMoveToDialog(ObjectType::DAT_C_SPRITE, wxString::FromUTF8(m_currentObject->name));
    wxString newName = result.second;
    if (newName.IsEmpty()) {
        return; // User cancelled or name was empty
    }

    // Create a new compiled sprite object using the improved createSampleObject function
    auto newObj = std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_C_SPRITE));
    newObj->setProperty('NAME', newName.ToStdString());
    newObj->updateDateProperty();

    // Use utility function to find and replace object
    bool replaced = ObjectTraversalUtils::FindAndReplaceObject(m_objects, m_currentObject, newObj);

    if (replaced) {
        m_currentObject = newObj;
        SetModified(true);
        RefreshTreeDisplay();
        UpdateObjectPreview();
        SetStatusText("Object changed to Compiled Sprite.");
    } else {
        wxMessageBox("Failed to find and replace object.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnReplaceWithXSprite(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("replace with X-sprite")) {
        return;
    }

    // Use the reusable dialog to get the new name
    auto result = ShowMoveToDialog(ObjectType::DAT_XC_SPRITE, wxString::FromUTF8(m_currentObject->name));
    wxString newName = result.second;
    if (newName.IsEmpty()) {
        return; // User cancelled or name was empty
    }

    // Create a new X-sprite object using the improved createSampleObject function
    auto newObj = std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_XC_SPRITE));
    newObj->setProperty('NAME', newName.ToStdString());
    newObj->updateDateProperty();

    // Use utility function to find and replace object
    bool replaced = ObjectTraversalUtils::FindAndReplaceObject(m_objects, m_currentObject, newObj);

    if (replaced) {
        m_currentObject = newObj;
        SetModified(true);
        RefreshTreeDisplay();
        UpdateObjectPreview();
        SetStatusText("Object changed to X-Compiled Sprite.");
    } else {
        wxMessageBox("Failed to find and replace object.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnReplaceWithDatafile(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("replace with datafile")) {
        return;
    }

    // Use the reusable dialog to get the new name
    auto result = ShowMoveToDialog(ObjectType::DAT_FILE, wxString::FromUTF8(m_currentObject->name));
    wxString newName = result.second;
    if (newName.IsEmpty()) {
        return; // User cancelled or name was empty
    }

    // Create a new datafile object using the improved createSampleObject function
    auto newObj = std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_FILE));
    newObj->setProperty('NAME', newName.ToStdString());
    newObj->updateDateProperty();

    // Use utility function to find and replace object
    bool replaced = ObjectTraversalUtils::FindAndReplaceObject(m_objects, m_currentObject, newObj);

    if (replaced) {
        m_currentObject = newObj;
        SetModified(true);
        RefreshTreeDisplay();
        UpdateObjectPreview();
        SetStatusText("Object changed to Datafile.");
    } else {
        wxMessageBox("Failed to find and replace object.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnReplaceWithFLI(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("replace with FLI")) {
        return;
    }

    // Use the reusable dialog to get the new name
    auto result = ShowMoveToDialog(ObjectType::DAT_FLI, wxString::FromUTF8(m_currentObject->name));
    wxString newName = result.second;
    if (newName.IsEmpty()) {
        return; // User cancelled or name was empty
    }

    // Create a new FLI object using the improved createSampleObject function
    auto newObj = std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_FLI));
    newObj->setProperty('NAME', newName.ToStdString());
    newObj->updateDateProperty();

    // Use utility function to find and replace object
    bool replaced = ObjectTraversalUtils::FindAndReplaceObject(m_objects, m_currentObject, newObj);

    if (replaced) {
        m_currentObject = newObj;
        SetModified(true);
        RefreshTreeDisplay();
        UpdateObjectPreview();
        SetStatusText("Object changed to FLI Animation.");
    } else {
        wxMessageBox("Failed to find and replace object.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnReplaceWithMIDI(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("replace with MIDI")) {
        return;
    }

    // Use the reusable dialog to get the new name
    auto result = ShowMoveToDialog(ObjectType::DAT_MIDI, wxString::FromUTF8(m_currentObject->name));
    wxString newName = result.second;
    if (newName.IsEmpty()) {
        return; // User cancelled or name was empty
    }

    // Create a new MIDI object using the improved createSampleObject function
    auto newObj = std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_MIDI));
    newObj->setProperty('NAME', newName.ToStdString());
    newObj->updateDateProperty();

    // Use utility function to find and replace object
    bool replaced = ObjectTraversalUtils::FindAndReplaceObject(m_objects, m_currentObject, newObj);

    if (replaced) {
        m_currentObject = newObj;
        SetModified(true);
        RefreshTreeDisplay();
        UpdateObjectPreview();
        SetStatusText("Object changed to MIDI File.");
    } else {
        wxMessageBox("Failed to find and replace object.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnReplaceWithPalette(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("replace with palette")) {
        return;
    }

    // Use the reusable dialog to get the new name
    auto result = ShowMoveToDialog(ObjectType::DAT_PALETTE, wxString::FromUTF8(m_currentObject->name));
    wxString newName = result.second;
    if (newName.IsEmpty()) {
        return; // User cancelled or name was empty
    }

    // Create a new palette object using the current palette data
    DataParser::DataObject paletteObj;
    if (DataParser::createFromPalette(m_currentPalette, paletteObj)) {
        paletteObj.setProperty('NAME', newName.ToStdString());
        paletteObj.updateDateProperty();
        
        // Create shared_ptr to the palette object
        auto newObj = std::make_shared<DataParser::DataObject>(std::move(paletteObj));
        
        // Use utility function to find and replace object
        bool replaced = ObjectTraversalUtils::FindAndReplaceObject(m_objects, m_currentObject, newObj);

        if (replaced) {
            m_currentObject = newObj;
            SetModified(true);
            RefreshTreeDisplay();
            UpdateObjectPreview();
            SetStatusText("Object changed to Palette.");
        } else {
            wxMessageBox("Failed to find and replace object.", "Error", wxOK | wxICON_ERROR);
        }
    } else {
        wxMessageBox("Failed to create palette object.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnReplaceWithRLE(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("replace with RLE sprite")) {
        return;
    }

    // Use the reusable dialog to get the new name
    auto result = ShowMoveToDialog(ObjectType::DAT_RLE_SPRITE, wxString::FromUTF8(m_currentObject->name));
    wxString newName = result.second;
    if (newName.IsEmpty()) {
        return; // User cancelled or name was empty
    }

    // Create a new RLE sprite object using the improved createSampleObject function
    auto newObj = std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_RLE_SPRITE));
    newObj->setProperty('NAME', newName.ToStdString());
    newObj->updateDateProperty();

    // Use utility function to find and replace object
    bool replaced = ObjectTraversalUtils::FindAndReplaceObject(m_objects, m_currentObject, newObj);

    if (replaced) {
        m_currentObject = newObj;
        SetModified(true);
        RefreshTreeDisplay();
        UpdateObjectPreview();
        SetStatusText("Object changed to RLE Sprite.");
    } else {
        wxMessageBox("Failed to find and replace object.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnReplaceWithSample(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("replace with sample")) {
        return;
    }

    // Use the reusable dialog to get the new name
    auto result = ShowMoveToDialog(ObjectType::DAT_SAMP, wxString::FromUTF8(m_currentObject->name));
    wxString newName = result.second;
    if (newName.IsEmpty()) {
        return; // User cancelled or name was empty
    }

    // Create a new sample object using the improved createSampleObject function
    auto newObj = std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_SAMP));
    newObj->setProperty('NAME', newName.ToStdString());
    newObj->updateDateProperty();

    // Use utility function to find and replace object
    bool replaced = ObjectTraversalUtils::FindAndReplaceObject(m_objects, m_currentObject, newObj);

    if (replaced) {
        m_currentObject = newObj;
        SetModified(true);
        RefreshTreeDisplay();
        UpdateObjectPreview();
        SetStatusText("Object changed to Audio Sample.");
    } else {
        wxMessageBox("Failed to find and replace object.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnReplaceWithOther(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("replace with other")) {
        return;
    }

    // Use the reusable dialog to get both type and name
    auto result = ShowMoveToDialog(ObjectType::DAT_DATA, wxString::FromUTF8(m_currentObject->name), true);
    wxString newType = result.first;
    wxString newName = result.second;
    
    if (newType.IsEmpty() || newName.IsEmpty()) {
        return; // User cancelled or input was empty
    }

    // Convert type string to uint32_t (big endian)
    uint32_t typeID = ((unsigned char)newType[0] << 24) |
                      ((unsigned char)newType[1] << 16) |
                      ((unsigned char)newType[2] << 8)  |
                      ((unsigned char)newType[3]);
    
    // Create a new object using the custom type
    auto newObj = std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(static_cast<ObjectType>(typeID)));
    newObj->setProperty('NAME', newName.ToStdString());
    newObj->updateDateProperty();

    // Use utility function to find and replace object
    bool replaced = ObjectTraversalUtils::FindAndReplaceObject(m_objects, m_currentObject, newObj);

    if (replaced) {
        m_currentObject = newObj;
        SetModified(true);
        RefreshTreeDisplay();
        UpdateObjectPreview();
        SetStatusText("Object changed to " + newType + ".");
    } else {
        wxMessageBox("Failed to find and replace object.", "Error", wxOK | wxICON_ERROR);
    }
}

void MyFrame::OnRenameObject(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("rename object")) {
        return;
    }

    wxDialog dialog(this, wxID_ANY, "Rename", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 2, 8, 8);
    gridSizer->AddGrowableCol(1, 1);

    wxStaticText* nameLabel = new wxStaticText(&dialog, wxID_ANY, "Name:");
    wxTextCtrl* nameText = new wxTextCtrl(&dialog, wxID_ANY, wxString::FromUTF8(m_currentObject->name), wxDefaultPosition, wxSize(180, -1));
    gridSizer->Add(nameLabel, 0, wxALIGN_LEFT | wxALL, 5);
    gridSizer->Add(nameText, 1, wxEXPAND | wxALL, 5);

    mainSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(&dialog, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(&dialog, wxID_CANCEL, "Cancel");
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);

    dialog.SetSizer(mainSizer);
    mainSizer->Fit(&dialog);
    dialog.CentreOnParent();

    nameText->SetFocus();
    nameText->SelectAll();

    if (dialog.ShowModal() == wxID_OK) {
        wxString newName = nameText->GetValue();
        if (newName.IsEmpty()) {
            wxMessageBox("Name cannot be empty.", "Error", wxOK | wxICON_ERROR);
            return;
        }

        m_currentObject->setProperty('NAME', newName.ToStdString());
        m_currentObject->updateDateProperty();

        SetModified(true);
        RefreshTreeDisplay();
        UpdateObjectPreview();
        SetStatusText("Object renamed to " + newName);
    }
}

void MyFrame::OnSetProperty(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("set property")) {
        return;
    }

    wxDialog dialog(this, wxID_ANY, "Set Property", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 2, 8, 8);
    gridSizer->AddGrowableCol(1, 1);

    wxStaticText* idLabel = new wxStaticText(&dialog, wxID_ANY, "ID:");
    wxTextCtrl* idText = new wxTextCtrl(&dialog, wxID_ANY, "", wxDefaultPosition, wxSize(60, -1));
    gridSizer->Add(idLabel, 0, wxALIGN_LEFT | wxALL, 5);
    gridSizer->Add(idText, 1, wxEXPAND | wxALL, 5);

    wxStaticText* valueLabel = new wxStaticText(&dialog, wxID_ANY, "Value:");
    wxTextCtrl* valueText = new wxTextCtrl(&dialog, wxID_ANY, "", wxDefaultPosition, wxSize(180, -1));
    gridSizer->Add(valueLabel, 0, wxALIGN_LEFT | wxALL, 5);
    gridSizer->Add(valueText, 1, wxEXPAND | wxALL, 5);

    mainSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(&dialog, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(&dialog, wxID_CANCEL, "Cancel");
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 10);

    dialog.SetSizer(mainSizer);
    mainSizer->Fit(&dialog);
    dialog.CentreOnParent();

    idText->SetFocus();

    idText->Bind(wxEVT_TEXT, [idText, okButton](wxCommandEvent& evt) {
        wxString value = idText->GetValue();
        wxString filtered;
        for (wxChar c : value) {
            if (c >= 32 && c <= 126 && filtered.Length() < 4) {
                filtered += c;
            }
        }
        if (filtered != value) {
            int pos = idText->GetInsertionPoint();
            idText->ChangeValue(filtered);
            idText->SetInsertionPoint(pos > filtered.Length() ? filtered.Length() : pos);
        }
        okButton->Enable(filtered.Length() == 4);
    });
    okButton->Enable(false);

    if (dialog.ShowModal() == wxID_OK) {
        wxString propIdStr = idText->GetValue();
        wxString propValue = valueText->GetValue();

        if (propIdStr.Length() != 4) {
            wxMessageBox("Property ID must be 4 characters long.", "Error", wxOK | wxICON_ERROR);
            return;
        }

        uint32_t propId = ((unsigned char)propIdStr[0] << 24) |
                          ((unsigned char)propIdStr[1] << 16) |
                          ((unsigned char)propIdStr[2] << 8) |
                          ((unsigned char)propIdStr[3]);

        m_currentObject->setProperty(propId, propValue.ToStdString());
        m_currentObject->updateDateProperty();

        SetModified(true);
        UpdatePropertyList(m_currentObject);
        SetStatusText("Property " + propIdStr + " set.");
    }
}

// Update OnAutocrop to support multiple selection
void MyFrame::OnAutocrop(wxCommandEvent& event)
{
    auto selectedObjs = GetSelectedObjects();
    if (selectedObjs.empty()) {
        wxMessageBox("No objects selected for autocrop.", "Autocrop", wxOK | wxICON_INFORMATION);
        return;
    }
    int cropped = 0, ignored = 0;
    for (auto& obj : selectedObjs) {
        if (!obj->isBitmap()) {
            ++ignored;
            continue;
        }
        BitmapData& bitmap = obj->getBitmap();
        int xCrop, yCrop;
        if (bitmap.autoCrop(xCrop, yCrop)) {
            obj->updateDateProperty();
            ++cropped;
        }
    }
    if (cropped > 0) {
        SetModified(true);
        RefreshTreeDisplay();
        SetStatusText(wxString::Format("%d bitmap(s) autocropped.", cropped));
    }
    if (ignored > 0) {
        wxMessageBox(wxString::Format("%d non-bitmap object(s) were ignored", ignored), "Autocrop", wxOK | wxICON_INFORMATION);
    }
}

// Update OnDepth* to support multiple selection
#define MULTI_DEPTH_HANDLER(FUNC, bits, label) \
void MyFrame::FUNC(wxCommandEvent& event) { \
    auto selectedObjs = GetSelectedObjects(); \
    if (selectedObjs.empty()) { \
        wxMessageBox("No objects selected for depth change.", "Change Depth", wxOK | wxICON_INFORMATION); \
        return; \
    } \
    int changed = 0, ignored = 0; \
    for (auto& obj : selectedObjs) { \
        if (!obj->isBitmap()) { ++ignored; continue; } \
        BitmapData& bmpData = const_cast<BitmapData&>(obj->getBitmap()); \
        if (bmpData.setColorDepth(bits, m_currentPalette, m_grabberInfo.GetDither(), m_grabberInfo.GetTransparency())) { \
            obj->updateDateProperty(); \
            ++changed; \
        } \
    } \
    if (changed > 0) { SetModified(true); RefreshTreeDisplay(); SetStatusText(wxString::Format("%d object(s) converted to %s", changed, label)); } \
    if (ignored > 0) { wxMessageBox(wxString::Format("%d non-bitmap object(s) were ignored", ignored), "Change Depth", wxOK | wxICON_INFORMATION); } \
}
MULTI_DEPTH_HANDLER(OnDepth256, 8, "256 color palette")
MULTI_DEPTH_HANDLER(OnDepth15, 15, "15-bit hicolor")
MULTI_DEPTH_HANDLER(OnDepth16, 16, "16-bit hicolor")
MULTI_DEPTH_HANDLER(OnDepth24, 24, "24-bit truecolor")
MULTI_DEPTH_HANDLER(OnDepth32, 32, "32-bit truecolor")
#undef MULTI_DEPTH_HANDLER

// Update OnChangeFilenameToRelative/Absolute to support multiple selection
void MyFrame::OnChangeFilenameToRelative(wxCommandEvent& event) {
    auto selectedObjs = GetSelectedObjects();
    if (selectedObjs.empty()) {
        wxMessageBox("No objects selected for filename change.", "Change Filename", wxOK | wxICON_INFORMATION);
        return;
    }
    int changed = 0, ignored = 0;
    wxString cwd = wxGetCwd();
    for (auto& obj : selectedObjs) {
        std::string orig = obj->getProperty('ORIG');
        if (orig.empty()) { ++ignored; continue; }
        wxFileName fileName(wxString::FromUTF8(orig));
        if (!fileName.IsAbsolute()) { ++ignored; continue; }
        fileName.MakeRelativeTo(cwd);
        std::string newOrig = fileName.GetFullPath().ToStdString();
        if (newOrig == orig) { ++ignored; continue; }
        obj->setProperty('ORIG', newOrig);
        obj->updateDateProperty();
        ++changed;
    }
    if (changed > 0) { SetModified(true); RefreshTreeDisplay(); SetStatusText(wxString::Format("%d object(s) changed to relative filename", changed)); }
    if (ignored > 0) { wxMessageBox(wxString::Format("%d object(s) were ignored", ignored), "Change Filename", wxOK | wxICON_INFORMATION); }
}
void MyFrame::OnChangeFilenameToAbsolute(wxCommandEvent& event) {
    auto selectedObjs = GetSelectedObjects();
    if (selectedObjs.empty()) {
        wxMessageBox("No objects selected for filename change.", "Change Filename", wxOK | wxICON_INFORMATION);
        return;
    }
    int changed = 0, ignored = 0;
    wxString cwd = wxGetCwd();
    for (auto& obj : selectedObjs) {
        std::string orig = obj->getProperty('ORIG');
        if (orig.empty()) { ++ignored; continue; }
        wxFileName fileName(wxString::FromUTF8(orig));
        if (fileName.IsAbsolute()) { ++ignored; continue; }
        fileName.MakeAbsolute(cwd);
        std::string newOrig = fileName.GetFullPath().ToStdString();
        if (newOrig == orig) { ++ignored; continue; }
        obj->setProperty('ORIG', newOrig);
        obj->updateDateProperty();
        ++changed;
    }
    if (changed > 0) { SetModified(true); RefreshTreeDisplay(); SetStatusText(wxString::Format("%d object(s) changed to absolute filename", changed)); }
    if (ignored > 0) { wxMessageBox(wxString::Format("%d object(s) were ignored", ignored), "Change Filename", wxOK | wxICON_INFORMATION); }
}

// Update OnChangeType* to support multiple selection
#define MULTI_TYPE_HANDLER(FUNC, TYPE, LABEL) \
void MyFrame::FUNC(wxCommandEvent& event) { \
    auto selectedObjs = GetSelectedObjects(); \
    if (selectedObjs.empty()) { \
        wxMessageBox("No objects selected for type change.", "Change Type", wxOK | wxICON_INFORMATION); \
        return; \
    } \
    int changed = 0, failed = 0; \
    for (auto& obj : selectedObjs) { \
        if (!obj->isBitmap()) { ++failed; continue; } \
        BitmapData& bmp = obj->getBitmap(); \
        if (!bmp.setType(TYPE)) { ++failed; continue; } \
        obj->typeID = TYPE; \
        ++changed; \
    } \
    if (changed > 0) { SetModified(true); RefreshTreeDisplay(); SetStatusText(wxString::Format("%d object(s) converted to %s", changed, LABEL)); } \
    if (failed > 0) { wxMessageBox(wxString::Format("%d object(s) failed to convert or were not bitmaps", failed), "Change Type", wxOK | wxICON_INFORMATION); } \
}
MULTI_TYPE_HANDLER(OnChangeTypeToBitmap, ObjectType::DAT_BITMAP, "Bitmap")
MULTI_TYPE_HANDLER(OnChangeTypeToRLE, ObjectType::DAT_RLE_SPRITE, "RLE Sprite")
MULTI_TYPE_HANDLER(OnChangeTypeToCompiled, ObjectType::DAT_C_SPRITE, "Compiled Sprite")
MULTI_TYPE_HANDLER(OnChangeTypeToXCompiled, ObjectType::DAT_XC_SPRITE, "X Sprite")
#undef MULTI_TYPE_HANDLER

void MyFrame::OnShellEdit(wxCommandEvent& event)
{
    // 1. Check for object selection
    if (!m_currentObject) {
        wxMessageBox("No object selected. Please select an object first.", "No Selection", wxOK | wxICON_INFORMATION);
        return;
    }

    // 2. Lookup shell command in allegro.cfg via GrabberInfo
    std::string typeStr = DataParser::ConvertIDToString(m_currentObject->typeID);
    std::string shellCmd = m_grabberInfo.GetShellCommandForType(typeStr);
    if (shellCmd.empty()) {
        wxMessageBox(wxString::Format(
            "No shell association for this object type!\nAdd a \"%s=command\" line to the [grabber] section in allegro.cfg", typeStr),
            "No Shell Association", wxOK | wxICON_INFORMATION);
        return;
    }

    // 3. Export to temp file (choose extension based on type)
    wxString ext;
    if (m_currentObject->isBitmap()) ext = "bmp";
    else if (m_currentObject->isFont()) ext = "fnt";
    else if (m_currentObject->isAudio()) {
        if (m_currentObject->typeID == ObjectType::DAT_SAMP) ext = "wav";
        else if (m_currentObject->typeID == ObjectType::DAT_OGG) ext = "ogg";
        else if (m_currentObject->typeID == ObjectType::DAT_MIDI) ext = "mid";
        else ext = "bin";
    } else if (m_currentObject->isVideo()) ext = "fli";
    else ext = "bin";

    wxFileName tempFile = wxFileName::CreateTempFileName("grabber_shell_edit_");
    tempFile.SetExt(ext);
    wxString tempPath = tempFile.GetFullPath();

    // Export the object to the temp file (reuse export logic)
    bool exportSuccess = false;
    if (m_currentObject->isBitmap()) {
        wxImage image;
        if (m_currentObject->getBitmap().toWxImage(image, m_currentPalette)) {
            exportSuccess = image.SaveFile(tempPath, wxBITMAP_TYPE_BMP);
        }
    } else if (m_currentObject->isFont()) {
        const FontData& fontData = m_currentObject->getFont();
        exportSuccess = FontData::ExportRangeAsFnt(fontData.ranges[0], tempPath);
    } else if (m_currentObject->isAudio()) {
        const AudioData& audioData = m_currentObject->getAudio();
        if (m_currentObject->typeID == ObjectType::DAT_SAMP) {
            std::vector<uint8_t> wavData = audioData.getWavData();
            std::ofstream outFile(tempPath.ToStdString(), std::ios::binary);
            if (outFile) {
                outFile.write(reinterpret_cast<const char*>(wavData.data()), wavData.size());
                exportSuccess = outFile.good();
                outFile.close();
            }
        } else if (m_currentObject->typeID == ObjectType::DAT_OGG) {
            const std::vector<uint8_t>& data = audioData.data;
            std::ofstream outFile(tempPath.ToStdString(), std::ios::binary);
            if (outFile) {
                outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
                exportSuccess = outFile.good();
                outFile.close();
            }
        } else if (m_currentObject->typeID == ObjectType::DAT_MIDI) {
            const std::vector<uint8_t>& data = audioData.getMidiData();
            std::ofstream outFile(tempPath.ToStdString(), std::ios::binary);
            if (outFile) {
                outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
                exportSuccess = outFile.good();
                outFile.close();
            }
        }
    } else if (m_currentObject->isVideo()) {
        const VideoData& videoData = m_currentObject->getVideo();
        const std::vector<uint8_t>& data = videoData.data;
        std::ofstream outFile(tempPath.ToStdString(), std::ios::binary);
        if (outFile) {
            outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
            exportSuccess = outFile.good();
            outFile.close();
        }
    } else if (m_currentObject->isRawData()) {
        const std::vector<uint8_t>& data = m_currentObject->getRawData();
        std::ofstream outFile(tempPath.ToStdString(), std::ios::binary);
        if (outFile) {
            outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
            exportSuccess = outFile.good();
            outFile.close();
        }
    }

    if (!exportSuccess) {
        wxMessageBox("Failed to export object to temporary file for shell editing.", "Export Error", wxOK | wxICON_ERROR);
        return;
    }

    // 4. Launch command (replace %1 or add file as argument)
    wxString command = wxString::FromUTF8(shellCmd);
    if (command.Contains("%1")) {
        command.Replace("%1", tempPath);
    } else {
        command += " \"" + tempPath + "\"";
    }
    // Launch the command and wait for it to finish
    wxExecute(command, wxEXEC_SYNC);

    // 5. Detect changes and prompt for import
    wxFileName tempFileCheck(tempPath);
    if (!tempFileCheck.FileExists()) {
        wxMessageBox("Temporary file was deleted or not found after editing.", "Shell Edit", wxOK | wxICON_WARNING);
        return;
    }
    static std::map<wxString, wxDateTime> lastEditTimes;
    wxDateTime lastEdit = tempFileCheck.GetModificationTime();
    wxDateTime prevEdit = lastEditTimes[tempPath];
    lastEditTimes[tempPath] = lastEdit;
    if (prevEdit.IsValid() && lastEdit <= prevEdit) {
        wxMessageBox("No changes detected in the file after editing.", "Shell Edit", wxOK | wxICON_INFORMATION);
        return;
    }
    int answer = wxMessageBox("The file was modified. Do you want to import the changes back into the datafile?", "Shell Edit", wxYES_NO | wxICON_QUESTION);
    if (answer == wxYES) {
        // Import logic (reuse Grab/Replace logic)
        wxString dummyPath = tempPath;
        if (m_currentObject->isBitmap()) {
            wxImage image;
            if (BitmapData::readFileToWxImage(dummyPath, image)) {
                BitmapData newBmp;
                if (newBmp.loadFromWxImage(image, m_currentObject->getBitmap().bits, &m_currentPalette, m_grabberInfo.GetDither(), m_grabberInfo.GetTransparency())) {
                    m_currentObject->data = newBmp;
                    SetModified(true);
                    UpdateObjectPreview();
                    SetStatusText("Bitmap updated from shell edit.");
                }
            }
        } else if (m_currentObject->isFont()) {
            FontData fontData;
            if (FontEditDialog::GrabFontFromFile(this, fontData, dummyPath)) {
                m_currentObject->data = fontData;
                SetModified(true);
                UpdateObjectPreview();
                SetStatusText("Font updated from shell edit.");
            }
        } else if (m_currentObject->isAudio()) {
            AudioData audioData = m_currentObject->getAudio();
            if (audioData.importFromFile(dummyPath.ToStdString())) {
                m_currentObject->data = audioData;
                SetModified(true);
                UpdateObjectPreview();
                SetStatusText("Audio updated from shell edit.");
            }
        } else if (m_currentObject->isVideo()) {
            VideoData videoData;
            if (videoData.importFromFile(dummyPath.ToStdString())) {
                m_currentObject->data = videoData;
                SetModified(true);
                UpdateObjectPreview();
                SetStatusText("Video updated from shell edit.");
            }
        } else if (m_currentObject->isRawData()) {
            std::ifstream file(dummyPath.ToStdString(), std::ios::binary);
            if (file) {
                std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                m_currentObject->data = data;
                SetModified(true);
                UpdateObjectPreview();
                SetStatusText("Raw data updated from shell edit.");
            }
        }
    }
    // Optionally: remove temp file
    // wxRemoveFile(tempPath);
}

void MyFrame::OnSortObjects(wxCommandEvent& event)
{
    // Update the sort flag in GrabberInfo
    wxMenuBar* menuBar = GetMenuBar();
    if (menuBar) {
        wxMenu* optionsMenu = menuBar->GetMenu(menuBar->FindMenu("O&ptions"));
        if (optionsMenu) {
            m_grabberInfo.SetSort(optionsMenu->IsChecked(ID_SORT_OBJECTS));
        }
    }
    // Refresh the tree display (will sort if enabled)
    RefreshTreeDisplay();
}

void MyFrame::SortObjectsByName(std::vector<std::shared_ptr<DataParser::DataObject>>& objects) {
    std::sort(objects.begin(), objects.end(), [](const std::shared_ptr<DataParser::DataObject>& a, const std::shared_ptr<DataParser::DataObject>& b) {
        return a->getProperty('NAME') < b->getProperty('NAME');
    });
    for (auto& obj : objects) {
        if (obj->isNested()) {
            // For nested objects, we need to sort their nested objects too
            // Since this is a member function, we can call it recursively
            SortObjectsByName(obj->getNestedObjects());
        }
    }
}

void MyFrame::OnStoreRelativeFilenames(wxCommandEvent& event)
{
    // Update the store relative filenames flag in GrabberInfo
    wxMenuBar* menuBar = GetMenuBar();
    if (menuBar) {
        wxMenu* optionsMenu = menuBar->GetMenu(menuBar->FindMenu("O&ptions"));
        if (optionsMenu) {
            m_grabberInfo.SetRelativeFilenames(optionsMenu->IsChecked(ID_STORE_RELATIVE));
        }
    }
}

void MyFrame::SetOrigPropertyWithFormat(std::shared_ptr<DataParser::DataObject> obj, const std::string& path)
{
    if (path.empty()) {
        obj->setProperty('ORIG', "");
        return;
    }
    
    wxFileName fileName(wxString::FromUTF8(path));
    std::string formattedPath;
    
    if (m_grabberInfo.GetRelativeFilenames()) {
        // Convert to relative path
        if (fileName.IsAbsolute()) {
            wxString cwd = wxGetCwd();
            fileName.MakeRelativeTo(cwd);
            formattedPath = fileName.GetFullPath().ToStdString();
        } else {
            formattedPath = path; // Already relative
        }
    } else {
        // Convert to absolute path
        if (!fileName.IsAbsolute()) {
            wxString cwd = wxGetCwd();
            fileName.MakeAbsolute(cwd);
            formattedPath = fileName.GetFullPath().ToStdString();
        } else {
            formattedPath = path; // Already absolute
        }
    }
    
    obj->setProperty('ORIG', formattedPath);
}

void MyFrame::OnDitherImages(wxCommandEvent& event)
{
    wxMenuBar* menuBar = GetMenuBar();
    if (menuBar) {
        wxMenu* optionsMenu = menuBar->GetMenu(menuBar->FindMenu("O&ptions"));
        if (optionsMenu) {
            m_grabberInfo.SetDither(optionsMenu->IsChecked(ID_DITHER_IMAGES));
        }
    }
    SetModified(true);
}

void MyFrame::OnPreserveTransparency(wxCommandEvent& event)
{
    wxMenuBar* menuBar = GetMenuBar();
    if (menuBar) {
        wxMenu* optionsMenu = menuBar->GetMenu(menuBar->FindMenu("O&ptions"));
        if (optionsMenu) {
            m_grabberInfo.SetTransparency(optionsMenu->IsChecked(ID_PRESERVE_TRANSPARENCY));
        }
    }
    SetModified(true);
}

// Handler stub for tree item context menu
void MyFrame::OnTreeItemMenu(wxTreeEvent& event) {
    wxTreeItemId item = event.GetItem();
    if (!item.IsOk()) return;
    m_tree->SelectItem(item);

    wxArrayTreeItemIds selectedItems;
    m_tree->GetSelections(selectedItems);
    bool multi = selectedItems.size() > 1;

    wxMenu menu;
    if (item == m_tree->GetRootItem()) {
        // Only show New submenu for root
        wxMenu* newMenu = new wxMenu();
        newMenu->Append(ID_NEW_BITMAP, "Bitmap");
        newMenu->Append(ID_NEW_RLE, "RLE Sprite");
        newMenu->Append(ID_NEW_COMPILED_SPRITE, "Compiled Sprite");
        newMenu->Append(ID_NEW_X_SPRITE, "Compiled X-sprite");
        newMenu->Append(ID_NEW_DATAFILE, "Datafile");
        newMenu->Append(ID_NEW_FLI, "FLI/FLC animation");
        newMenu->Append(ID_NEW_FONT, "Font");
        newMenu->Append(ID_NEW_MIDI, "MIDI file");
        newMenu->Append(ID_NEW_PALETTE, "Palette");
        newMenu->Append(ID_NEW_SAMPLE, "Sample");
        newMenu->Append(ID_NEW_OGG_AUDIO, "Ogg audio");
        newMenu->Append(ID_NEW_OTHER, "Other");
        menu.AppendSubMenu(newMenu, "New");
        PopupMenu(&menu);
        return;
    }
    if (multi) {
        // ... existing multi-selection menu ...
        menu.Append(ID_DELETE, "Delete");
        menu.Append(ID_AUTOCROP, "Autocrop");
        wxMenu* depthMenu = new wxMenu();
        depthMenu->Append(ID_DEPTH_256, "8-bit (256)");
        depthMenu->Append(ID_DEPTH_15, "15-bit");
        depthMenu->Append(ID_DEPTH_16, "16-bit");
        depthMenu->Append(ID_DEPTH_24, "24-bit");
        depthMenu->Append(ID_DEPTH_32, "32-bit");
        menu.AppendSubMenu(depthMenu, "Change Depth");
        wxMenu* filenameMenu = new wxMenu();
        filenameMenu->Append(ID_FILENAME_RELATIVE, "To Relative");
        filenameMenu->Append(ID_FILENAME_ABSOLUTE, "To Absolute");
        menu.AppendSubMenu(filenameMenu, "Change Filename");
        wxMenu* typeMenu = new wxMenu();
        typeMenu->Append(ID_TYPE_BITMAP, "To Bitmap");
        typeMenu->Append(ID_TYPE_RLE, "To RLE Sprite");
        typeMenu->Append(ID_TYPE_COMPILED, "To Compiled Sprite");
        typeMenu->Append(ID_TYPE_X_COMPILED, "To X Sprite");
        menu.AppendSubMenu(typeMenu, "Change Type");
        wxMenu* newMenu = new wxMenu();
        newMenu->Append(ID_NEW_BITMAP, "Bitmap");
        newMenu->Append(ID_NEW_RLE, "RLE Sprite");
        newMenu->Append(ID_NEW_COMPILED_SPRITE, "Compiled Sprite");
        newMenu->Append(ID_NEW_X_SPRITE, "X Sprite");
        newMenu->Append(ID_NEW_DATAFILE, "Datafile");
        newMenu->Append(ID_NEW_FLI, "FLI");
        newMenu->Append(ID_NEW_FONT, "Font");
        newMenu->Append(ID_NEW_MIDI, "MIDI");
        newMenu->Append(ID_NEW_PALETTE, "Palette");
        newMenu->Append(ID_NEW_SAMPLE, "Sample");
        newMenu->Append(ID_NEW_OGG_AUDIO, "Ogg audio");
        newMenu->Append(ID_NEW_OTHER, "Other");
        menu.AppendSubMenu(newMenu, "New");
    } else {
        // ... existing single-selection menu ...
        menu.Append(ID_GRAB, "Grab");
        menu.Append(ID_EXPORT, "Export");
        menu.Append(ID_DELETE, "Delete");
        wxMenu* moveMenu = new wxMenu();
        moveMenu->Append(ID_MOVE_UP, "Up");
        moveMenu->Append(ID_MOVE_DOWN, "Down");
        menu.AppendSubMenu(moveMenu, "Move");
        wxMenu* replaceMenu = new wxMenu();
        replaceMenu->Append(ID_REPLACE_BITMAP, "Bitmap");
        replaceMenu->Append(ID_REPLACE_COMPILED_SPRITE, "Compiled sprite");
        replaceMenu->Append(ID_REPLACE_X_SPRITE, "Compiled X-sprite");
        replaceMenu->Append(ID_REPLACE_DATAFILE, "Datafile");
        replaceMenu->Append(ID_REPLACE_FLI, "FLI/FLC animation");
        replaceMenu->Append(ID_REPLACE_FONT, "Font");
        replaceMenu->Append(ID_REPLACE_MIDI, "MIDI file");
        replaceMenu->Append(ID_REPLACE_PALETTE, "Palette");
        replaceMenu->Append(ID_REPLACE_RLE, "RLE sprite");
        replaceMenu->Append(ID_REPLACE_SAMPLE, "Sample");
        replaceMenu->Append(ID_REPLACE_OTHER, "Other");
        menu.AppendSubMenu(replaceMenu, "Replace");
        menu.Append(ID_RENAME, "Rename");
        menu.Append(ID_AUTOCROP, "Autocrop");
        menu.Append(ID_UNGRAB, "Ungrab");
        wxMenu* alphaMenu = new wxMenu();
        alphaMenu->Append(ID_VIEW_ALPHA, "View Alpha");
        alphaMenu->Append(ID_IMPORT_ALPHA, "Import Alpha");
        alphaMenu->Append(ID_EXPORT_ALPHA, "Export Alpha");
        alphaMenu->Append(ID_DELETE_ALPHA, "Delete Alpha");
        menu.AppendSubMenu(alphaMenu, "Alpha Channel");
        wxMenu* depthMenu = new wxMenu();
        depthMenu->Append(ID_DEPTH_256, "8-bit (256)");
        depthMenu->Append(ID_DEPTH_15, "15-bit");
        depthMenu->Append(ID_DEPTH_16, "16-bit");
        depthMenu->Append(ID_DEPTH_24, "24-bit");
        depthMenu->Append(ID_DEPTH_32, "32-bit");
        menu.AppendSubMenu(depthMenu, "Change Depth");
        wxMenu* filenameMenu = new wxMenu();
        filenameMenu->Append(ID_FILENAME_RELATIVE, "To Relative");
        filenameMenu->Append(ID_FILENAME_ABSOLUTE, "To Absolute");
        menu.AppendSubMenu(filenameMenu, "Change Filename");
        wxMenu* typeMenu = new wxMenu();
        typeMenu->Append(ID_TYPE_BITMAP, "To Bitmap");
        typeMenu->Append(ID_TYPE_RLE, "To RLE Sprite");
        typeMenu->Append(ID_TYPE_COMPILED, "To Compiled Sprite");
        typeMenu->Append(ID_TYPE_X_COMPILED, "To X Sprite");
        menu.AppendSubMenu(typeMenu, "Change Type");
        menu.Append(ID_SHELL_EDIT, "Shell Edit");
        wxMenu* newMenu = new wxMenu();
        newMenu->Append(ID_NEW_BITMAP, "Bitmap");
        newMenu->Append(ID_NEW_RLE, "RLE Sprite");
        newMenu->Append(ID_NEW_COMPILED_SPRITE, "Compiled Sprite");
        newMenu->Append(ID_NEW_X_SPRITE, "X Sprite");
        newMenu->Append(ID_NEW_DATAFILE, "Datafile");
        newMenu->Append(ID_NEW_FLI, "FLI");
        newMenu->Append(ID_NEW_FONT, "Font");
        newMenu->Append(ID_NEW_MIDI, "MIDI");
        newMenu->Append(ID_NEW_PALETTE, "Palette");
        newMenu->Append(ID_NEW_SAMPLE, "Sample");
        newMenu->Append(ID_NEW_OGG_AUDIO, "Ogg audio");
        newMenu->Append(ID_NEW_OTHER, "Other");
        menu.AppendSubMenu(newMenu, "New");
    }
    PopupMenu(&menu);
}

// Handler for Box Grab
void MyFrame::OnBoxGrab(wxCommandEvent& event)
{
    if (!ValidateObjectSelection("box grab")) {
        return;
    }
    if (!m_currentObject->isBitmap()) {
        wxMessageBox("Box grab is only available for bitmaps.", "Error", wxOK | wxICON_ERROR);
        return;
    }
    if (!m_loadedImage || !m_loadedImage->IsOk()) {
        wxMessageBox("Please read a bitmap first using File->Read Bitmap.", "No Bitmap Loaded", wxOK | wxICON_EXCLAMATION);
        return;
    }
    // Use point mode for GrabPreviewDialog
    GrabPreviewDialog dialog(this, *m_loadedImage, "Box Grab", 0, 0, GrabPreviewDialog::SelectionMode::Point);
    if (dialog.ShowModal() == wxID_OK) {
        wxRect selection = dialog.GetSelection();
        wxImage croppedImage = *m_loadedImage; // Copy the image
        if (!selection.IsEmpty()) {
            croppedImage = m_loadedImage->GetSubImage(selection);
        }
        BitmapData newBitmapData;
        if (!newBitmapData.loadFromWxImage(croppedImage, m_currentObject->isBitmap() ? m_currentObject->getBitmap().bits : 32, &m_currentPalette, m_grabberInfo.GetDither(), m_grabberInfo.GetTransparency())) {
            wxMessageBox("Failed to convert image to bitmap format.", "Error", wxOK | wxICON_ERROR);
            return;
        }
        if (m_currentObject->isBitmap()) {
            m_currentObject->getBitmap() = newBitmapData;
        } else {
            m_currentObject->data = newBitmapData;
            m_currentObject->typeID = ObjectType::DAT_BITMAP;
        }
        m_currentObject->updateDateProperty();
        SetModified(true);
        UpdatePropertyList(m_currentObject);
        UpdateObjectPreview();
        SetStatusText("Object data updated with box grab.");
    }
}

void MyFrame::OnUngrab(wxCommandEvent& event)
{
    if (!m_currentObject || !m_currentObject->isBitmap()) {
        wxMessageBox("Only bitmap objects can be ungrabbed!", "", wxOK | wxICON_INFORMATION);
        return;
    }
    wxImage image;
    if (!m_currentObject->getBitmap().toWxImage(image, m_currentPalette)) {
        wxMessageBox("Failed to convert bitmap to image.", "Error", wxOK | wxICON_ERROR);
        return;
    }
    m_loadedImage = std::make_shared<wxImage>(image); // Copy the image
    std::string origPath = m_currentObject->getProperty('ORIG');
    if (!origPath.empty()) {
        m_loadedBitmapPath = wxString::FromUTF8(origPath);
    } else {
        wxString name = wxString::FromUTF8(m_currentObject->name);
        if (name.IsEmpty())
            name = "ungrabbed_bitmap";
        m_loadedBitmapPath = name;
    }
    SetStatusText("Bitmap copied to loaded image.");
}

void MyFrame::OnViewAlpha(wxCommandEvent& event)
{
    if (!IsBitmapOrRLESpriteSelected("view"))
        return;
    const BitmapData& bmpData = m_currentObject->getBitmap();
    int width = bmpData.width;
    int height = bmpData.height;
    if (!bmpData.hasAlphaData() || width <= 0 || height <= 0) {
        wxMessageBox("There is no alpha channel in this image", "Sorry", wxOK | wxICON_INFORMATION, this);
        return;
    }
    wxImage alphaImg = bmpData.getAlphaDataAsImage();
    BitmapPreviewDialog dlg(this, "Alpha Channel", alphaImg);
    dlg.ShowModal();
}

void MyFrame::OnDeleteAlpha(wxCommandEvent& event)
{
    if (!IsBitmapOrRLESpriteSelected("delete"))
        return;
    BitmapData& bmpData = const_cast<BitmapData&>(m_currentObject->getBitmap());
    bmpData.deleteAlphaData();
    UpdateObjectPreview();
    SetModified(true);
    wxMessageBox("Success: alpha channel moved to /dev/null", "Cool", wxOK | wxICON_INFORMATION, this);
}

void MyFrame::OnImportAlpha(wxCommandEvent& event)
{
    if (!IsBitmapOrRLESpriteSelected("import"))
        return;
    BitmapData& bmpData = const_cast<BitmapData&>(m_currentObject->getBitmap());
    int width = bmpData.width;
    int height = bmpData.height;
    if (width <= 0 || height <= 0) {
        wxMessageBox("Current image has invalid dimensions.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    wxFileDialog openFileDialog(this, "Open Image File for Alpha Channel", "", "",
                               "Image files (*.bmp;*.png;*.jpg;*.jpeg;*.tga;*.pcx)|*.bmp;*.png;*.jpg;*.jpeg;*.tga;*.pcx",
                               wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;
    wxString path = openFileDialog.GetPath();
    wxImage image;
    if (!BitmapData::readFileToWxImage(path, image)) {
        wxMessageBox("Failed to load image file: " + path, "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    if (!bmpData.importAlphaDataFromImage(image)) {
        wxMessageBox("Failed to import alpha channel from image.", "Error", wxOK | wxICON_ERROR, this);
        return;
    }
    UpdateObjectPreview();
    SetModified(true);
    SetStatusText("Alpha channel imported successfully!");
    if (bmpData.hasAlphaData()) {
        wxImage alphaImg = bmpData.getAlphaDataAsImage();
        BitmapPreviewDialog dlg(this, "Imported Alpha Channel", alphaImg);
        dlg.ShowModal();
    }
}

void MyFrame::OnExportAlpha(wxCommandEvent& event)
{
    if (!IsBitmapOrRLESpriteSelected("export"))
        return;
    const BitmapData& bmpData = m_currentObject->getBitmap();
    if (!bmpData.hasAlphaData()) {
        wxMessageBox("There is no alpha channel in this image", "No Alpha Channel", wxOK | wxICON_INFORMATION, this);
        return;
    }
    wxString defaultName = wxString::FromUTF8(m_currentObject->name);
    if (defaultName.IsEmpty()) {
        defaultName = "alpha_channel";
    }
    defaultName += "_alpha";
    wxFileDialog saveFileDialog(this, "Export Alpha Channel", "", defaultName,
                               "PNG files (*.png)|*.png|BMP files (*.bmp)|*.bmp|JPEG files (*.jpg)|*.jpg|PCX files (*.pcx)|*.pcx|TGA files (*.tga)|*.tga|All files (*.*)|*.*",
                               wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;
    wxString path = saveFileDialog.GetPath();
    wxImage alphaImage = bmpData.getAlphaDataAsImage();
    if (!alphaImage.IsOk()) {
        wxMessageBox("Failed to generate alpha channel image", "Export Error", wxOK | wxICON_ERROR, this);
        return;
    }
    wxString ext = path.AfterLast('.').Lower();
    bool success = false;
    if (ext == "png") {
        success = alphaImage.SaveFile(path, wxBITMAP_TYPE_PNG);
    } 
    else if (ext == "bmp") {
        success = alphaImage.SaveFile(path, wxBITMAP_TYPE_BMP);
    }
    else if (ext == "jpg" || ext == "jpeg") {
        success = alphaImage.SaveFile(path, wxBITMAP_TYPE_JPEG);
    }
    else if (ext == "pcx") {
        success = BitmapData::WritePCXFile(path, alphaImage);
    }
    else if (ext == "tga") {
        success = alphaImage.SaveFile(path, wxBITMAP_TYPE_TGA);
    }
    else {
        if (!path.AfterLast('.').IsEmpty()) {
            success = alphaImage.SaveFile(path, wxBITMAP_TYPE_PNG);
        } else {
            path += ".png";
            success = alphaImage.SaveFile(path, wxBITMAP_TYPE_PNG);
        }
    }
    if (success) {
        SetStatusText("Successfully exported alpha channel: " + path);
        logInfo("Exported alpha channel from " + m_currentObject->name + " to " + path.ToStdString());
    } else {
        wxMessageBox("Failed to export alpha channel to file: " + path, "Export Error", wxOK | wxICON_ERROR, this);
        logError("Failed to export alpha channel from " + m_currentObject->name + " to " + path.ToStdString());
    }
}

bool MyFrame::IsBitmapOrRLESpriteSelected(const wxString& action, bool showMessage) const {
    if (!m_currentObject ||
        !(m_currentObject->typeID == ObjectType::DAT_BITMAP || m_currentObject->typeID == ObjectType::DAT_RLE_SPRITE)) {
        if (showMessage) {
            wxString msg;
            if (action == "view")
                msg = "You must select a single bitmap or RLE sprite object before you can view the alpha channel";
            else if (action == "delete")
                msg = "You must select a single bitmap or RLE sprite object before you can delete the alpha channel";
            else if (action == "import")
                msg = "You must select a single bitmap or RLE sprite object before you can import the alpha channel";
            else if (action == "export")
                msg = "You must select a single bitmap or RLE sprite object before you can export the alpha channel";
            else
                msg = "You must select a single bitmap or RLE sprite object before you can perform this action";
            wxMessageBox(msg, action == "delete" ? "Sorry" : "Sorry", wxOK | wxICON_INFORMATION);
        }
        return false;
    }
    return true;
}

bool MyFrame::GenerateHeaderFile(const std::string& datFilePath) {
    // Check if header text is empty
    wxString headerText = m_headerText->GetValue().Trim();
    if (headerText.IsEmpty()) {
        return true; // No header file to generate
    }

    // Get prefix text
    wxString prefixText = m_prefixText->GetValue().Trim();
    if (prefixText.IsEmpty()) {
        prefixText = "obj"; // Default prefix if none specified
    }
    
    // Sanitize prefix text for C preprocessor
    std::string sanitizedPrefix = prefixText.ToStdString();
    for (char& c : sanitizedPrefix) {
        if (!isalnum(c) && c != '_') {
            c = '_';
        }
    }

    // Create header file path
    wxFileName datFileName(wxString::FromUTF8(datFilePath));
    wxString headerFilePath;
    
    if (datFileName.IsAbsolute()) {
        headerFilePath = datFileName.GetPath() + wxFileName::GetPathSeparator() + headerText + ".h";
    } else {
        // For relative paths, create header file in current directory
        headerFilePath = headerText + ".h";
    }

    // Get current date and time
    wxDateTime now = wxDateTime::Now();
    wxString dateStr = now.Format("%a %b %d %H:%M:%S %Y");

    // Create header file content
    std::ofstream headerFile(headerFilePath.ToStdString());
    if (!headerFile.is_open()) {
        logError("Failed to create header file: " + headerFilePath.ToStdString());
        return false;
    }

    // Check if we have any objects to process
    if (m_objects.empty()) {
        logWarning("No objects to generate header file for");
        headerFile.close();
        return true;
    }

    // Write header file content
    headerFile << "/* Allegro datafile object indexes, produced by " << GrabberName << " v" << GrabberVersion << " */\n";
    headerFile << "/* Datafile: " << datFilePath << " */\n";
    headerFile << "/* Date: " << dateStr.ToStdString() << " */\n";
    headerFile << "/* Do not hand edit! */\n\n";

    // Generate object definitions
    int index = 0;
    for (const auto& obj : m_objects) {
        std::string objName = obj->getProperty('NAME');
        objName = sanitizedPrefix + "_" + objName;

        // Sanitize object name for C preprocessor (replace spaces and special chars with underscores)
        std::string sanitizedName = objName;
        for (char& c : sanitizedName) {
            if (!isalnum(c) && c != '_') {
                c = '_';
            }
        }

        // Get object type string
        std::string objType = DataParser::ConvertIDToString(obj->typeID);
        
        // Write the define line with proper alignment
        headerFile << "#define " << sanitizedName;
        
        // Add spaces for alignment (assuming max name length of 30 characters)
        int spacesNeeded = 30 - static_cast<int>(sanitizedName.length());
        for (int i = 0; i < spacesNeeded; ++i) {
            headerFile << " ";
        }
        
        headerFile << index;
        
        // Add spaces for index alignment
        if (index < 10) {
            headerFile << "        ";
        } else if (index < 100) {
            headerFile << "       ";
        } else {
            headerFile << "      ";
        }
        
        headerFile << "/* " << objType << "  */\n";
        index++;
    }

    // Add count define
    headerFile << "#define " << sanitizedPrefix << "_COUNT";
    
    // Add spaces for alignment
    int spacesNeeded = 30 - static_cast<int>(sanitizedPrefix.length()) - 6; // 6 for "_COUNT"
    for (int i = 0; i < spacesNeeded; ++i) {
        headerFile << " ";
    }
    
    headerFile << index;
    
    // Add spaces for index alignment
    if (index < 10) {
        headerFile << "        ";
    } else if (index < 100) {
        headerFile << "       ";
    } else {
        headerFile << "      ";
    }
    
    headerFile << "\n";

    headerFile.close();
    
    logInfo("Generated header file: " + headerFilePath.ToStdString());
    return true;
}

void MyFrame::OnHelpSystem(wxCommandEvent& event)
{
    // OS
    wxString osDesc = wxGetOsDescription();

    // CPU
    wxString cpuDesc;
#if defined(__WXMSW__)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    cpuDesc = wxString::Format("%d cores, arch %d", (int)sysInfo.dwNumberOfProcessors, (int)sysInfo.wProcessorArchitecture);
#elif defined(__APPLE__)
    int nm[2];
    size_t len = 4;
    uint32_t count;
    nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
    sysctl(nm, 2, &count, &len, NULL, 0);
    if(count < 1) {
        nm[1] = HW_NCPU;
        sysctl(nm, 2, &count, &len, NULL, 0);
        if(count < 1) count = 1;
    }
    char model[256];
    size_t modelLen = sizeof(model);
    sysctlbyname("machdep.cpu.brand_string", &model, &modelLen, NULL, 0);
    cpuDesc = wxString::Format("%u cores, %s", count, wxString::FromUTF8(model));
#elif defined(__linux__) || defined(__unix__)
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);
    wxString model;
    std::ifstream cpuinfo("/proc/cpuinfo");
    wxString line;
    while (std::getline(cpuinfo, line.ToStdString())) {
        if (line.StartsWith("model name")) {
            model = line.AfterFirst(':').Trim();
            break;
        }
    }
    cpuDesc = wxString::Format("%ld cores, %s", nprocs, model);
#else
    cpuDesc = "Unknown";
#endif

    // Video
    wxString videoDesc;
    wxDisplay display;
    wxRect rect = display.GetGeometry();
    int depth = display.GetCurrentMode().GetDepth();
    videoDesc = wxString::Format("%dx%d, %d bpp", rect.width, rect.height, depth);

    // Audio
    wxString audioDesc;
#if defined(__WXMSW__)
    WAVEOUTCAPS woc;
    if (waveOutGetDevCaps(WAVE_MAPPER, &woc, sizeof(woc)) == MMSYSERR_NOERROR) {
        audioDesc = wxString(woc.szPname, wxStrlen(woc.szPname));
    } else {
        audioDesc = "Unknown";
    }
#else
    audioDesc = "Unknown";
#endif

    // MIDI
    wxString midiDesc;
#if defined(__WXMSW__)
    MIDIOUTCAPS moc;
    if (midiOutGetDevCaps(MIDI_MAPPER, &moc, sizeof(moc)) == MMSYSERR_NOERROR) {
        midiDesc = wxString(moc.szPname, wxStrlen(moc.szPname));
    } else {
        midiDesc = "Unknown";
    }
#else
    midiDesc = "Unknown";
#endif

    wxString info;
    info << "System Status\n";
    info << "==============\n\n";
    info << "Platform: " << osDesc << "\n";
    info << "CPU: " << cpuDesc << "\n\n";
    info << "Video: " << videoDesc << "\n\n";
    info << "Audio: " << audioDesc << "\n\n";
    info << "MIDI: " << midiDesc << "\n\n";
    info << "Object plugins:\n\n";
    info << " BMP  - Bitmap\n";
    info << "   Import: bmp;png;jpeg;tga;pcx\n";
    info << "   Export: bmp;png;jpeg;pcx\n\n";
    info << " CMP  - Compiled sprite\n";
    info << "   Import: bmp;png;jpeg;tga;pcx\n";
    info << "   Export: bmp;png;jpeg;pcx\n\n";
    info << " XCMP - Compiled X-sprite\n";
    info << "   Import: bmp;png;jpeg;tga;pcx\n";
    info << "   Export: bmp;png;jpeg;pcx\n\n";
    info << " FILE - Datafile\n";
    info << "   Import: dat\n";
    info << "   Export: dat\n\n";
    info << " FLIC - FLI/FLC animation\n";
    info << "   Import: fli;flc\n";
    info << "   Export: fli;flc\n\n";
    info << " FONT - Font\n";
    info << "   Import: bmp;pcx;tga;fnt\n";
    info << "   Export: bmp;pcx;tga;fnt\n\n";
    info << " MIDI - MIDI file\n";
    info << "   Import: mid\n";
    info << "   Export: mid\n\n";
    info << " PAL  - Palette\n";
    info << "   Import: bmp;png;jpeg;tga;pcx\n";
    info << "   Export: bmp;png;jpeg;pcx\n\n";
    info << " RLE  - RLE sprite\n";
    info << "   Import: bmp;png;jpeg;tga;pcx\n";
    info << "   Export: bmp;png;jpeg;pcx\n\n";
    info << " SAMP - Sample\n";
    info << "   Import: wav\n";
    info << "   Export: wav\n";

    wxDialog dlg(this, wxID_ANY, "System Status", wxDefaultPosition, wxSize(700, 600), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    wxTextCtrl* textCtrl = new wxTextCtrl(&dlg, wxID_ANY, info, wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP);
    wxFont monoFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    textCtrl->SetFont(monoFont);
    sizer->Add(textCtrl, 1, wxEXPAND | wxALL, 10);
    wxButton* exitBtn = new wxButton(&dlg, wxID_OK, "Exit");
    sizer->Add(exitBtn, 0, wxALIGN_CENTER | wxBOTTOM, 10);
    dlg.SetSizer(sizer);
    dlg.SetMinSize(wxSize(700, 600));
    dlg.CentreOnParent();
    dlg.ShowModal();
}

bool MyFrame::GrabNested(wxString& path) {
    // Prompt user to select a .dat file
    wxFileDialog openFileDialog(this, "Open DAT file", "", "", "DAT files (*.dat)|*.dat", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return false;
    path = openFileDialog.GetPath();
    std::string pathStr = path.ToStdString();
    // Load objects from the selected .dat file
    std::vector<std::shared_ptr<DataParser::DataObject>> nestedObjects;
    std::string password = m_grabberInfo.GetPassword();
    auto [success, _] = DataParser::LoadPackfile(pathStr, nestedObjects, password);
    if (!success) {
        wxMessageBox("Failed to load .dat file: " + path, "Error", wxOK | wxICON_ERROR);
        return false;
    }
    // Remove info object(s) from the list of subobjects
    nestedObjects.erase(
        std::remove_if(nestedObjects.begin(), nestedObjects.end(), [](const std::shared_ptr<DataParser::DataObject>& obj) {
            return obj && obj->typeID == ObjectType::DAT_INFO;
        }),
        nestedObjects.end()
    );
    // Replace the nested objects in the current object
    DataParser::DataObject& obj = const_cast<DataParser::DataObject&>(*m_currentObject);
    obj.data = nestedObjects;
    UpdateObjectAfterGrab(pathStr, "datafile");
    RefreshTreeDisplay();
    UpdateObjectPreview();
    logDebug("Successfully grabbed nested objects from " + path.ToStdString());
    return true;
}

// Add after other event handlers:
void MyFrame::OnNewOggAudio(wxCommandEvent& event) {
    auto obj = std::make_shared<DataParser::DataObject>(DataParser::createSampleObject(ObjectType::DAT_OGG));
    m_objects.push_back(obj);
    RefreshTreeDisplay();
    SetModified(true);
    UpdateObjectPreview();
}
