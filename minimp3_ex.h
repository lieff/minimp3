#ifndef MINIMP3_EXT_H
#define MINIMP3_EXT_H
/*
    https://github.com/lieff/minimp3
    To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
    This software is distributed without any warranty.
    See <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "minimp3.h"

typedef struct
{
    int16_t *buffer;
    size_t samples; /* channels included, byte size = samples*sizeof(int16_t) */
    int channels, hz, layer, avg_bitrate_kbps;
} mp3dec_file_info_t;

typedef int (*MP3D_ITERATE_CB)(void *user_data, const uint8_t *frame, int frame_size, size_t offset, mp3dec_frame_info_t *info);
typedef int (*MP3D_PROGRESS_CB)(void *user_data, size_t file_size, size_t offset, mp3dec_frame_info_t *info);

#ifdef __cplusplus
extern "C" {
#endif

/* decode whole buffer block */
void mp3dec_load_buf(mp3dec_t *dec, const uint8_t *buf, size_t buf_size, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data);
/* iterate through frames with optional decoding */
void mp3dec_iterate_buf(const uint8_t *buf, size_t buf_size, MP3D_ITERATE_CB callback, void *user_data);
#ifndef MINIMP3_NO_STDIO
/* stdio versions with file pre-load */
int mp3dec_load(mp3dec_t *dec, const char *file_name, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data);
int mp3dec_iterate(const char *file_name, MP3D_ITERATE_CB callback, void *user_data);
#endif

#ifdef __cplusplus
}
#endif
#endif /*MINIMP3_EXT_H*/

#ifdef MINIMP3_IMPLEMENTATION

static size_t mp3dec_skip_id3v2(const uint8_t *buf, size_t buf_size)
{
    if (buf_size > 10 && !strncmp((char *)buf, "ID3", 3))
    {
        return (((buf[6] & 0x7f) << 21) | ((buf[7] & 0x7f) << 14) |
            ((buf[8] & 0x7f) << 7) | (buf[9] & 0x7f)) + 10;
    }
    return 0;
}

void mp3dec_load_buf(mp3dec_t *dec, const uint8_t *buf, size_t buf_size, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    size_t orig_buf_size = buf_size;
    short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    mp3dec_frame_info_t frame_info;
    memset(info, 0, sizeof(*info));
    memset(&frame_info, 0, sizeof(frame_info));
    /* skip id3v2 */
    size_t id3v2size = mp3dec_skip_id3v2(buf, buf_size);
    if (id3v2size > buf_size)
        return;
    buf      += id3v2size;
    buf_size -= id3v2size;
    /* try to make allocation size assumption by first frame */
    mp3dec_init(dec);
    int samples;
    do
    {
        samples = mp3dec_decode_frame(dec, buf, buf_size, pcm, &frame_info);
        buf      += frame_info.frame_bytes;
        buf_size -= frame_info.frame_bytes;
        if (samples)
            break;
    } while (frame_info.frame_bytes);
    if (!samples)
        return;
    info->samples = samples*frame_info.channels;
    size_t allocated = (buf_size/frame_info.frame_bytes)*info->samples*2 + MINIMP3_MAX_SAMPLES_PER_FRAME*2;
    info->buffer = malloc(allocated);
    memcpy(info->buffer, pcm, info->samples*2);
    if (!info->buffer)
        return;
    /* save info */
    info->channels = frame_info.channels;
    info->hz       = frame_info.hz;
    info->layer    = frame_info.layer;
    size_t avg_bitrate_kbps = frame_info.bitrate_kbps;
    size_t frames = 1;
    /* decode rest frames */
    do
    {
        if ((allocated - info->samples*2) < MINIMP3_MAX_SAMPLES_PER_FRAME*2)
        {
            allocated *= 2;
            info->buffer = realloc(info->buffer, allocated);
        }
        samples = mp3dec_decode_frame(dec, buf, buf_size, info->buffer + info->samples, &frame_info);
        if (samples)
        {
            if (info->hz != frame_info.hz || info->layer != frame_info.layer)
                break;
            if (info->channels && info->channels != frame_info.channels)
#ifdef MINIMP3_ALLOW_MONO_STEREO_TRANSITION
                info->channels = 0; /* mark file with mono-stereo transition */
#else
                break;
#endif
            info->samples += samples*frame_info.channels;
            avg_bitrate_kbps += frame_info.bitrate_kbps;
            frames++;
            if (progress_cb)
                progress_cb(user_data, orig_buf_size, orig_buf_size - buf_size, &frame_info);
        }
        buf      += frame_info.frame_bytes;
        buf_size -= frame_info.frame_bytes;
    } while (frame_info.frame_bytes);
    /* reallocate to normal buffer size */
    if (allocated != info->samples*2)
        info->buffer = realloc(info->buffer, info->samples*2);
    info->avg_bitrate_kbps = avg_bitrate_kbps/frames;
}

void mp3dec_iterate_buf(const uint8_t *buf, size_t buf_size, MP3D_ITERATE_CB callback, void *user_data)
{
    if (!callback)
        return;
    mp3dec_frame_info_t frame_info;
    memset(&frame_info, 0, sizeof(frame_info));
    /* skip id3v2 */
    size_t id3v2size = mp3dec_skip_id3v2(buf, buf_size);
    if (id3v2size > buf_size)
        return;
    const uint8_t *orig_buf = buf;
    buf      += id3v2size;
    buf_size -= id3v2size;
    do
    {
        int free_format_bytes = 0, frame_size = 0;
        int i = mp3d_find_frame(buf, buf_size, &free_format_bytes, &frame_size);
        buf      += i;
        buf_size -= i;
        if (i && !frame_size)
            continue;
        if (!frame_size)
            break;
        const uint8_t *hdr = buf;
        frame_info.channels = HDR_IS_MONO(hdr) ? 1 : 2;
        frame_info.hz = hdr_sample_rate_hz(hdr);
        frame_info.layer = 4 - HDR_GET_LAYER(hdr);
        frame_info.bitrate_kbps = hdr_bitrate_kbps(hdr);

        if (callback(user_data, hdr, frame_size, hdr - orig_buf, &frame_info))
            break;
        buf      += frame_size;
        buf_size -= frame_size;
    } while (1);
}

#ifndef MINIMP3_NO_STDIO

#if defined(__linux__) || defined(__FreeBSD__)
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#if !defined(MAP_POPULATE) && defined(__linux__)
#define MAP_POPULATE 0x08000
#elif !defined(MAP_POPULATE)
#define MAP_POPULATE 0
#endif

typedef struct
{
    uint8_t *buffer;
    size_t size;
    int file;
} mp3dec_map_info_t;

static void mp3dec_close_file(mp3dec_map_info_t *map_info)
{
    if (map_info->buffer && MAP_FAILED != map_info->buffer)
        munmap(map_info->buffer, map_info->size);
    if (map_info->file)
        close(map_info->file);
    map_info->buffer = 0;
    map_info->file = 0;
}

static int mp3dec_open_file(const char *file_name, mp3dec_map_info_t *map_info)
{
    struct stat st;
    memset(map_info, 0, sizeof(*map_info));
retry_open:
    map_info->file = open(file_name, O_RDONLY);
    if (map_info->file < 0 && (errno == EAGAIN || errno == EINTR))
        goto retry_open;
    if (map_info->file < 0 || fstat(map_info->file, &st) < 0)
    {
        mp3dec_close_file(map_info);
        return -1;
    }

    map_info->size = st.st_size;
retry_mmap:
    map_info->buffer = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, map_info->file, 0);
    if (MAP_FAILED == map_info->buffer && (errno == EAGAIN || errno == EINTR))
        goto retry_mmap;
    if (MAP_FAILED == map_info->buffer)
    {
        mp3dec_close_file(map_info);
        return -1;
    }
    return 0;
}
#elif defined(_WIN32)
#include <windows.h>
typedef struct
{
    uint8_t *buffer;
    size_t size;
    HANDLE file;
} mp3dec_map_info_t;

static void mp3dec_close_file(mp3dec_map_info_t *map_info)
{
    if (map_info->buffer)
        UnmapViewOfFile(map_info->buffer);
    if (INVALID_HANDLE_VALUE != map_info->file)
        CloseHandle(map_info->file);
    map_info->buffer = 0;
    map_info->file = 0;
}

static int mp3dec_open_file(const char *file_name, mp3dec_map_info_t *map_info)
{
    memset(map_info, 0, sizeof(*map_info));

    map_info->file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (INVALID_HANDLE_VALUE == map_info->file)
        goto error;
    LARGE_INTEGER s;
    s.LowPart = GetFileSize(map_info->file, (DWORD*)&s.HighPart);
    if (s.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
        goto error;
    map_info->size = s.QuadPart;

    HANDLE mapping = CreateFileMapping(map_info->file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!mapping)
        goto error;
    map_info->buffer = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, s.QuadPart);
    CloseHandle(mapping);
    if (!map_info->buffer)
        goto error;

    return 0;
error:
    mp3dec_close_file(map_info);
    return -1;
}
#else
#include <stdio.h>
typedef struct
{
    uint8_t *buffer;
    FILE *file;
    size_t size;
} mp3dec_map_info_t;

static void mp3dec_close_file(mp3dec_map_info_t *map_info)
{
    if (map_info->buffer)
        free(map_info->buffer);
    if (map_info->file)
        fclose(map_info->file);
    map_info->buffer = 0;
    map_info->file = 0;
}

static int mp3dec_open_file(const char *file_name, mp3dec_map_info_t *map_info)
{
    memset(map_info, 0, sizeof(*map_info));
    map_info->file = fopen(file_name, "rb");
    if (!map_info->file)
        return -1;

    if (fseek(map_info->file, 0, SEEK_END))
        goto error;
    long size = ftell(map_info->file);
    if (size < 0)
        goto error;
    map_info->size = (size_t)size;
    if (fseek(map_info->file, 0, SEEK_SET))
        goto error;
    map_info->buffer = (uint8_t *)malloc(map_info->size);
    if (!map_info->buffer)
        goto error;
    if (fread(map_info->buffer, 1, map_info->size, map_info->file) != map_info->size)
        goto error;
    return 0;
error:
    mp3dec_close_file(map_info);
    return -1;
}
#endif

int mp3dec_load(mp3dec_t *dec, const char *file_name, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    int ret;
    mp3dec_map_info_t map_info;
    if ((ret = mp3dec_open_file(file_name, &map_info)))
        return ret;
    mp3dec_load_buf(dec, map_info.buffer, map_info.size, info, progress_cb, user_data);
    mp3dec_close_file(&map_info);
    return 0;
}

int mp3dec_iterate(const char *file_name, MP3D_ITERATE_CB callback, void *user_data)
{
    int ret;
    mp3dec_map_info_t map_info;
    if ((ret = mp3dec_open_file(file_name, &map_info)))
        return ret;
    mp3dec_iterate_buf(map_info.buffer, map_info.size, callback, user_data);
    mp3dec_close_file(&map_info);
    return 0;
}
#endif

#endif /*MINIMP3_IMPLEMENTATION*/
