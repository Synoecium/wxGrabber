#ifndef VORBIS_WRAPPER_H
#define VORBIS_WRAPPER_H

#include <vector>
#include <cstdint>
/*
typedef struct {
    int sample_rate;
    int channels;
    int bitrate;
    int pcm_data_size;
    int duration_ms;
} VorbisInfo;*/

#include "minivorbis_impl.h"
class VorbisWrapper {
public:
    // Convert Ogg Vorbis data to WAV format
    static bool ConvertOggToWav(const std::vector<uint8_t>& oggBuffer, std::vector<uint8_t>& wavBuffer);
    static bool GetVorbisInfo(const std::vector<uint8_t>& oggBuffer, VorbisInfo& info);
    static bool ConvertOggToPCM(const std::vector<uint8_t>& oggBuffer, std::vector<uint8_t>& pcmBuffer);
    
private:
    // Prevent instantiation
    VorbisWrapper() = delete;
    ~VorbisWrapper() = delete;
};

#endif // VORBIS_WRAPPER_H 