#ifndef AVCONTROLINTERFACE_H
#define AVCONTROLINTERFACE_H
#include <functional>
#include <wx/timer.h>
#include <chrono>
#include "log.h"
#include <memory>

class AudioData;
class wxSound;

class AVControlTimer : public wxTimer {
public:
    AVControlTimer(std::function<void()> callback) : m_callback(callback) {}
    void Notify() override { if (m_callback) m_callback(); }
private:
    std::function<void()> m_callback;
};

// Audio/Video control interface
class AVControlInterface {
public:
    AVControlInterface() : m_currentPositionMs(0) { m_timer = new AVControlTimer(std::bind(&AVControlInterface::OnTimer, this)); }
    virtual void PlayPause()
    {
        if (m_totalDurationMs <= 0) return;
        if (m_currentPositionMs >= m_totalDurationMs) {
            m_currentPositionMs = 0;    // Reset to start of video
            logDebug("PlayPause: Reset to start of video");
        }
        if (m_isPaused || !m_isPlaying) {
            logDebug("PlayPause: Resuming playback");
            m_isPaused = false; // Resume from pause
            m_isPlaying = true;
            m_lastUpdateTime = std::chrono::steady_clock::now();
            m_timer->Start(m_updateIntervalMs);
        } else {
            logDebug("PlayPause: Pausing playback");
            m_isPaused = true; // Pause
            m_isPlaying = false;
            if (m_timer) m_timer->Stop();
            OnTimer();
        }
    };
    virtual void Stop()
    {
        m_isPlaying = false;
        m_isPaused = false;
        if (m_timer) m_timer->Stop();
    }
    virtual ~AVControlInterface() = default;

    void SetTotalDurationMs(int totalDurationMs) { m_totalDurationMs = totalDurationMs; }
    virtual void SetCurrentPositionMs(int currentPositionMs) { 
        m_currentPositionMs = currentPositionMs;
        m_lastUpdateTime = std::chrono::steady_clock::now();
        //logDebug("SetCurrentPositionMs: m_currentPositionMs=" + std::to_string(m_currentPositionMs) + ", m_totalDurationMs=" + std::to_string(m_totalDurationMs));
        m_updateCallback(m_currentPositionMs, m_totalDurationMs);
    }
    bool GetIsPaused() const { return m_isPaused; }
    bool GetIsPlaying() const { return m_isPlaying; }

protected:
    void SetUpdateCallback(std::function<void(int, int)> callback) { m_updateCallback = callback; }
    void OnTimer()
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpdateTime).count();
        m_currentPositionMs += elapsed;
        m_lastUpdateTime = now;
        if (m_currentPositionMs > m_totalDurationMs) {
            m_currentPositionMs = m_totalDurationMs;
            logDebug("OnTimer: Reached end of video");
            Stop();
        }
        m_updateCallback(m_currentPositionMs, m_totalDurationMs);
    }

    bool m_isPlaying = false;
    bool m_isPaused = false;
    int m_totalDurationMs = 0;
    int m_currentPositionMs = 0;
    int m_updateIntervalMs = 100;
    std::function<void(int, int)> m_updateCallback;
    std::chrono::steady_clock::time_point m_lastUpdateTime;
    AVControlTimer* m_timer = nullptr;
};
#endif // AVCONTROLINTERFACE_H