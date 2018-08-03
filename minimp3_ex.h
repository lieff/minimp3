#ifndef MINIMP3_EXT_H
#define MINIMP3_EXT_H
/*
    https://github.com/lieff/minimp3
    To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
    This software is distributed without any warranty.
    See <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include <minimp3.h>

typedef struct
{
    short *buffer;
    size_t samples;
    int channels, hz, layer, avg_bitrate_kbps;
} mp3dec_file_info_t;

typedef int (*MP3D_ITERATE_CB)(mp3dec_t *dec, void *user, const void *frame, int frame_size, mp3dec_frame_info_t *info);

#ifdef __cplusplus
extern "C" {
#endif

/* decode whole file/buffer block */
int mp3dec_load(mp3dec_t *dec, const char *file_name, mp3dec_file_info_t *info);
int mp3dec_load_buf(mp3dec_t *dec, const void *buf, size_t buf_size, mp3dec_file_info_t *info);
/* iterate through frames with optional decoding */
int mp3dec_iterate(mp3dec_t *dec, const char *file_name, MP3D_ITERATE_CB *cb, void *user);
int mp3dec_iterate_buf(mp3dec_t *dec, const void *buf, size_t buf_size, MP3D_ITERATE_CB *cb, void *user);

#ifdef __cplusplus
}
#endif
#endif /*MINIMP3_EXT_H*/

#ifdef MINIMP3_EXT_IMPLEMENTATION

int mp3dec_load(mp3dec_t *dec, const char *file_name, mp3dec_file_info_t *info)
{
    return 0;
}

int mp3dec_load_buf(mp3dec_t *dec, const void *buf, size_t buf_size, mp3dec_file_info_t *info)
{
    return 0;
}

int mp3dec_iterate(mp3dec_t *dec, const char *file_name, MP3D_ITERATE_CB *cb)
{
    return 0;
}

int mp3dec_iterate_buf(mp3dec_t *dec, const void *buf, size_t buf_size, MP3D_ITERATE_CB *cb)
{
    return 0;
}

#endif /*MINIMP3_EXT_IMPLEMENTATION*/
