#pragma once
#include "decode.h"
#ifdef __cplusplus
extern "C" {
#endif

int sdl_audio_init(void **audio_render, int samplerate, int channels, int format, int buffer);
void sdl_audio_release(void *audio_render);
void sdl_audio_set_dec(void *audio_render, decoder *dec);

#ifdef __cplusplus
}
#endif
