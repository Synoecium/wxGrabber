#include "../../include/vorbis/vorbis_wrapper.h"
#include "../../include/vorbis/minivorbis_impl.h"

bool VorbisWrapper::ConvertOggToWav(const std::vector<uint8_t>& oggBuffer, std::vector<uint8_t>& wavBuffer) {
    if (oggBuffer.empty()) {
        return false;
    }

    VorbisInfo info;
    if (get_vorbis_info_from_memory(oggBuffer.data(), oggBuffer.size(), &info) != 0) {
        return false;
    }

    // Allocate PCM buffer
    std::vector<char> pcm_data(info.pcm_data_size);
    
    // Convert to PCM
    if (convert_vorbis_to_pcm_from_memory(oggBuffer.data(), oggBuffer.size(), pcm_data.data(), &info.pcm_data_size) != 0) {
        return false;
    }

    // Create WAV in memory
    unsigned char* wav_data = nullptr;
    int wav_size = 0;
    if (create_wav_in_memory(pcm_data.data(), info.pcm_data_size, info.sample_rate, info.channels, &wav_data, &wav_size) != 0) {
        return false;
    }

    // Copy WAV data to output buffer
    wavBuffer.resize(wav_size);
    memcpy(wavBuffer.data(), wav_data, wav_size);

    // Clean up
    free(wav_data);

    return true;
} 

bool VorbisWrapper::GetVorbisInfo(const std::vector<uint8_t>& oggBuffer, VorbisInfo& info) {
    if (oggBuffer.empty()) {
        return false;
    }

    if (get_vorbis_info_from_memory(oggBuffer.data(), oggBuffer.size(), &info) != 0) {
        return false;
    }

    return true;
} 

bool VorbisWrapper::ConvertOggToPCM(const std::vector<uint8_t>& oggBuffer, std::vector<uint8_t>& pcmBuffer) {
    if (oggBuffer.empty()) {
        return false;
    }

    VorbisInfo info;
    if (get_vorbis_info_from_memory(oggBuffer.data(), oggBuffer.size(), &info) != 0) {
        return false;
    }

    // Allocate PCM buffer
    pcmBuffer.resize(info.pcm_data_size);
    
    // Convert to PCM
    if (convert_vorbis_to_pcm_from_memory(oggBuffer.data(), oggBuffer.size(), 
        reinterpret_cast<char*>(pcmBuffer.data()), &info.pcm_data_size) != 0) {
        return false;
    }

    // Resize buffer to actual PCM data size
    pcmBuffer.resize(info.pcm_data_size);

    return true;
} 