#pragma once

#include <wx/dialog.h>
#include <wx/image.h>

// Custom dialog class for bitmap preview that closes on any key press or mouse click
class BitmapPreviewDialog : public wxDialog {
public:
    BitmapPreviewDialog(wxWindow* parent, const wxString& title, const wxImage& image);

private:
    // Helper function to recursively bind events to a window and all its children
    void BindEventsToWindow(wxWindow* window);

    void OnCharHook(wxKeyEvent& event);
    void OnMouseClick(wxMouseEvent& event);

    DECLARE_EVENT_TABLE()
}; 