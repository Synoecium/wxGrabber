#define OGG_IMPL
#define VORBIS_IMPL
#include "../../include/vorbis/minivorbis.h"
#include "../../include/vorbis/minivorbis_impl.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFFER_SIZE (1024 * 1024)

// Memory buffer structure for callbacks
typedef struct {
    const unsigned char* data;
    size_t size;
    size_t position;
} MemoryBuffer;

// Callback functions for memory buffer
static size_t memory_read(void* ptr, size_t size, size_t nmemb, void* datasource) {
    MemoryBuffer* buffer = (MemoryBuffer*)datasource;
    size_t bytes_to_read = size * nmemb;
    
    if (buffer->position + bytes_to_read > buffer->size) {
        bytes_to_read = buffer->size - buffer->position;
    }
    
    if (bytes_to_read > 0) {
        memcpy(ptr, buffer->data + buffer->position, bytes_to_read);
        buffer->position += bytes_to_read;
    }
    
    return bytes_to_read / size;
}

static int memory_seek(void* datasource, ogg_int64_t offset, int whence) {
    MemoryBuffer* buffer = (MemoryBuffer*)datasource;
    
    switch (whence) {
        case SEEK_SET:
            buffer->position = offset;
            break;
        case SEEK_CUR:
            buffer->position += offset;
            break;
        case SEEK_END:
            buffer->position = buffer->size + offset;
            break;
        default:
            return -1;
    }
    
    if (buffer->position > buffer->size) {
        buffer->position = buffer->size;
        return -1;
    }
    
    return 0;
}

static int memory_close(void* datasource) {
    return 0;  // Nothing to close
}

static long memory_tell(void* datasource) {
    return ((MemoryBuffer*)datasource)->position;
}

// Custom callbacks for memory buffer
static ov_callbacks memory_callbacks = {
    memory_read,
    memory_seek,
    memory_close,
    memory_tell
};

// WAV header structure
#pragma pack(push, 1)
typedef struct {
    char riff[4];                // "RIFF"
    uint32_t chunk_size;         // File size - 8
    char wave[4];                // "WAVE"
    char fmt[4];                 // "fmt "
    uint32_t fmt_chunk_size;     // 16 for PCM
    uint16_t audio_format;       // 1 for PCM
    uint16_t num_channels;       // Number of channels
    uint32_t sample_rate;        // Sample rate
    uint32_t byte_rate;          // Sample rate * channels * bits per sample / 8
    uint16_t block_align;        // Channels * bits per sample / 8
    uint16_t bits_per_sample;    // Bits per sample
    char data[4];                // "data"
    uint32_t data_chunk_size;    // Size of audio data
} WavHeader;
#pragma pack(pop)


int convert_vorbis_to_pcm_from_memory(const unsigned char* data, size_t size, char* pcm_data, int* pcm_data_size) {
    MemoryBuffer buffer = {data, size, 0};
    
    /* Open sound stream. */
    OggVorbis_File vorbis;
    if(ov_open_callbacks(&buffer, &vorbis, NULL, 0, memory_callbacks) != 0) {
        printf("Invalid Ogg data.");
        return -1;
    }
    
    /* Get sound information */
    vorbis_info* info = ov_info(&vorbis, -1);
    if(!info) {
        ov_clear(&vorbis);
        return -1;
    }
    
    /* Allocate buffer for reading */
    unsigned char* read_buffer = (unsigned char*)malloc(BUFFER_SIZE);
    if (!read_buffer) {
        ov_clear(&vorbis);
        return -1;
    }
    
    /* Read the entire sound stream into PCM buffer */
    int section = 0;
    long bytes;
    *pcm_data_size = 0;
    
    while((bytes = ov_read(&vorbis, read_buffer, BUFFER_SIZE, 0, 2, 1, &section)) > 0) {
        if(*pcm_data_size + bytes > BUFFER_SIZE * 1000) { // Safety check
            free(read_buffer);
            ov_clear(&vorbis);
            return -1;
        }
        memcpy(pcm_data + *pcm_data_size, read_buffer, bytes);
        *pcm_data_size += bytes;
    }
    
    /* Cleanup */
    free(read_buffer);
    ov_clear(&vorbis);
    return 0;
}

int get_vorbis_info_from_memory(const unsigned char* data, size_t size, VorbisInfo* res_info) {
    MemoryBuffer buffer = {data, size, 0};
    
    /* Open sound stream. */
    OggVorbis_File vorbis;
    if(ov_open_callbacks(&buffer, &vorbis, NULL, 0, memory_callbacks) != 0) {
        printf("Invalid Ogg data.");
        return -1;
    }
    
    /* Get sound information */
    vorbis_info* info = ov_info(&vorbis, -1);
    if(!info) {
        ov_clear(&vorbis);
        return -1;
    }

    /* Calculate total PCM size */
    ogg_int64_t total_samples = ov_pcm_total(&vorbis, -1);
    if(total_samples < 0) {
        ov_clear(&vorbis);
        return -1;
    }

    /* PCM size = total samples * channels * bytes per sample */
    res_info->pcm_data_size = (int)(total_samples * info->channels * 2); // 2 bytes per sample for 16-bit PCM
    res_info->sample_rate = info->rate;
    res_info->channels = info->channels;
    res_info->duration_ms = (int)(total_samples * 1000 / info->rate);
    
    /* Close sound file */
    ov_clear(&vorbis);
    return 0;
}

int create_wav_in_memory(const char* pcm_data, int pcm_size, int sample_rate, int channels, 
                        unsigned char** wav_data, int* wav_size) {
    // Calculate total WAV size
    *wav_size = sizeof(WavHeader) + pcm_size;
    
    // Allocate memory for WAV data
    *wav_data = (unsigned char*)malloc(*wav_size);
    if (!*wav_data) {
        return -1;
    }

    // Prepare WAV header
    WavHeader header = {0};
    memcpy(header.riff, "RIFF", 4);
    header.chunk_size = pcm_size + sizeof(WavHeader) - 8;
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    header.fmt_chunk_size = 16;
    header.audio_format = 1;  // PCM
    header.num_channels = channels;
    header.sample_rate = sample_rate;
    header.bits_per_sample = 16;  // 16-bit PCM
    header.block_align = channels * (header.bits_per_sample / 8);
    header.byte_rate = sample_rate * header.block_align;
    memcpy(header.data, "data", 4);
    header.data_chunk_size = pcm_size;

    // Copy header to output buffer
    memcpy(*wav_data, &header, sizeof(WavHeader));
    
    // Copy PCM data after header
    memcpy(*wav_data + sizeof(WavHeader), pcm_data, pcm_size);

    return 0;
}
