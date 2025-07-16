#ifndef MINIVORBIS_IMPL_H
#define MINIVORBIS_IMPL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int sample_rate;
    int channels;
    int bitrate;
    int pcm_data_size;
    int duration_ms;
} VorbisInfo;

int get_vorbis_info_from_memory(const unsigned char* data, size_t size, VorbisInfo* res_info);
int convert_vorbis_to_pcm_from_memory(const unsigned char* data, size_t size, char* pcm_data, int* pcm_data_size);
int create_wav_in_memory(const char* pcm_data, int pcm_size, int sample_rate, int channels, 
                        unsigned char** wav_data, int* wav_size);

#ifdef __cplusplus
}
#endif

#endif
