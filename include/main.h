#ifndef MAIN_H
#define MAIN_H

#define wxUSE_UNICODE 1

// For compilers that support precompilation, includes "wx/wx.h".
//#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/treectrl.h>
#include <wx/statbmp.h>
#include <wx/mstream.h>
#include <wx/listctrl.h>
#include <wx/dialog.h>
#include <wx/statline.h>
#include <wx/mediactrl.h>
#include <wx/timer.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <string>
#include <cmath>
#include "../include/DataParser.h"
#include "../include/UnitTests.h"
#include "GrabberInfo.h"
#include <wx/filename.h>
#include <wx/sound.h>
#include <memory>
#include <wx/slider.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/advprops.h>
#include "VideoDataPanel.h"
#include "AudioPlaybackControl.h"

// Custom streambuf that writes to both file and original stream
class TeeStreamBuf : public std::streambuf {
public:
    TeeStreamBuf(std::streambuf* buf1, std::streambuf* buf2) 
        : buffer1(buf1), buffer2(buf2) {}

protected:
    virtual int overflow(int c) {
        if (c == EOF) return !EOF;
        int r1 = buffer1->sputc(c);
        int r2 = buffer2->sputc(c);
        return r1 == EOF || r2 == EOF ? EOF : c;
    }

    virtual int sync() {
        int r1 = buffer1->pubsync();
        int r2 = buffer2->pubsync();
        return r1 == 0 && r2 == 0 ? 0 : -1;
    }

private:
    std::streambuf* buffer1;
    std::streambuf* buffer2;
};

// Custom ostream that uses TeeStreamBuf
class TeeStream : public std::ostream {
public:
    TeeStream(std::ostream& stream1, std::ostream& stream2)
        : std::ostream(&teeBuffer), 
          teeBuffer(stream1.rdbuf(), stream2.rdbuf()) {}

private:
    TeeStreamBuf teeBuffer;
};

// Property edit dialog
class PropertyEditDialog : public wxDialog {
public:
    PropertyEditDialog(wxWindow* parent, const wxString& title, const wxString& propId, const wxString& value);
    wxString GetValue() const { return m_value->GetValue(); }
private:
    wxTextCtrl* m_value;
    wxString m_originalValue;  // Store the original value to detect changes
};

class MyApp : public wxApp
{
public:
    virtual bool OnInit() override;
    virtual int OnExit() override;
private:
    bool InitializeLogging();
    std::ofstream m_logFile;
    std::streambuf* m_oldCout;
    std::streambuf* m_oldCerr;
};

class MyFrame : public wxFrame
{
public:
    MyFrame();
    virtual ~MyFrame();

    const int HeaderLabelsWidth = 80;

private:
    void OnAbout(wxCommandEvent& event);
    void OnLoad(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnSaveAs(wxCommandEvent& event);
    void OnSaveStripped(wxCommandEvent& event);
    void OnMerge(wxCommandEvent& event);
    void OnUpdate(wxCommandEvent& event);
    void OnUpdateSelection(wxCommandEvent& event);
    void OnForceUpdate(wxCommandEvent& event);
    void OnForceUpdateSelection(wxCommandEvent& event);
    void OnGrab(wxCommandEvent& event);
    void OnExport(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnReadBitmap(wxCommandEvent& event);
    void OnViewBitmap(wxCommandEvent& event);
    void OnGrabFromGrid(wxCommandEvent& event);
    void OnReadAlpha(wxCommandEvent& event);
    void ShowImageDialog(const wxString& title);
    void OnQuit(wxCommandEvent& event);
    void OnTreeSelectionChanged(wxTreeEvent& event);
    void OnTreeItemActivated(wxTreeEvent& event);
    void OnTreeBeginDrag(wxTreeEvent& event);
    void OnTreeEndDrag(wxTreeEvent& event);
    void OnPropertyActivated(wxListEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnHelp(wxCommandEvent& event);
    void UpdatePropertyList(std::shared_ptr<DataParser::DataObject> obj);
    void OnZoomChange(wxCommandEvent& event);
    void OnPackModeChanged(wxCommandEvent& event);
    void OnGridChange(wxCommandEvent& event);
    void OnHeaderChange(wxCommandEvent& event);
    void OnPasswordChange(wxCommandEvent& event);
    void OnIndexObjects(wxCommandEvent& event);
    void RefreshTreeDisplay();
    void UpdateImageDisplay(const wxImage& image, double zoomLevel, bool addBorder = false);
    void OnNotImplemented(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void UpdateUIFromGrabberInfo();
    void OnNew(wxCommandEvent& event);
    
    // Audio/video playback
    void OnPlayPauseAV(wxCommandEvent& event);
    void UpdatePreviewControls(std::shared_ptr<DataParser::DataObject> obj);
    void StopAVPlayback();
    void ResetAVPlayback();

    bool HasUnsavedChanges() const;
    void ResetToDefaults();
    void SetModified(bool modified);
    bool IsModified() const { return m_isModified; }

    wxTreeCtrl* m_tree;
    wxListCtrl* m_details;
    wxStaticText* m_infoText;        // General info/preview text
    wxStaticText* m_paletteInfoText; // Specific text for palette preview
    wxStaticBitmap* m_imagePreview;
    wxScrolledWindow* m_imagePreviewPanel;
    wxSlider* m_zoomSlider;
    wxStaticText* m_zoomLabel;
    std::vector<std::shared_ptr<DataParser::DataObject>> m_objects;
    std::shared_ptr<DataParser::DataObject> m_currentObject;
    //std::vector<DataParser::DataObject> m_objects;
    //const DataParser::DataObject* m_currentObject;
    wxTreeItemId m_currentTreeItem;
    std::string m_currentFilePath;
    bool m_showIndices;
    bool m_isModified;  // Track if there are unsaved changes

    wxTextCtrl* m_editingText;
    wxTextCtrl* m_headerText;
    wxTextCtrl* m_prefixText;
    wxTextCtrl* m_passwordText;
    wxTextCtrl* m_xGridText;
    wxTextCtrl* m_yGridText;
    wxRadioBox* m_compressionBox;
    wxButton* m_toggleHeaderButton; // Show/hide additional header
    bool m_showAdditionalHeader = true;
    wxBitmap m_arrowDownBmp;
    wxBitmap m_arrowUpBmp;
    GrabberInfo m_grabberInfo;

    AVControlInterface* m_avControl;
    wxSlider* m_audioPositionSlider;
    wxButton* m_playButton;
    wxPanel* m_audioPanel;

    // Audio playback members
    std::unique_ptr<AudioPlaybackControl> m_audioControl;

    // Icon management
    wxBitmap GetPlayIcon();
    wxBitmap GetPauseIcon();
    void UpdatePlayButtonIcon(bool isPlaying);

    wxSlider* m_AVSlider;
    wxStaticText* m_timeLabel;  // Label to show current position and duration
    
    // Shared pointer for storing loaded image
    std::shared_ptr<wxImage> m_loadedImage;
    
    // Path to the currently loaded bitmap
    wxString m_loadedBitmapPath;
    std::vector<uint8_t> m_currentPalette;
    std::vector<uint8_t> m_currentAlphaChannel;  // Store the loaded alpha channel
    
    // Drag and drop support
    wxTreeItemId m_draggedItem;
    std::shared_ptr<DataParser::DataObject> m_draggedObject;
    
    void OnSliderDrag(wxScrollEvent& event);
    void OnSliderRelease(wxScrollEvent& event);
    void UpdateSliderPosition();
    void GenerateObjectNames(std::vector<std::shared_ptr<DataParser::DataObject>>& objects);
    void UpdateObjects(std::vector<std::shared_ptr<DataParser::DataObject>>& objects, bool ForceUpdate = false);

    // Helper functions for object menu commands
    bool ValidateObjectSelection(const wxString& operation, bool requireBitmap = false, bool requireAudio = false, bool requireFont = false, bool requireVideo = false);
    void UpdateObjectPreview();
    
    // Grab helper methods
    bool GrabBitmap(wxString& path);
    bool GrabAudio(wxString& path);
    bool GrabFont(wxString& path);
    bool GrabVideo(wxString& path);
    bool GrabRawBinary(wxString& path);
    bool GrabNested(wxString& path);
    void UpdateObjectAfterGrab(const std::string& path, const std::string& objectType);

    void OnShowGridComplete(wxCommandEvent& event);
    void OnMoveUp(wxCommandEvent& event);
    void OnMoveDown(wxCommandEvent& event);
    void OnReplaceWithBitmap(wxCommandEvent& event);
    void OnReplaceWithFont(wxCommandEvent& event);
    void OnReplaceWithCompiledSprite(wxCommandEvent& event);
    void OnReplaceWithXSprite(wxCommandEvent& event);
    void OnReplaceWithDatafile(wxCommandEvent& event);
    void OnReplaceWithFLI(wxCommandEvent& event);
    void OnReplaceWithMIDI(wxCommandEvent& event);
    void OnReplaceWithPalette(wxCommandEvent& event);
    void OnReplaceWithRLE(wxCommandEvent& event);
    void OnReplaceWithSample(wxCommandEvent& event);
    void OnReplaceWithOther(wxCommandEvent& event);
    void OnNewBitmap(wxCommandEvent& event);
    void OnNewRLE(wxCommandEvent& event);
    void OnNewCompiledSprite(wxCommandEvent& event);
    void OnNewXSprite(wxCommandEvent& event);
    void OnNewDatafile(wxCommandEvent& event);
    void OnNewFLI(wxCommandEvent& event);
    void OnNewPalette(wxCommandEvent& event);
    void OnNewSample(wxCommandEvent& event);
    void OnNewOther(wxCommandEvent& event);
    void OnNewFont(wxCommandEvent& event);
    void OnNewMIDI(wxCommandEvent& event);
    void OnRenameObject(wxCommandEvent& event);
    void OnSetProperty(wxCommandEvent& event);
    void OnAutocrop(wxCommandEvent& event);
    void OnBoxGrab(wxCommandEvent& event);
    void OnUngrab(wxCommandEvent& event);
    void OnViewAlpha(wxCommandEvent& event);
    void OnDeleteAlpha(wxCommandEvent& event);
    void OnImportAlpha(wxCommandEvent& event);
    void OnExportAlpha(wxCommandEvent& event);

    // Depth change event handlers
    void OnDepth256(wxCommandEvent& event);
    void OnDepth15(wxCommandEvent& event);
    void OnDepth16(wxCommandEvent& event);
    void OnDepth24(wxCommandEvent& event);
    void OnDepth32(wxCommandEvent& event);

    // Helper to add a new object to the root or to the selected datafile's nested objects
    void addObjectToCurrentOrRoot(std::shared_ptr<DataParser::DataObject> obj);

    // Video preview and playback
    VideoDataPanel* m_videoPanel;
    wxScrolledWindow* m_videoPanelContainer;
    int m_playbackDuration;

    void AVPositionUpdateCallback(int currentPositionMs, int totalDurationMs);

    // Drag and drop helper methods
    bool IsItemInSubtree(wxTreeItemId targetItem, wxTreeItemId sourceItem);
    bool MoveObjectInTree(std::shared_ptr<DataParser::DataObject> sourceObj, std::shared_ptr<DataParser::DataObject> targetObj);
    
    // Reusable dialog for MoveTo operations
    std::pair<wxString, wxString> ShowMoveToDialog(const ObjectType objectType, const wxString& currentName, bool typeEditable = false);
    
    // Reusable dialog for custom object creation/editing
    std::pair<wxString, wxString> ShowCustomObjectDialog(const wxString& initialType, const wxString& initialName, const wxString& dialogTitle = "New Object");

    // Utility: Check if a single bitmap or RLE sprite object is selected for a given action
    bool IsBitmapOrRLESpriteSelected(const wxString& action, bool showMessage = true) const;

    // Generate header file with object indexes
    bool GenerateHeaderFile(const std::string& datFilePath);

    void OnChangeFilenameToRelative(wxCommandEvent& event);
    void OnChangeFilenameToAbsolute(wxCommandEvent& event);

    void ShowIgnoredObjectMessage();

    void OnChangeTypeToBitmap(wxCommandEvent& event);
    void OnChangeTypeToRLE(wxCommandEvent& event);
    void OnChangeTypeToCompiled(wxCommandEvent& event);
    void OnChangeTypeToXCompiled(wxCommandEvent& event);

    void OnShellEdit(wxCommandEvent& event);

    void OnSortObjects(wxCommandEvent& event);
    
    // Helper method for sorting objects by name
    void SortObjectsByName(std::vector<std::shared_ptr<DataParser::DataObject>>& objects);
    
    void OnStoreRelativeFilenames(wxCommandEvent& event);
    
    // Helper method to set ORIG property with correct format based on current setting
    void SetOrigPropertyWithFormat(std::shared_ptr<DataParser::DataObject> obj, const std::string& path);
    
    void OnDitherImages(wxCommandEvent& event);
    void OnPreserveTransparency(wxCommandEvent& event);

    void OnTreeItemMenu(wxTreeEvent& event);

    std::vector<std::shared_ptr<DataParser::DataObject>> GetSelectedObjects();
    void OnHelpSystem(wxCommandEvent& event);
    void OnNewOggAudio(wxCommandEvent& event);
};

enum
{
    ID_RESERVED = wxID_HIGHEST + 1,
    ID_OPEN_FILE,
    ID_SAVE_FILE,
    ID_BACKUP_DATAFILES,
    ID_INDEX_OBJECTS,
    ID_SAVE_STRIPPED,
    ID_SAVE_AS,
    ID_MERGE,
    ID_UPDATE,
    ID_UPDATE_SELECTION,
    ID_FORCE_UPDATE,
    ID_FORCE_UPDATE_SELECTION,
    ID_READ_BITMAP,
    ID_VIEW_BITMAP,
    ID_GRAB_FROM_GRID,
    ID_READ_ALPHA,
    ID_GRAB,
    ID_EXPORT,
    ID_DELETE,
    ID_MOVE_UP,
    ID_MOVE_DOWN,
    ID_MOVE_BITMAP,
    ID_MOVE_COMPILED_SPRITE,
    ID_MOVE_X_SPRITE,
    ID_MOVE_DATAFILE,
    ID_MOVE_FLI,
    ID_MOVE_FONT,
    ID_MOVE_MIDI,
    ID_MOVE_PALETTE,
    ID_MOVE_RLE,
    ID_MOVE_SAMPLE,
    ID_MOVE_OTHER,
    ID_REPLACE_BITMAP,
    ID_REPLACE_COMPILED_SPRITE,
    ID_REPLACE_X_SPRITE,
    ID_REPLACE_DATAFILE,
    ID_REPLACE_FLI,
    ID_REPLACE_FONT,
    ID_REPLACE_MIDI,
    ID_REPLACE_PALETTE,
    ID_REPLACE_RLE,
    ID_REPLACE_SAMPLE,
    ID_REPLACE_OTHER,
    ID_RENAME,
    ID_SET_PROPERTY,
    ID_AUTOCROP,
    ID_BOX_GRAB,
    ID_UNGRAB,
    ID_VIEW_ALPHA,
    ID_IMPORT_ALPHA,
    ID_EXPORT_ALPHA,
    ID_DELETE_ALPHA,
    ID_DEPTH_256,
    ID_DEPTH_15,
    ID_DEPTH_16,
    ID_DEPTH_24,
    ID_DEPTH_32,
    ID_FILENAME_RELATIVE,
    ID_FILENAME_ABSOLUTE,
    ID_TYPE_BITMAP,
    ID_TYPE_RLE,
    ID_TYPE_COMPILED,
    ID_TYPE_X_COMPILED,
    ID_SHELL_EDIT,
    ID_NEW_BITMAP,
    ID_NEW_COMPILED_SPRITE,
    ID_NEW_X_SPRITE,
    ID_NEW_DATAFILE,
    ID_NEW_FLI,
    ID_NEW_FONT,
    ID_NEW_MIDI,
    ID_NEW_PALETTE,
    ID_NEW_RLE,
    ID_NEW_SAMPLE,
    ID_NEW_OGG_AUDIO,
    ID_NEW_OTHER,
    ID_SORT_OBJECTS,
    ID_STORE_RELATIVE,
    ID_DITHER_IMAGES,
    ID_PRESERVE_TRANSPARENCY,
    ID_HELP_SYSTEM,
    ID_HELP_WORMS,
    ID_TREE_CTRL,
    ID_LIST_CTRL,
    ID_PLAY_AUDIOVIDEO,
    ID_AUDIOVIDEO_POSITION,
};

wxDECLARE_EVENT(wxEVT_SHOW_GRID_COMPLETE, wxCommandEvent);
#endif // MAIN_H
