#include "../include/AudioData.h"
#include "../include/vorbis/vorbis_wrapper.h"
#include "../include/log.h"
#include <cstdint>
#include <filesystem>

bool AudioData::isValidFormat() const {
    if (typeID == ObjectType::DAT_MIDI) {
        return midiDivisions > 0 && midiTracks.size() == 32;
    }
    return sampleRate > 0 && channels > 0 && bitsPerSample > 0 && !data.empty();
}

int AudioData::getDurationMs() const {
    return durationMs;
}

std::vector<uint8_t> AudioData::getWavData(int position_ms) const {
    if (!isValidFormat()) {
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> pcmData;
    if (typeID == ObjectType::DAT_OGG) {
        // Convert ogg data to pcm data
        if (!VorbisWrapper::ConvertOggToPCM(data, pcmData)) {
            logError("Failed to convert ogg data to pcm data");
            return std::vector<uint8_t>();
        }
    } else if (typeID == ObjectType::DAT_SAMP) {
        // For DAT_SAMP, data is already PCM
        pcmData = data;
    } else if (typeID == ObjectType::DAT_MIDI) {
        // MIDI cannot be converted to WAV directly here
        logWarning("getWavData: DAT_MIDI to WAV conversion is not supported");
        return std::vector<uint8_t>();
    } else {
        logError("Unsupported audio type for getWavData: " + std::to_string(static_cast<int>(typeID)));
        return std::vector<uint8_t>();
    }

    // Calculate offset in PCM data based on position
    uint64_t bytesPerSample = bitsPerSample / 8;
    uint64_t bytesPerSecond = sampleRate * channels * bytesPerSample;
    uint64_t offsetBytes = (static_cast<uint64_t>(position_ms) * bytesPerSecond) / 1000;
    
    // Ensure offset is aligned with sample boundaries
    // round up to the nearest channel * bytesPerSample
    offsetBytes = (offsetBytes + (channels * bytesPerSample) - 1) & ~(channels * bytesPerSample - 1);
    
    // Ensure we don't exceed the buffer
    if (offsetBytes >= pcmData.size()) {
        logError("Offset exceeds buffer size " + std::to_string(offsetBytes) + " >= " + std::to_string(pcmData.size()));
        return std::vector<uint8_t>();
    }

    // Calculate remaining PCM data size
    uint32_t remainingPcmSize = static_cast<uint32_t>(pcmData.size()) - static_cast<uint32_t>(offsetBytes);

    // Create WAV header
    wavHeader header;
    header.chunkSize = sizeof(wavHeader) + remainingPcmSize - 8;
    header.numChannels = channels;
    header.sampleRate = sampleRate;
    header.byteRate = sampleRate * channels * bitsPerSample / 8;
    header.blockAlign = channels * bitsPerSample / 8;
    header.bitsPerSample = bitsPerSample;
    header.subchunk2Size = remainingPcmSize;

    // Create output buffer
    std::vector<uint8_t> wavData;
    wavData.reserve(sizeof(wavHeader) + remainingPcmSize);
    
    // Add header
    const uint8_t* headerPtr = reinterpret_cast<const uint8_t*>(&header);
    wavData.insert(wavData.end(), headerPtr, headerPtr + sizeof(wavHeader));
    
    // Add PCM data starting from the calculated offset
    wavData.insert(wavData.end(), pcmData.begin() + offsetBytes, pcmData.end());

    return wavData;
}

std::vector<uint8_t> AudioData::getMidiData() const {
    if (typeID != ObjectType::DAT_MIDI) {
        logWarning("getMidiData: Object is not a MIDI type");
        return std::vector<uint8_t>();
    }

    if (!isValidFormat()) {
        logError("getMidiData: Invalid MIDI format");
        return std::vector<uint8_t>();
    }

    // Generate standard MIDI file (SMF) format
    std::vector<uint8_t> buffer;
    
    // Count non-empty tracks
    uint16_t numTracks = 0;
    for (size_t i = 0; i < midiTracks.size() && i < 32; ++i) {
        if (!midiTracks[i].data.empty()) {
            numTracks++;
        }
    }
    
    // If no tracks have data, return empty buffer
    if (numTracks == 0) {
        logWarning("getMidiData: No MIDI tracks contain data");
        return std::vector<uint8_t>();
    }
    
    // === HEADER CHUNK ===
    // "MThd" - MIDI header identifier (4 bytes)
    buffer.push_back('M');
    buffer.push_back('T');
    buffer.push_back('h');
    buffer.push_back('d');
    
    // Header length - always 6 bytes (4 bytes, big endian)
    buffer.push_back(0x00);
    buffer.push_back(0x00);
    buffer.push_back(0x00);
    buffer.push_back(0x06);
    
    // Format - MIDI format (2 bytes, big endian)
    uint16_t midiFormat = 1; // multiple track file format
    buffer.push_back(static_cast<uint8_t>((midiFormat >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(midiFormat & 0xFF));
    
    // Number of tracks (2 bytes, big endian)
    buffer.push_back(static_cast<uint8_t>((numTracks >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(numTracks & 0xFF));
    
    // Division - MIDI timing division (2 bytes, big endian)
    // Use the midiDivisions value from the Allegro MIDI data
    buffer.push_back(static_cast<uint8_t>((midiDivisions >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(midiDivisions & 0xFF));
    
    // === TRACK CHUNKS ===
    for (size_t i = 0; i < midiTracks.size() && i < 32; ++i) {
        const auto& track = midiTracks[i];
        
        // Skip empty tracks
        if (track.data.empty()) {
            continue;
        }
        
        // Prepare track data with End of Track meta event
        std::vector<uint8_t> trackData = track.data;
        
        // Add End of Track meta event if not already present (0x00 0xFF 0x2F 0x00)
        // Check if track already ends with End of Track (last 4 bytes should be 0x00 0xFF 0x2F 0x00)
        bool hasEndOfTrack = false;
        if (trackData.size() >= 4) {
            size_t pos = trackData.size() - 4;
            if (trackData[pos] == 0x00 && trackData[pos + 1] == 0xFF && 
                trackData[pos + 2] == 0x2F && trackData[pos + 3] == 0x00) {
                hasEndOfTrack = true;
            }
        }
        
        if (!hasEndOfTrack) {
            // Add End of Track: delta time (0x00) + meta event (0xFF 0x2F 0x00)
            trackData.push_back(0x00); // Delta time = 0
            trackData.push_back(0xFF); // Meta event
            trackData.push_back(0x2F); // End of Track type
            trackData.push_back(0x00); // Length = 0
        }
        
        // "MTrk" - Track header identifier (4 bytes)
        buffer.push_back('M');
        buffer.push_back('T');
        buffer.push_back('r');
        buffer.push_back('k');
        
        // Track length (4 bytes, big endian)
        uint32_t trackLength = static_cast<uint32_t>(trackData.size());
        buffer.push_back(static_cast<uint8_t>((trackLength >> 24) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((trackLength >> 16) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((trackLength >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>(trackLength & 0xFF));
        
        // Track data
        buffer.insert(buffer.end(), trackData.begin(), trackData.end());
    }
    
    return buffer;
}

bool AudioData::parse(const std::vector<uint8_t>& dataBuffer, ObjectType typeID, AudioData& outAudio) {
    outAudio.typeID = typeID;
    if (typeID == ObjectType::DAT_OGG) {
        // Get OGG info
        VorbisInfo vorbisInfo;
        if (!VorbisWrapper::GetVorbisInfo(dataBuffer, vorbisInfo)) {
            return false;
        }

        // Store OGG info
        outAudio.sampleRate = vorbisInfo.sample_rate;
        outAudio.channels = vorbisInfo.channels;
        outAudio.bitsPerSample = 16; // OGG Vorbis typically uses 16-bit samples
        outAudio.durationMs = vorbisInfo.duration_ms;
        outAudio.pcmDataSize = vorbisInfo.pcm_data_size;
        outAudio.data = dataBuffer;     // Store raw OGG data
    } else if (typeID == ObjectType::DAT_SAMP) {
        // DAT_SAMP format:
        // 16 bit - <bits>               - sample bits (negative for stereo)
        // 16 bit - <freq>               - sample frequency
        // 32 bit - <length>             - sample length
        // var    - <data>               - sample data
        
        if (dataBuffer.size() < 8) { // Minimum size for header (2+2+4 bytes)
            logError("DAT_SAMP data too small");
            return false;
        }

        const uint8_t* data = dataBuffer.data();
        size_t pos = 0;

        // Read bits (16-bit)
        int16_t bits = (data[pos] << 8) | data[pos + 1];
        pos += 2;

        // Read frequency (16-bit)
        uint16_t freq = (data[pos] << 8) | data[pos + 1];
        pos += 2;

        // Read length (32-bit)
        uint32_t length = (data[pos] << 24) | (data[pos + 1] << 16) | 
                         (data[pos + 2] << 8) | data[pos + 3];
        pos += 4;

        // Validate data size
        if (pos + length > dataBuffer.size()) {
            logError("DAT_SAMP data length exceeds buffer size");
            return false;
        }

        logDebug("DAT_SAMP: bits=" + std::to_string(bits) + ", freq=" + std::to_string(freq) + ", length=" + std::to_string(length));

        // Set audio parameters
        outAudio.bitsPerSample = abs(bits);  // Use absolute value for bits
        outAudio.channels = (bits < 0) ? 2 : 1;  // Negative bits indicates stereo
        outAudio.sampleRate = freq;
        outAudio.pcmDataSize = length;
        
        // Calculate duration in milliseconds
        // duration = (length in bytes) / (bytes per second)
        // bytes per second = sampleRate * channels * (bitsPerSample/8)
        outAudio.durationMs = (int)((1000.0 * length) / 
            (outAudio.sampleRate * outAudio.channels * (outAudio.bitsPerSample / 8.0)));

        logDebug("DAT_SAMP: durationMs=" + std::to_string(outAudio.durationMs));

        // Store the raw sample data
        outAudio.data.assign(data + pos, data + pos + length);
    } else if (typeID == ObjectType::DAT_MIDI) {
        // DAT_MIDI format:
        // 16 bit - <divisions>          - MIDI beat divisions
        // 32x {
        //    32 bit - <length>          - track length, in bytes
        //    var    - <data>            - MIDI track data
        // }
        if (dataBuffer.size() < 2) {
            logError("DAT_MIDI data too small");
            return false;
        }
        size_t pos = 0;
        // Read divisions (16-bit, big endian)
        outAudio.midiDivisions = (dataBuffer[pos] << 8) | dataBuffer[pos + 1];
        pos += 2;
        outAudio.midiTracks.clear();
        for (int i = 0; i < 32; ++i) {
            if (pos + 4 > dataBuffer.size()) {
                logError("DAT_MIDI: not enough data for track length");
                return false;
            }
            uint32_t length = (dataBuffer[pos] << 24) | (dataBuffer[pos + 1] << 16) |
                              (dataBuffer[pos + 2] << 8) | dataBuffer[pos + 3];
            pos += 4;
            if (pos + length > dataBuffer.size()) {
                logError("DAT_MIDI: not enough data for track");
                return false;
            }
            AudioData::MidiTrack track;
            track.data.assign(dataBuffer.begin() + pos, dataBuffer.begin() + pos + length);
            outAudio.midiTracks.push_back(std::move(track));
            pos += length;
        }
        outAudio.calculateMIDIDuration();
        return true;
    } else {
        logError("Invalid typeID for AudioData::parse: " + std::to_string(static_cast<int>(typeID)));
        return false;
    }
    return true;
}

std::vector<uint8_t> AudioData::serialize() const {
    if (!isValidFormat()) {
        return std::vector<uint8_t>();
    }
    if (typeID == ObjectType::DAT_OGG) {
        // For OGG data, just return the raw data
        return data;
    } else if (typeID == ObjectType::DAT_SAMP) {
        // For DAT_SAMP, format according to spec:
        // 16 bit - <bits>               - sample bits (negative for stereo)
        // 16 bit - <freq>               - sample frequency
        // 32 bit - <length>             - sample length
        // var    - <data>               - sample data

        // Calculate total size needed
        size_t totalSize = 8 + data.size(); // 8 bytes for header + data size
        std::vector<uint8_t> buffer;
        buffer.reserve(totalSize);

        // Write bits (16-bit, negative for stereo)
        int16_t bits = bitsPerSample;
        if (channels == 2) {
            bits = -bits; // Negative for stereo
        }
        buffer.push_back(static_cast<uint8_t>((bits >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>(bits & 0xFF));

        // Write frequency (16-bit)
        buffer.push_back(static_cast<uint8_t>((sampleRate >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>(sampleRate & 0xFF));

        // Write length (32-bit)
        uint32_t length = static_cast<uint32_t>(data.size());
        buffer.push_back(static_cast<uint8_t>((length >> 24) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((length >> 16) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>(length & 0xFF));

        // Write sample data
        buffer.insert(buffer.end(), data.begin(), data.end());

        return buffer;
    } else if (typeID == ObjectType::DAT_MIDI) {
        // Format: 16-bit divisions, then 32 tracks of (32-bit length + data)
        std::vector<uint8_t> buffer;
        // Write divisions (16-bit, big endian)
        buffer.push_back(static_cast<uint8_t>((midiDivisions >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>(midiDivisions & 0xFF));
        for (int i = 0; i < 32; ++i) {
            const auto& track = (i < midiTracks.size()) ? midiTracks[i] : AudioData::MidiTrack{};
            uint32_t length = static_cast<uint32_t>(track.data.size());
            buffer.push_back(static_cast<uint8_t>((length >> 24) & 0xFF));
            buffer.push_back(static_cast<uint8_t>((length >> 16) & 0xFF));
            buffer.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
            buffer.push_back(static_cast<uint8_t>(length & 0xFF));
            buffer.insert(buffer.end(), track.data.begin(), track.data.end());
        }
        return buffer;
    }
    // For unknown types, return empty buffer
    return std::vector<uint8_t>();
}

bool AudioData::compareWithFile(const std::string& filepath) const {
    // Check if we have valid audio data
    if (data.empty() || !isValidFormat()) {
        return false;
    }

    // Load file into audio data
    AudioData audioData;
    audioData.typeID = typeID;
    if (!audioData.importFromFile(filepath)) {
        return false;
    }

    // Compare audio data
    return data == audioData.data;
}

bool AudioData::importFromFile(const std::string& filepath) {
    // Open file
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return false;
    }

    // Get file size
    size_t fileSize = std::filesystem::file_size(filepath);

    // Read file into buffer
    std::vector<uint8_t> buffer(fileSize);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return false;
    }

    // Parse buffer into this audio data
    if (typeID == ObjectType::DAT_OGG) {
        return parse(buffer, typeID, *this);
    } else if (typeID == ObjectType::DAT_SAMP) {
        return parseSample(buffer, *this);
    } else if (typeID == ObjectType::DAT_MIDI) {
        return parseMIDI(buffer, *this);
    }
    return false;
}

bool AudioData::parseSample(const std::vector<uint8_t>& wavData, AudioData& outAudio) {
    // Minimal WAV header parsing (PCM only)
    if (wavData.size() < 44) return false;
    const uint8_t* d = wavData.data();
    if (memcmp(d, "RIFF", 4) != 0 || memcmp(d + 8, "WAVE", 4) != 0) return false;
    // Find 'fmt ' chunk
    size_t pos = 12;
    uint16_t audioFormat = 0;
    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitsPerSample = 0;
    uint32_t dataOffset = 0, dataSize = 0;
    while (pos + 8 <= wavData.size()) {
        uint32_t chunkId = *(uint32_t*)(d + pos);
        uint32_t chunkSize = *(uint32_t*)(d + pos + 4);
        if (memcmp(d + pos, "fmt ", 4) == 0) {
            if (chunkSize < 16 || pos + 8 + chunkSize > wavData.size()) return false;
            audioFormat = *(uint16_t*)(d + pos + 8);
            numChannels = *(uint16_t*)(d + pos + 10);
            sampleRate = *(uint32_t*)(d + pos + 12);
            bitsPerSample = *(uint16_t*)(d + pos + 22);
        } else if (memcmp(d + pos, "data", 4) == 0) {
            dataOffset = pos + 8;
            dataSize = chunkSize;
            break;
        }
        pos += 8 + chunkSize;
    }
    if (audioFormat != 1 || numChannels == 0 || sampleRate == 0 || bitsPerSample == 0 || dataOffset == 0 || dataSize == 0) return false;
    if (dataOffset + dataSize > wavData.size()) return false;
    outAudio.sampleRate = sampleRate;
    outAudio.channels = numChannels;
    outAudio.bitsPerSample = bitsPerSample;
    outAudio.data.assign(d + dataOffset, d + dataOffset + dataSize);
    outAudio.pcmDataSize = dataSize;
    outAudio.durationMs = (int)((1000.0 * dataSize) / (sampleRate * numChannels * (bitsPerSample / 8.0)));
    return true;
}

bool AudioData::parseMIDI(const std::vector<uint8_t>& midiData, AudioData& outAudio) {
    // Clear previous data
    outAudio.midiTracks.clear();
    outAudio.midiTracks.resize(32);
    outAudio.midiDivisions = 0;
    outAudio.typeID = ObjectType::DAT_MIDI;
    outAudio.sampleRate = 0;
    outAudio.channels = 0;
    outAudio.bitsPerSample = 0;
    outAudio.pcmDataSize = 0;
    outAudio.durationMs = 0;

    if (midiData.size() < 14) return false; // Minimum for header + 1 track header
    size_t pos = 0;
    // Parse header chunk
    if (!(midiData[pos] == 'M' && midiData[pos+1] == 'T' && midiData[pos+2] == 'h' && midiData[pos+3] == 'd')) {
        logError("parseMIDI: Missing MThd header");
        return false;
    }
    pos += 4;
    // Header length (should be 6)
    uint32_t headerLen = (midiData[pos] << 24) | (midiData[pos+1] << 16) | (midiData[pos+2] << 8) | midiData[pos+3];
    pos += 4;
    if (headerLen != 6 || pos + 6 > midiData.size()) {
        logError("parseMIDI: Invalid header length");
        return false;
    }
    // Format
    uint16_t midiFormat = (midiData[pos] << 8) | midiData[pos+1];
    pos += 2;
    // Number of tracks
    uint16_t numTracks = (midiData[pos] << 8) | midiData[pos+1];
    pos += 2;
    // Division
    outAudio.midiDivisions = (midiData[pos] << 8) | midiData[pos+1];
    pos += 2;

    // Parse each track
    int trackCount = 0;
    while (pos + 8 <= midiData.size() && trackCount < 32) {
        // Look for MTrk
        if (!(midiData[pos] == 'M' && midiData[pos+1] == 'T' && midiData[pos+2] == 'r' && midiData[pos+3] == 'k')) {
            logError("parseMIDI: Missing MTrk header at track " + std::to_string(trackCount));
            return false;
        }
        pos += 4;
        // Track length
        uint32_t trackLen = (midiData[pos] << 24) | (midiData[pos+1] << 16) | (midiData[pos+2] << 8) | midiData[pos+3];
        pos += 4;
        if (pos + trackLen > midiData.size()) {
            logError("parseMIDI: Track length exceeds buffer size");
            return false;
        }
        // Store track data
        outAudio.midiTracks[trackCount].data.assign(midiData.begin() + pos, midiData.begin() + pos + trackLen);
        pos += trackLen;
        ++trackCount;
    }
    // Clear any remaining tracks
    for (int i = trackCount; i < 32; ++i) {
        outAudio.midiTracks[i].data.clear();
    }

    // Calculate duration after parsing
    outAudio.calculateMIDIDuration();
    return true;
}

void AudioData::calculateMIDIDuration() {
    if (typeID != ObjectType::DAT_MIDI || midiDivisions == 0) {
        return;
    }

    // Only support ticks per quarter note (not SMPTE) for now
    if (midiDivisions & 0x8000) {
        logDebug("SMPTE timing not supported in this implementation.");
        durationMs = 0;
        return;
    }
    uint16_t ticksPerQuarter = midiDivisions & 0x7FFF;

    struct TempoEvent {
        uint32_t tick;
        uint32_t tempo; // in microseconds per quarter note
    };

    std::vector<TempoEvent> tempoChanges = { {0, 500000} }; // Default tempo
    uint32_t maxTick = 0;

    // Gather all tempo changes and max tick across all tracks
    for (size_t track = 0; track < midiTracks.size(); ++track) {
        const auto& data = midiTracks[track].data;
        if (data.empty()) continue;
        size_t pos = 0;
        uint32_t tick = 0;
        uint8_t runningStatus = 0;
        while (pos < data.size()) {
            // Read delta time (variable length)
            uint32_t delta = 0;
            uint8_t byte;
            do {
                if (pos >= data.size()) break;
                byte = data[pos++];
                delta = (delta << 7) | (byte & 0x7F);
            } while (byte & 0x80);
            tick += delta;

            if (pos >= data.size()) break;
            uint8_t statusByte = data[pos];
            if (statusByte < 0x80) {
                statusByte = runningStatus;
            } else {
                statusByte = data[pos++];
                runningStatus = statusByte;
            }

            if (statusByte == 0xFF) {
                if (pos + 1 >= data.size()) break;
                uint8_t metaType = data[pos++];
                // Read variable length for meta event
                uint32_t length = 0;
                do {
                    if (pos >= data.size()) break;
                    byte = data[pos++];
                    length = (length << 7) | (byte & 0x7F);
                } while (byte & 0x80);
                if (metaType == 0x51 && length == 3 && pos + 2 < data.size()) {
                    uint32_t tempo = (data[pos] << 16) | (data[pos + 1] << 8) | data[pos + 2];
                    tempoChanges.push_back({tick, tempo});
                }
                pos += length;
            } else if ((statusByte & 0xF0) == 0xF0) {
                // SysEx event
                uint32_t length = 0;
                do {
                    if (pos >= data.size()) break;
                    byte = data[pos++];
                    length = (length << 7) | (byte & 0x7F);
                } while (byte & 0x80);
                pos += length;
            } else {
                int dataLen = ((statusByte & 0xF0) == 0xC0 || (statusByte & 0xF0) == 0xD0) ? 1 : 2;
                pos += dataLen;
            }
        }
        if (tick > maxTick) maxTick = tick;
    }

    // Sort tempo changes by tick (just in case)
    std::sort(tempoChanges.begin(), tempoChanges.end(), [](const TempoEvent& a, const TempoEvent& b) {
        return a.tick < b.tick;
    });

    // Calculate total duration in ms
    double totalMs = 0.0;
    uint32_t prevTick = 0;
    uint32_t currentTempo = 500000; // default tempo
    for (size_t i = 0; i < tempoChanges.size(); ++i) {
        uint32_t nextTick = (i + 1 < tempoChanges.size()) ? tempoChanges[i + 1].tick : maxTick;
        if (nextTick > maxTick) nextTick = maxTick;
        uint32_t deltaTicks = nextTick - prevTick;
        double currentTempoMs = static_cast<double>(currentTempo) / 1000.0;
        double segmentMs = (static_cast<double>(deltaTicks) * currentTempoMs) / static_cast<double>(ticksPerQuarter);
        totalMs += segmentMs;
        currentTempo = tempoChanges[i].tempo;
        prevTick = nextTick;
    }
    durationMs = static_cast<uint32_t>(totalMs);
}

wxString AudioData::getPreviewCaption() const {
    wxString typeStr;
    if (typeID == ObjectType::DAT_OGG) {
        typeStr = "OGG Vorbis";
    } else if (typeID == ObjectType::DAT_SAMP) {
        typeStr = "PCM Sample";
    } else if (typeID == ObjectType::DAT_MIDI) {
        return wxString::Format("MIDI: %d divisions, %d tracks", midiDivisions, (int)midiTracks.size());
    } else {
        typeStr = "Audio";
    }
    wxString durationStr = wxString::Format("%d ms", durationMs);
    if (durationMs >= 60000) {
        durationStr = wxString::Format("%d:%02d min", durationMs / 60000, (durationMs % 60000) / 1000);
    }
    else if (durationMs >= 1000) {
        durationStr = wxString::Format("%.2f s", durationMs / 1000.0);
    }
    return wxString::Format("%s: %d Hz, %d ch, %d bit, duration: %s", 
        typeStr, sampleRate, channels, bitsPerSample, durationStr);
}

AudioData AudioData::createSampleAudio() {
    AudioData audio;
    
    // Set basic properties for a 100ms audio sample
    audio.sampleRate = 44100;     // Standard sample rate
    audio.channels = 1;           // Mono
    audio.bitsPerSample = 16;     // 16-bit
    audio.durationMs = 100;       // 100 milliseconds
    audio.typeID = ObjectType::DAT_SAMP;
    
    // Calculate number of samples for 100ms
    int numSamples = (audio.sampleRate * audio.durationMs) / 1000; // 4410 samples
    int dataSize = numSamples * (audio.bitsPerSample / 8); // 8820 bytes
    audio.pcmDataSize = dataSize;
    
    // Generate 100ms of random noise
    audio.data.resize(dataSize);
    
    // Use a simple linear congruential generator for consistent random noise
    uint32_t seed = 12345; // Fixed seed for reproducible noise
    for (int i = 0; i < numSamples; i++) {
        // Generate pseudo-random 16-bit signed value
        seed = seed * 1103515245 + 12345; // LCG formula
        int16_t sample = static_cast<int16_t>((seed >> 16) & 0xFFFF);
        
        // Scale down the amplitude to about 25% to avoid being too loud
        sample = sample / 4;
        
        // Store as little-endian 16-bit PCM
        audio.data[i * 2] = static_cast<uint8_t>(sample & 0xFF);
        audio.data[i * 2 + 1] = static_cast<uint8_t>((sample >> 8) & 0xFF);
    }
    
    return audio;
}

AudioData AudioData::createSampleMIDI() {
    AudioData midi;
    
    // Set MIDI-specific properties
    midi.typeID = ObjectType::DAT_MIDI;
    midi.midiDivisions = 96;  // Standard MIDI divisions (ticks per quarter note)
    
    // Create 32 empty MIDI tracks as per DAT_MIDI format
    midi.midiTracks.resize(32);
    for (int i = 0; i < 32; ++i) {
        midi.midiTracks[i].data.clear();  // Empty tracks
    }
    
    // For MIDI files, these fields are not used the same way
    midi.sampleRate = 0;
    midi.channels = 0;
    midi.bitsPerSample = 0;
    midi.durationMs = 0;  // No playback duration for empty MIDI
    midi.pcmDataSize = 0;
    
    return midi;
}

AudioData AudioData::createSampleOGG() {
    AudioData audio;
    audio.typeID = ObjectType::DAT_OGG;
    audio.sampleRate = 44100; // Standard sample rate (placeholder)
    audio.channels = 2;       // Stereo (placeholder)
    audio.bitsPerSample = 16; // 16-bit (placeholder)
    audio.durationMs = 0;     // No audio data
    audio.pcmDataSize = 0;
    audio.data.clear();       // No OGG data
    return audio;
}