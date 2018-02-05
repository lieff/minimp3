#pragma once
#include "system.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct decoder
{
    CRITICAL_SECTION mp3_lock;
    const char *file_name;
    short *mp3_buf;
    int mp3_size, mp3_pos, mp3_allocated, mp3_file_size, mp3_rate, mp3_channels;
    float mp3_duration;
    HANDLE mp3_open_thread;
} decoder;

extern decoder _dec;

int preload_mp3(decoder *dec, const char *file_name);

#ifdef __cplusplus
}
#endif
