#include "../include/GrabPreviewDialog.h"
#include <wx/sizer.h>
#include <wx/button.h>

BEGIN_EVENT_TABLE(GrabPreviewDialog, wxDialog)
END_EVENT_TABLE()

GrabPreviewDialog::GrabPreviewDialog(wxWindow* parent, const wxImage& image, const wxString& title, long xGridSnap, long yGridSnap, SelectionMode mode)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    // Create main sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create scrolled window
    wxScrolledWindow* scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
    scrolledWindow->SetScrollRate(10, 10);
    
    // Create status text with monospace font
    wxFont monoFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    m_statusText = new wxStaticText(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
    m_statusText->SetFont(monoFont);
    
    // Create image panel with grid snapping and selection mode
    m_imagePanel = new ImagePanel(scrolledWindow, image, m_statusText, xGridSnap, yGridSnap, static_cast<ImagePanel::SelectionMode>(mode));
    
    // Create sizer for scrolled window
    wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);
    scrolledSizer->Add(m_imagePanel, 1, wxEXPAND | wxALL, 5);
    scrolledWindow->SetSizer(scrolledSizer);
    
    // Set virtual size to match the image size
    scrolledWindow->SetVirtualSize(image.GetWidth(), image.GetHeight());
    
    // Add scrolled window to main sizer
    mainSizer->Add(scrolledWindow, 1, wxEXPAND | wxALL, 5);
    
    // Add status text below the scrolled window
    mainSizer->Add(m_statusText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Create button sizer
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okButton = new wxButton(this, wxID_OK, "OK");
    wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
    buttonSizer->Add(okButton, 0, wxALL, 5);
    buttonSizer->Add(cancelButton, 0, wxALL, 5);
    
    // Add button sizer to main sizer
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxBOTTOM, 5);
    
    // Set the dialog's sizer
    SetSizer(mainSizer);
    
    // Set minimum size to prevent dialog from being too small
    SetMinSize(wxSize(400, 300));
    
    // Center the dialog on the parent window
    CentreOnParent();
} 