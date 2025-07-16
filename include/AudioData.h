#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <cstring>
#include <wx/string.h>
#include "CommonTypes.h"

#pragma pack(push, 1)
struct wavHeader {
    // RIFF chunk
    char     chunkId[4] = {'R', 'I', 'F', 'F'};
    uint32_t chunkSize;          // File size - 8
    char     format[4] = {'W', 'A', 'V', 'E'};
    
    // fmt subchunk
    char     subchunk1Id[4] = {'f', 'm', 't', ' '};
    uint32_t subchunk1Size = 16; // 16 for PCM
    uint16_t audioFormat = 1;    // 1 for PCM
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    
    // data subchunk
    char     subchunk2Id[4] = {'d', 'a', 't', 'a'};
    uint32_t subchunk2Size;      // data size
};
#pragma pack(pop)

class AudioData {
public:
    int sampleRate = 0;     // Audio sample rate
    int channels = 0;       // Number of audio channels
    int bitsPerSample = 0;  // Bits per sample
    int durationMs = 0;     // Duration in milliseconds
    int pcmDataSize = 0;    // Size of PCM data
    std::vector<uint8_t> data;  // Raw audio data
    ObjectType typeID = ObjectType::DAT_OGG;  // Original type ID from parser (e.g., DAT_SAMPLE, DAT_OGG)

    // MIDI-specific members
    uint16_t midiDivisions = 0; // MIDI beat divisions
    struct MidiTrack {
        std::vector<uint8_t> data;
    };
    std::vector<MidiTrack> midiTracks;

    // Helper function to check if format is valid
    bool isValidFormat() const;

    // Parse OGG data from buffer
    static bool parse(const std::vector<uint8_t>& dataBuffer, ObjectType typeID, AudioData& outAudio);

    // Create a sample empty audio sample
    static AudioData createSampleAudio();
    
    // Create a sample empty MIDI file
    static AudioData createSampleMIDI();

    // Create a sample empty OGG audio object
    static AudioData createSampleOGG();

    // Parse WAV/PCM sample data for SAMP objects
    static bool parseSample(const std::vector<uint8_t>& wavData, AudioData& outAudio);

    // Parse MIDI data for MIDI objects
    static bool parseMIDI(const std::vector<uint8_t>& midiData, AudioData& outAudio);

    // Serialize audio data to raw buffer
    // Returns a buffer containing the serialized audio data
    std::vector<uint8_t> serialize() const;

    // Get WAV data for playback
    // Returns a vector containing the WAV data
    std::vector<uint8_t> getWavData(int position_ms = 0) const;

    // Get MIDI data in proper MIDI file format
    // Returns a vector containing the MIDI file data if object type is MIDI
    std::vector<uint8_t> getMidiData() const;

    // import audio data from a file
    bool importFromFile(const std::string& filepath);

    // Get audio duration in milliseconds
    int getDurationMs() const;
    
    // Compare audio data with the content of a file
    // Returns true if the audio data equals the file content
    bool compareWithFile(const std::string& filepath) const;

    // Equality operator
    bool operator==(const AudioData& other) const {
        return data.size() == other.data.size() &&
               std::memcmp(data.data(), other.data.data(), data.size()) == 0;
    }

    bool operator!=(const AudioData& other) const {
        return !(*this == other);
    }

    // Get a descriptive caption for preview display
    wxString getPreviewCaption() const;

    void calculateMIDIDuration();
};