#pragma once
#ifdef __cplusplus
extern "C" {
#endif

int sdl_audio_init(void **audio_render, int samplerate, int channels, int format, int buffer);
void sdl_audio_release(void *audio_render);

#ifdef __cplusplus
}
#endif
