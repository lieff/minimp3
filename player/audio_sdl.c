#include <stddef.h>
#include <stdlib.h>
#include <sys/param.h>
#include <SDL2/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "audio_sdl.h"

#define SDL2

typedef struct audio_ctx
{
#ifdef SDL2
    int dev;
#endif
    SDL_AudioSpec outputSpec;
    decoder *dec;
} audio_ctx;

static void audio_cb(void *udata, Uint8 *stream, int len)
{
    audio_ctx *ctx = (audio_ctx *)udata;
    decode_samples(ctx->dec, stream, len);
}

int sdl_audio_init(void **audio_render, int samplerate, int channels, int format, int buffer)
{
    *audio_render = 0;
    audio_ctx *ctx = calloc(1, sizeof(audio_ctx));
    SDL_AudioSpec wanted;
    memset(&wanted, 0, sizeof(wanted));
    wanted.freq = samplerate;
    wanted.format = format ? AUDIO_F32 : AUDIO_S16;
    wanted.channels = channels;
    wanted.samples = buffer ? buffer : 4096;
    wanted.callback = audio_cb;
    wanted.userdata = ctx;
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        printf("error: sdl init failed: %s\n", SDL_GetError());
        return 0;
    }
#ifdef SDL2
    int dev = SDL_OpenAudioDevice(NULL, 0, &wanted, &ctx->outputSpec, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (dev <= 0)
    {
        printf("error: couldn't open audio: %s\n", SDL_GetError());
        free(ctx);
        return 0;
    }
    ctx->dev = dev;
#else
    if (SDL_OpenAudio(&wanted, &ctx->outputSpec) < 0)
    {
        printf("error: couldn't open audio: %s\n", SDL_GetError());
        return 0;
    }
#endif
    *audio_render = ctx;
    return 1;
}

void sdl_audio_release(void *audio_render)
{
    audio_ctx *ctx = (audio_ctx *)audio_render;
#ifdef SDL2
    if (ctx->dev)
    {
        SDL_CloseAudioDevice(ctx->dev);
        ctx->dev = 0;
    }
#else
    SDL_PauseAudio(1);
    SDL_CloseAudio();
#endif
    free(ctx);
}

void sdl_audio_set_dec(void *audio_render, decoder *dec)
{
    audio_ctx *ctx = (audio_ctx *)audio_render;
#ifdef SDL2
    SDL_PauseAudioDevice(ctx->dev, 1);
#else
    SDL_PauseAudio(1);
#endif
    ctx->dec = dec;
    if (dec)
    {
#ifdef SDL2
        SDL_PauseAudioDevice(ctx->dev, 0);
#else
        SDL_PauseAudio(0);
#endif
    }
}
