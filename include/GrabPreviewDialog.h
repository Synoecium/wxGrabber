#pragma once

#include <wx/dialog.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>
#include <wx/image.h>
#include <wx/dcbuffer.h>
#include <wx/bitmap.h>
#include <wx/pen.h>
#include <wx/brush.h>
#include <wx/stattext.h>
#include "BitmapData.h"

// Custom panel class for drawing
class ImagePanel : public wxPanel {
public:
    enum class SelectionMode {
        Rectangle, // Click and drag to select a rectangle (grid snap)
        Point      // Click to select a point, then find region
    };
    ImagePanel(wxWindow* parent, const wxImage& image, wxStaticText* statusText, long xGridSnap = 0, long yGridSnap = 0, SelectionMode mode = SelectionMode::Rectangle)
        : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
        , m_image(image)
        , m_selectionRect(0, 0, 0, 0)
        , m_isDragging(false)
        , m_startPoint(0, 0)
        , m_statusText(statusText)
        , m_xGridSnap(xGridSnap)
        , m_yGridSnap(yGridSnap)
        , m_mode(mode)
        , m_selectedPoint(-1, -1)
        , m_imagePtr(std::make_shared<wxImage>(image))
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetSize(image.GetWidth(), image.GetHeight());
        
        // Bind events
        Bind(wxEVT_PAINT, &ImagePanel::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &ImagePanel::OnMouseDown, this);
        Bind(wxEVT_LEFT_UP, &ImagePanel::OnMouseUp, this);
        Bind(wxEVT_MOTION, &ImagePanel::OnMouseMove, this);
        Bind(wxEVT_MOUSE_CAPTURE_LOST, &ImagePanel::OnCaptureLost, this);
        
        // Update initial status
        UpdateStatusText();
    }
    
    wxRect GetSelection() const { return m_selectionRect; }
    wxPoint GetSelectedPoint() const { return m_selectedPoint; } // Only valid in Point mode
    void SetSelectionMode(SelectionMode mode) { m_mode = mode; }
    SelectionMode GetSelectionMode() const { return m_mode; }
    
private:
    void UpdateStatusText() {
        if (m_statusText) {
            if (m_mode == SelectionMode::Point) {
                if (m_selectedPoint.x >= 0 && m_selectedPoint.y >= 0) {
                    m_statusText->SetLabel(wxString::Format(
                        "Point: (%d,%d) Region: (%d,%d) %dx%d",
                        m_selectedPoint.x, m_selectedPoint.y,
                        m_selectionRect.x, m_selectionRect.y,
                        m_selectionRect.width, m_selectionRect.height
                    ));
                } else {
                    m_statusText->SetLabel("Click a point inside the image to select a region");
                }
            } else {
                if (m_selectionRect.width > 0 && m_selectionRect.height > 0) {
                    m_statusText->SetLabel(wxString::Format(
                        "Selection: (%d,%d) %dx%d",
                        m_selectionRect.x, m_selectionRect.y,
                        m_selectionRect.width, m_selectionRect.height
                    ));
                } else {
                    m_statusText->SetLabel("No selection - click and drag to select a region");
                }
            }
        }
    }
    
    // Helper function to constrain a point to image boundaries
    wxPoint ConstrainPoint(const wxPoint& point) const {
        return wxPoint(
            std::clamp(point.x, 0, m_image.GetWidth() - 1),
            std::clamp(point.y, 0, m_image.GetHeight() - 1)
        );
    }
    
    // Helper function to constrain a single coordinate
    int ConstrainCoordinate(int value, int min, int max) const {
        return std::clamp(value, min, max);
    }

    void SnapRectToGrid(wxRect& rect) const {
        if (m_xGridSnap > 1) {
            int x1 = (rect.x / m_xGridSnap) * m_xGridSnap;
            int x2 = ((rect.x + rect.width - 1) / m_xGridSnap + 1) * m_xGridSnap;
            rect.x = x1;
            rect.width = std::max(1, x2 - x1);
        }
        if (m_yGridSnap > 1) {
            int y1 = (rect.y / m_yGridSnap) * m_yGridSnap;
            int y2 = ((rect.y + rect.height - 1) / m_yGridSnap + 1) * m_yGridSnap;
            rect.y = y1;
            rect.height = std::max(1, y2 - y1);
        }
    }
    
    void OnPaint(wxPaintEvent& event) {
        wxBufferedPaintDC dc(this);
        
        // Clear the background
        dc.SetBackground(*wxWHITE_BRUSH);
        dc.Clear();
        
        // Draw the image
        dc.DrawBitmap(wxBitmap(m_image), 0, 0);
        
        // Draw selection rectangle if it exists
        if (m_selectionRect.width > 0 && m_selectionRect.height > 0) {
            // Create a dotted pen (2 pixels wide, black, dotted style)
            wxPen dottedPen(*wxBLACK, 2, wxPENSTYLE_DOT);
            dc.SetPen(dottedPen);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawRectangle(m_selectionRect);
        }
    }
    
    void OnMouseDown(wxMouseEvent& event) {
        wxPoint pos = ConstrainPoint(event.GetPosition());
        if (pos.x >= 0 && pos.x < m_image.GetWidth() &&
            pos.y >= 0 && pos.y < m_image.GetHeight()) {
            if (m_mode == SelectionMode::Point) {
                m_selectedPoint = pos;
                // Call BitmapData::findCharacterRegion to set m_selectionRect
                m_selectionRect = BitmapData::findCharacterRegion(m_imagePtr, pos.x, pos.y);
                UpdateStatusText();
                Refresh();
            } else {
                m_isDragging = true;
                m_startPoint = pos;
                m_selectionRect = wxRect(m_startPoint, wxSize(0, 0));
                CaptureMouse();
                UpdateStatusText();
                Refresh();
            }
        }
    }
    
    void OnMouseUp(wxMouseEvent& event) {
        if (m_mode == SelectionMode::Rectangle && m_isDragging) {
            m_isDragging = false;
            if (HasCapture()) {
                ReleaseMouse();
            }
            wxPoint currentPoint = event.GetPosition();
            int x = ConstrainCoordinate(currentPoint.x, 0, m_image.GetWidth() - 1);
            int y = ConstrainCoordinate(currentPoint.y, 0, m_image.GetHeight() - 1);
            int left = std::min(m_startPoint.x, x);
            int right = std::max(m_startPoint.x, x);
            int top = std::min(m_startPoint.y, y);
            int bottom = std::max(m_startPoint.y, y);
            left = ConstrainCoordinate(left, 0, m_image.GetWidth() - 1);
            right = ConstrainCoordinate(right, 0, m_image.GetWidth() - 1);
            top = ConstrainCoordinate(top, 0, m_image.GetHeight() - 1);
            bottom = ConstrainCoordinate(bottom, 0, m_image.GetHeight() - 1);
            m_selectionRect = wxRect(left, top, right - left + 1, bottom - top + 1);
            SnapRectToGrid(m_selectionRect);
            UpdateStatusText();
            Refresh();
        }
    }
    
    void OnMouseMove(wxMouseEvent& event) {
        if (m_mode == SelectionMode::Rectangle && m_isDragging) {
            wxPoint currentPoint = event.GetPosition();
            int x = ConstrainCoordinate(currentPoint.x, 0, m_image.GetWidth() - 1);
            int y = ConstrainCoordinate(currentPoint.y, 0, m_image.GetHeight() - 1);
            int left = std::min(m_startPoint.x, x);
            int right = std::max(m_startPoint.x, x);
            int top = std::min(m_startPoint.y, y);
            int bottom = std::max(m_startPoint.y, y);
            left = ConstrainCoordinate(left, 0, m_image.GetWidth() - 1);
            right = ConstrainCoordinate(right, 0, m_image.GetWidth() - 1);
            top = ConstrainCoordinate(top, 0, m_image.GetHeight() - 1);
            bottom = ConstrainCoordinate(bottom, 0, m_image.GetHeight() - 1);
            m_selectionRect = wxRect(left, top, right - left + 1, bottom - top + 1);
            SnapRectToGrid(m_selectionRect);
            UpdateStatusText();
            Refresh();
        }
    }
    
    void OnCaptureLost(wxMouseCaptureLostEvent& event) {
        if (m_mode == SelectionMode::Rectangle && m_isDragging) {
            m_isDragging = false;
            UpdateStatusText();
            Refresh();
        }
    }
    
    wxImage m_image;
    wxRect m_selectionRect;
    bool m_isDragging;
    wxPoint m_startPoint;
    wxStaticText* m_statusText;
    long m_xGridSnap;
    long m_yGridSnap;
    SelectionMode m_mode;
    wxPoint m_selectedPoint;
    std::shared_ptr<wxImage> m_imagePtr;
};

class GrabPreviewDialog : public wxDialog {
public:
    enum class SelectionMode {
        Rectangle, // Click and drag to select a rectangle (grid snap)
        Point      // Click to select a point, then find region
    };
    GrabPreviewDialog(wxWindow* parent, const wxImage& image, const wxString& title, long xGridSnap = 0, long yGridSnap = 0, SelectionMode mode = SelectionMode::Rectangle);
    wxRect GetSelection() const { return m_imagePanel->GetSelection(); }
    wxPoint GetSelectedPoint() const { return m_imagePanel->GetSelectedPoint(); }
    void SetSelectionMode(SelectionMode mode) { m_imagePanel->SetSelectionMode(static_cast<ImagePanel::SelectionMode>(mode)); }
    SelectionMode GetSelectionMode() const { return static_cast<SelectionMode>(m_imagePanel->GetSelectionMode()); }
    
private:
    ImagePanel* m_imagePanel;
    wxStaticText* m_statusText;
    
    DECLARE_EVENT_TABLE()
}; 