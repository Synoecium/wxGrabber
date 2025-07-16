#ifndef AUDIOPLAYBACKCONTROL_H
#define AUDIOPLAYBACKCONTROL_H
#include "AVControlInterface.h"
#include <wx/image.h>
#include "AudioData.h"
#include <memory>
#include <wx/sound.h>
#include "DataParser.h"

// AudioPlaybackControl: Manages audio playback (play, pause, stop, seek) for AudioData objects. Not a UI panel.
// Use SetAudioData to assign audio, PlayPause/Stop to control playback, and SetCurrentPositionMs to seek.
class AudioPlaybackControl : public AVControlInterface {
public:
    AudioPlaybackControl(std::function<void(int, int)> updateCallback);
    void SetAudioData(std::shared_ptr<DataParser::DataObject> audioObject); // Assign audio data to play
    void PlayPause() override; // Play or pause audio
    void Stop() override;      // Stop playback and reset
    void SetCurrentPositionMs(int currentPositionMs) override; // Seek to position (ms)
    bool IsAudioLoaded() const; // Returns true if audio is loaded
private:
    std::shared_ptr<DataParser::DataObject> m_audioObject = nullptr;
    std::unique_ptr<wxSound> m_sound;
    int m_startPositionMs = 0;
};
#endif // AUDIOPLAYBACKCONTROL_H