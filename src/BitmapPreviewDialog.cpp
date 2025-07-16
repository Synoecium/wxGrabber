#include "../include/BitmapPreviewDialog.h"
#include <wx/scrolwin.h>
#include <wx/statbmp.h>
#include <wx/sizer.h>

BitmapPreviewDialog::BitmapPreviewDialog(wxWindow* parent, const wxString& title, const wxImage& image)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 600),
              wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // Create a scrolled window for the image
    wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
    scrolledWindow->SetScrollRate(10, 10);
    
    // Create a static bitmap to display the image
    wxStaticBitmap* bitmap = new wxStaticBitmap(scrolledWindow, wxID_ANY, 
                                              wxBitmap(image));
    
    // Create sizer for the scrolled window
    wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);
    scrolledSizer->Add(bitmap, 1, wxEXPAND | wxALL, 5);
    scrolledWindow->SetSizer(scrolledSizer);
    
    // Create main sizer for the dialog
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->Add(scrolledWindow, 1, wxEXPAND | wxALL, 5);
    SetSizer(mainSizer);

    // Bind events for closing on key press or mouse click to all windows
    BindEventsToWindow(this);
}

void BitmapPreviewDialog::BindEventsToWindow(wxWindow* window) {
    // Bind events to this window
    window->Bind(wxEVT_CHAR_HOOK, &BitmapPreviewDialog::OnCharHook, this);
    window->Bind(wxEVT_LEFT_DOWN, &BitmapPreviewDialog::OnMouseClick, this);
    window->Bind(wxEVT_RIGHT_DOWN, &BitmapPreviewDialog::OnMouseClick, this);
    window->Bind(wxEVT_MIDDLE_DOWN, &BitmapPreviewDialog::OnMouseClick, this);
    
    // Recursively bind to all children
    for (wxWindowList::iterator it = window->GetChildren().begin();
         it != window->GetChildren().end(); ++it) {
        BindEventsToWindow(*it);
    }
}

void BitmapPreviewDialog::OnCharHook(wxKeyEvent& event) {
    EndModal(wxID_OK);
}

void BitmapPreviewDialog::OnMouseClick(wxMouseEvent& event) {
    EndModal(wxID_OK);
}

BEGIN_EVENT_TABLE(BitmapPreviewDialog, wxDialog)
    EVT_CHAR_HOOK(BitmapPreviewDialog::OnCharHook)
    EVT_LEFT_DOWN(BitmapPreviewDialog::OnMouseClick)
    EVT_RIGHT_DOWN(BitmapPreviewDialog::OnMouseClick)
    EVT_MIDDLE_DOWN(BitmapPreviewDialog::OnMouseClick)
END_EVENT_TABLE() 