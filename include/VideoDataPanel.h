#pragma once
#include <wx/panel.h>
#include <wx/image.h>
#include <wx/bitmap.h>
#include "VideoData.h"
#include "AVControlInterface.h"
#include "DataParser.h"

// Custom event for frame change notification
wxDECLARE_EVENT(wxEVT_VIDEO_FRAME_CHANGED, wxCommandEvent);
// Custom event reached end of video
wxDECLARE_EVENT(wxEVT_VIDEO_REACHED_END, wxCommandEvent);

class VideoDataPanel : public wxPanel, public AVControlInterface {
public:
    VideoDataPanel(wxWindow* parent, std::function<void(int, int)> updateCallback);
    void SetVideoData(std::shared_ptr<DataParser::DataObject> videoObject);
    void PlayPause() override;
    void Stop() override;
    void SetCurrentPositionMs(int currentPositionMs) override;
    void OnTimer();

protected:
    void OnPaint(wxPaintEvent& event);
    void RenderFrame(wxDC& dc);
    void UpdatePreview();

    std::shared_ptr<DataParser::DataObject> m_videoObject = nullptr;
    wxImage m_currentFrame;
    int m_frameRate = 0;
    uint64_t m_currentFrameIndex = 0;
    class VideoDataPanelTimer* m_timer = nullptr;
    std::vector<wxImage> m_frames;

    wxDECLARE_EVENT_TABLE();
}; 