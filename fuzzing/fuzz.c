#define MINIMP3_IMPLEMENTATION
#include "../minimp3.h"
#include <stdio.h>

int main()
{
    static mp3dec_t mp3d;
    mp3dec_frame_info_t info;
    unsigned char buf[4096];

#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
    while (__AFL_LOOP(1000))
#endif
    {
        int nbuf = 0;
        mp3dec_init(&mp3d);
        do
        {
            short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
            nbuf += fread(buf + nbuf, 1, sizeof(buf) - nbuf, stdin);
            mp3dec_decode_frame(&mp3d, buf, nbuf, pcm, &info);
            nbuf -= info.frame_bytes;
        } while (info.frame_bytes);
    }

    return 0;
}
