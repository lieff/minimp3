#pragma once
#include <stdint.h>
#include "../minimp3.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef int (*PARSE_GET_FILE_CB)(void *user, char **file_name);
typedef int (*PARSE_INFO_CB)(void *user, char *file_name, int rate, int mp3_channels, float duration);

typedef struct decoder
{
    mp3dec_t mp3d;
    mp3dec_frame_info_t info;
    uint8_t *mp3_buf;
    size_t mp3_size, pos;
    int file, pcm_bytes, pcm_copied, mp3_rate, mp3_channels;
    float mp3_duration;
    float spectrum[32][2]; // for visualization
    short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
} decoder;

extern decoder _dec;

int open_dec(decoder *dec, const char *file_name);
int close_dec(decoder *dec);
void decode_samples(decoder *dec, uint8_t *buf, int bytes);
void start_parser(PARSE_GET_FILE_CB fcb, void *fcb_user, PARSE_INFO_CB icb, void *icb_user);

#ifdef __cplusplus
}
#endif
