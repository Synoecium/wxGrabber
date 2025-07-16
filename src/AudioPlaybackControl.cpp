#include "../include/AudioPlaybackControl.h"
#include <wx/msgdlg.h>

AudioPlaybackControl::AudioPlaybackControl(std::function<void(int, int)> updateCallback)
{
    SetUpdateCallback(updateCallback);
}

void AudioPlaybackControl::SetAudioData(std::shared_ptr<DataParser::DataObject> audioObject) {
    m_audioObject = audioObject;
    if (m_audioObject) {
        SetTotalDurationMs(m_audioObject->getAudio().getDurationMs());
    } else {
        SetTotalDurationMs(0);
    }
}

void AudioPlaybackControl::PlayPause() {
    if (!m_audioObject) return;
    AVControlInterface::PlayPause();
    if (m_isPlaying) {
        int startPosition = m_currentPositionMs;
        std::vector<uint8_t> wavData = m_audioObject->getAudio().getWavData(startPosition);
        if (wavData.empty()) {
            wxMessageBox("Failed to create WAV data", "Error", wxOK | wxICON_ERROR);
            return;
        }
        m_sound = std::make_unique<wxSound>(wavData.size(), wavData.data());
        if (!m_sound->IsOk()) {
            wxMessageBox("Failed to create sound", "Error", wxOK | wxICON_ERROR);
            return;
        }
        if (!m_sound->Play(wxSOUND_ASYNC)) {
            wxMessageBox("Failed to play sound", "Error", wxOK | wxICON_ERROR);
            return;
        }
    } else {
        if (m_sound) m_sound->Stop();
    }
}

void AudioPlaybackControl::Stop() {
    AVControlInterface::Stop();
    if (m_sound) m_sound->Stop();
}

void AudioPlaybackControl::SetCurrentPositionMs(int currentPositionMs) {
    AVControlInterface::SetCurrentPositionMs(currentPositionMs);
    if (!m_audioObject) return;
    if (m_isPlaying) {
        Stop();
        PlayPause();
    }
}

bool AudioPlaybackControl::IsAudioLoaded() const {
    return m_audioObject != nullptr;
} 