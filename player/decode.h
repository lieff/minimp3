#pragma once
#include <stdint.h>
#include "../minimp3_ex.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef int (*PARSE_GET_FILE_CB)(void *user, char **file_name);
typedef int (*PARSE_INFO_CB)(void *user, char *file_name, int rate, int mp3_channels, float duration);

typedef struct decoder
{
    mp3dec_ex_t mp3d;
    float mp3_duration;
    float spectrum[32][2]; // for visualization
} decoder;

extern decoder _dec;

int open_dec(decoder *dec, const char *file_name);
int close_dec(decoder *dec);
void decode_samples(decoder *dec, uint8_t *buf, int bytes);

#ifdef __cplusplus
}
#endif
