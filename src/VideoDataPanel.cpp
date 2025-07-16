#include "../include/VideoDataPanel.h"
#include "wx/gdicmn.h"
#include <wx/dcbuffer.h>
#include <wx/timer.h>

wxBEGIN_EVENT_TABLE(VideoDataPanel, wxPanel)
    EVT_PAINT(VideoDataPanel::OnPaint)
wxEND_EVENT_TABLE()

class VideoDataPanelTimer : public wxTimer {
public:
    VideoDataPanelTimer(VideoDataPanel* panel) : m_panel(panel) {}
    void Notify() override { if (m_panel) m_panel->OnTimer(); }
private:
    VideoDataPanel* m_panel;
};

VideoDataPanel::VideoDataPanel(wxWindow* parent, std::function<void(int, int)> updateCallback)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
      AVControlInterface(),
      m_timer(new VideoDataPanelTimer(this)),
      m_currentFrameIndex(0)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Needed for double buffering
    SetUpdateCallback(updateCallback);
}

void VideoDataPanel::SetVideoData(std::shared_ptr<DataParser::DataObject> videoObject) {
    m_videoObject = videoObject;
    if (m_videoObject && m_videoObject->isVideo()) {
        VideoData videoData = m_videoObject->getVideo();
        m_frameRate = videoData.frameRate;
        SetTotalDurationMs(videoData.getDurationMs());
        // Set the panel's minimum and initial size to match the video dimensions
        if (videoData.width > 0 && videoData.height > 0) {
            SetMinSize(wxSize(videoData.width, videoData.height));
            SetInitialSize(wxSize(videoData.width, videoData.height));
            if (GetParent()) GetParent()->Layout();
        }
    }
    m_frames.clear();
    UpdatePreview();
    Refresh();
}

void VideoDataPanel::OnPaint(wxPaintEvent& event) {
    wxBufferedPaintDC dc(this);
    RenderFrame(dc);
}

void VideoDataPanel::RenderFrame(wxDC& dc) {
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    if (m_currentFrame.IsOk()) {
        int w = m_currentFrame.GetWidth();
        int h = m_currentFrame.GetHeight();
        int panelW, panelH;
        GetClientSize(&panelW, &panelH);
        int x = (panelW - w) / 2;
        int y = (panelH - h) / 2;
        dc.DrawBitmap(m_currentFrame, x, y, false);
    }
}

void VideoDataPanel::PlayPause() {
    if (!m_videoObject) return;
    AVControlInterface::PlayPause();
    if (m_frames.empty()) {
        m_frames = m_videoObject->getVideo().getFrameArray();
    }
    if (m_isPlaying) {
        int delay = (m_frameRate > 0) ? (1000 / m_frameRate) : 100;
        m_timer->Start(delay);
    } else {
        m_timer->Stop();
    }
}

void VideoDataPanel::Stop() {
    AVControlInterface::Stop();
    m_currentFrameIndex = 0;
    if (m_timer) m_timer->Stop();
    UpdatePreview();
    Refresh();
}

void VideoDataPanel::OnTimer() {
    if (m_frames.empty()) return;
    m_currentFrameIndex++;
    if (m_currentFrameIndex >= m_frames.size()) {
        m_currentFrameIndex = m_frames.size() - 1;
        return;
    }
    m_currentFrame = m_frames[m_currentFrameIndex];
    Refresh();
}

void VideoDataPanel::UpdatePreview() {
    m_currentFrame = wxImage();
    if (!m_videoObject || m_videoObject->getVideo().frameCount == 0) return;
    m_currentFrame = m_videoObject->getVideo().getPreviewFrame();
}

void VideoDataPanel::SetCurrentPositionMs(int currentPositionMs) {
    AVControlInterface::SetCurrentPositionMs(currentPositionMs);
    m_currentFrameIndex = (currentPositionMs * m_frameRate) / 1000;
    if (!m_videoObject) return;
    if (m_frames.empty()) {
        m_frames = m_videoObject->getVideo().getFrameArray();
    }
    if (m_frames.empty()) {
        m_currentFrameIndex = 0;
        m_currentFrame = wxImage();
    } else {
        m_currentFrameIndex = std::min(m_currentFrameIndex, m_frames.size() - 1);
        //logDebug("SetCurrentPositionMs: m_currentFrameIndex = " + std::to_string(m_currentFrameIndex) + " m_frames.size() = " + std::to_string(m_frames.size()));
        m_currentFrame = m_frames[m_currentFrameIndex];
    }
    Refresh();
}