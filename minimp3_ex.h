#ifndef MINIMP3_EXT_H
#define MINIMP3_EXT_H
/*
    https://github.com/lieff/minimp3
    To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide.
    This software is distributed without any warranty.
    See <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#include "minimp3.h"

#define MP3D_SEEK_TO_BYTE   0
#define MP3D_SEEK_TO_SAMPLE 1

#define MINIMP3_PREDECODE_FRAMES 2 /* frames to pre-decode and skip after seek (to fill internal structures) */
/*#define MINIMP3_SEEK_IDX_LINEAR_SEARCH*/ /* define to use linear index search instead of binary search on seek */

#define MP3D_E_MEMORY -1

typedef struct
{
    mp3d_sample_t *buffer;
    size_t samples; /* channels included, byte size = samples*sizeof(int16_t) */
    int channels, hz, layer, avg_bitrate_kbps;
} mp3dec_file_info_t;

typedef struct
{
    const uint8_t *buffer;
    size_t size;
} mp3dec_map_info_t;

typedef struct
{
    uint64_t sample;
    uint64_t offset;
} mp3dec_frame_t;

typedef struct
{
    mp3dec_frame_t *frames;
    size_t num_frames, capacity;
} mp3dec_index_t;

/*typedef int (*MP3D_READ_CB)(void *buf, size_t size, void *user_data);*/

typedef struct
{
    mp3dec_t mp3d;
    mp3dec_map_info_t file;
    mp3dec_index_t index;
    uint64_t offset, samples, start_offset;
    mp3dec_frame_info_t info;
    mp3d_sample_t buffer[MINIMP3_MAX_SAMPLES_PER_FRAME];
#ifndef MINIMP3_NO_STDIO
    int is_file;
#endif
    int free_format_bytes;
    int seek_method, vbr_tag_found;
    int buffer_samples, buffer_consumed, to_skip;
} mp3dec_ex_t;

typedef int (*MP3D_ITERATE_CB)(void *user_data, const uint8_t *frame, int frame_size, int free_format_bytes, size_t offset, mp3dec_frame_info_t *info);
typedef int (*MP3D_PROGRESS_CB)(void *user_data, size_t file_size, size_t offset, mp3dec_frame_info_t *info);

#ifdef __cplusplus
extern "C" {
#endif

/* decode whole buffer block */
void mp3dec_load_buf(mp3dec_t *dec, const uint8_t *buf, size_t buf_size, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data);
/* iterate through frames */
size_t mp3dec_iterate_buf(const uint8_t *buf, size_t buf_size, MP3D_ITERATE_CB callback, void *user_data);
/* streaming decoder with seeking capability */
int mp3dec_ex_open_buf(mp3dec_ex_t *dec, const uint8_t *buf, size_t buf_size, int seek_method);
/*int mp3dec_ex_open_cb(mp3dec_ex_t *dec, MP3D_READ_CB cb, void *user_data, uint64_t file_size, int seek_method);*/
void mp3dec_ex_close(mp3dec_ex_t *dec);
int mp3dec_ex_seek(mp3dec_ex_t *dec, uint64_t position);
size_t mp3dec_ex_read(mp3dec_ex_t *dec, int16_t *buf, size_t samples);
#ifndef MINIMP3_NO_STDIO
/* stdio versions with file pre-load */
int mp3dec_load(mp3dec_t *dec, const char *file_name, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data);
int mp3dec_iterate(const char *file_name, MP3D_ITERATE_CB callback, void *user_data);
int mp3dec_ex_open(mp3dec_ex_t *dec, const char *file_name, int seek_method);
#endif

#ifdef __cplusplus
}
#endif
#endif /*MINIMP3_EXT_H*/

#ifdef MINIMP3_IMPLEMENTATION

static void mp3dec_skip_id3(const uint8_t **pbuf, size_t *pbuf_size)
{
    char *buf = (char *)(*pbuf);
    size_t buf_size = *pbuf_size;
#ifndef MINIMP3_NOSKIP_ID3V2
    if (buf_size > 10 && !memcmp(buf, "ID3", 3) && !((buf[5] & 15) || (buf[6] & 0x80) || (buf[7] & 0x80) || (buf[8] & 0x80) || (buf[9] & 0x80)))
    {
        size_t id3v2size = (((buf[6] & 0x7f) << 21) | ((buf[7] & 0x7f) << 14) | ((buf[8] & 0x7f) << 7) | (buf[9] & 0x7f)) + 10;
        if ((buf[5] & 16))
            id3v2size += 10; /* footer */
        if (id3v2size >= buf_size)
            id3v2size = buf_size;
        buf      += id3v2size;
        buf_size -= id3v2size;
    }
#endif
#ifndef MINIMP3_NOSKIP_ID3V1
    if (buf_size > 128 && !memcmp(buf + buf_size - 128, "TAG", 3))
    {
        buf_size -= 128;
        if (buf_size > 227 && !memcmp(buf + buf_size - 227, "TAG+", 4))
            buf_size -= 227;
    }
#endif
#ifndef MINIMP3_NOSKIP_APEV2
    if (buf_size > 32 && !memcmp(buf + buf_size - 32, "APETAGEX", 8))
    {
        buf_size -= 32;
    }
#endif
    *pbuf = (const uint8_t *)buf;
    *pbuf_size = buf_size;
}

void mp3dec_load_buf(mp3dec_t *dec, const uint8_t *buf, size_t buf_size, mp3dec_file_info_t *info, MP3D_PROGRESS_CB progress_cb, void *user_data)
{
    size_t orig_buf_size = buf_size;
    mp3d_sample_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    mp3dec_frame_info_t frame_info;
    memset(info, 0, sizeof(*info));
    memset(&frame_info, 0, sizeof(frame_info));
    /* skip id3 */
    mp3dec_skip_id3(&buf, &buf_size);
    if (!buf_size)
        return;
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
    samples *= frame_info.channels;
    size_t allocated = (buf_size/frame_info.frame_bytes)*samples*sizeof(mp3d_sample_t) + MINIMP3_MAX_SAMPLES_PER_FRAME*sizeof(mp3d_sample_t);
    info->buffer = (mp3d_sample_t*)malloc(allocated);
    if (!info->buffer)
        return;
    info->samples = samples;
    memcpy(info->buffer, pcm, info->samples*sizeof(mp3d_sample_t));
    /* save info */
    info->channels = frame_info.channels;
    info->hz       = frame_info.hz;
    info->layer    = frame_info.layer;
    size_t avg_bitrate_kbps = frame_info.bitrate_kbps;
    size_t frames = 1;
    /* decode rest frames */
    int frame_bytes;
    do
    {
        if ((allocated - info->samples*sizeof(mp3d_sample_t)) < MINIMP3_MAX_SAMPLES_PER_FRAME*sizeof(mp3d_sample_t))
        {
            allocated *= 2;
            info->buffer = (mp3d_sample_t*)realloc(info->buffer, allocated);
        }
        samples = mp3dec_decode_frame(dec, buf, buf_size, info->buffer + info->samples, &frame_info);
        frame_bytes = frame_info.frame_bytes;
        buf      += frame_bytes;
        buf_size -= frame_bytes;
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
    } while (frame_bytes);
    /* reallocate to normal buffer size */
    if (allocated != info->samples*sizeof(mp3d_sample_t))
        info->buffer = (mp3d_sample_t*)realloc(info->buffer, info->samples*sizeof(mp3d_sample_t));
    info->avg_bitrate_kbps = avg_bitrate_kbps/frames;
}

size_t mp3dec_iterate_buf(const uint8_t *buf, size_t buf_size, MP3D_ITERATE_CB callback, void *user_data)
{
    size_t frames = 0;
    const uint8_t *orig_buf = buf;
    /* skip id3 */
    mp3dec_skip_id3(&buf, &buf_size);
    if (!buf_size)
        return 0;
    mp3dec_frame_info_t frame_info;
    memset(&frame_info, 0, sizeof(frame_info));
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
        frame_info.frame_bytes = frame_size;

        frames++;
        if (callback)
        {
            if (callback(user_data, hdr, frame_size, free_format_bytes, hdr - orig_buf, &frame_info))
                break;
        }
        buf      += frame_size;
        buf_size -= frame_size;
    } while (1);
    return frames;
}

static int mp3dec_load_index(void *user_data, const uint8_t *frame, int frame_size, int free_format_bytes, size_t offset, mp3dec_frame_info_t *info)
{
    static const char g_xing_tag[4] = { 'X', 'i', 'n', 'g' };
    static const char g_info_tag[4] = { 'I', 'n', 'f', 'o' };
    mp3dec_frame_t *idx_frame;
    mp3dec_ex_t *dec = (mp3dec_ex_t *)user_data;
    if (!dec->index.frames && !dec->vbr_tag_found)
    {   /* detect VBR tag and try to avoid full scan */
        int i;
        dec->info = *info;
        dec->start_offset = offset;
        dec->free_format_bytes = free_format_bytes; /* should not change */
        for (i = 5; i < frame_size - 12; i++)
        {
            const uint8_t *tag = frame + i;
            if (!memcmp(g_xing_tag, tag, 4) || !memcmp(g_info_tag, tag, 4))
            {
                if (!((tag[7] & 1))/* frames field present */)
                    break;
                uint64_t frames = (uint32_t)(tag[8] << 24) | (tag[9] << 16) | (tag[10] << 8) | tag[11];
                dec->samples += hdr_frame_samples(frame)*info->channels*frames;
                dec->vbr_tag_found = 1;
                return 1;
            }
        }
    }
    if (dec->index.num_frames + 1 > dec->index.capacity)
    {
        if (!dec->index.capacity)
            dec->index.capacity = 4096;
        else
            dec->index.capacity *= 2;
        dec->index.frames = (mp3dec_frame_t *)realloc((void*)dec->index.frames, sizeof(mp3dec_frame_t)*dec->index.capacity);
        if (!dec->index.frames)
            return MP3D_E_MEMORY;
    }
    idx_frame = &dec->index.frames[dec->index.num_frames++];
    idx_frame->offset = offset;
    idx_frame->sample = dec->samples;
    dec->samples += hdr_frame_samples(frame)*info->channels;
    return 0;
}

int mp3dec_ex_open_buf(mp3dec_ex_t *dec, const uint8_t *buf, size_t buf_size, int seek_method)
{
    memset(dec, 0, sizeof(*dec));
    dec->file.buffer = buf;
    dec->file.size   = buf_size;
    dec->seek_method = seek_method;
    mp3dec_init(&dec->mp3d);
    if (mp3dec_iterate_buf(dec->file.buffer, dec->file.size, mp3dec_load_index, dec) > 0 && !dec->index.frames && !dec->vbr_tag_found)
        return MP3D_E_MEMORY;
    return 0;
}

#ifndef MINIMP3_SEEK_IDX_LINEAR_SEARCH
static size_t mp3dec_idx_binary_search(mp3dec_index_t *idx, uint64_t position)
{
    size_t end = idx->num_frames, start = 0, index = 0;
    while (start <= end)
    {
        size_t mid = (start + end) / 2;
        if (idx->frames[mid].sample >= position)
        {   /* move left side. */
            if (idx->frames[mid].sample == position)
                return mid;
            end = mid - 1;
        }  else
        {   /* move to right side */
            index = mid;
            start = mid + 1;
            if (start == idx->num_frames)
                break;
        }
    }
    return index;
}
#endif

int mp3dec_ex_seek(mp3dec_ex_t *dec, uint64_t position)
{
    size_t i;
    if (MP3D_SEEK_TO_BYTE == dec->seek_method)
    {
        dec->offset = position;
        return 0;
    }
    if (0 == position)
    {   /* optimize seek to zero, no index needed */
seek_zero:
        dec->offset  = dec->start_offset;
        dec->to_skip = 0;
        goto do_exit;
    }
    if (!dec->index.frames && dec->vbr_tag_found)
    {   /* no index created yet (vbr tag used to calculate track length) */
        dec->samples = 0;
        if (mp3dec_iterate_buf(dec->file.buffer, dec->file.size, mp3dec_load_index, dec) > 0 && !dec->index.frames)
            return MP3D_E_MEMORY;
    }
    if (!dec->index.frames)
        goto seek_zero; /* no frames in file - seek to zero */
#ifdef MINIMP3_SEEK_IDX_LINEAR_SEARCH
    for (i = 0; i < dec->index.num_frames; i++)
    {
        if (dec->index.frames[i].sample >= position)
            break;
    }
#else
    i = mp3dec_idx_binary_search(&dec->index, position);
#endif
    if (i)
    {
        int to_fill_bytes = 511;
        int skip_frames = MINIMP3_PREDECODE_FRAMES
#ifdef MINIMP3_SEEK_IDX_LINEAR_SEARCH
         + ((dec->index.frames[i].sample == position) ? 0 : 1)
#endif
        ;
        i -= MINIMP3_MIN(i, (size_t)skip_frames);
        while (i && to_fill_bytes)
        {   /* make sure bit-reservoir is filled when we start decoding */
            const uint8_t *hdr = dec->file.buffer + dec->index.frames[i - 1].offset;
            int frame_size = hdr_frame_bytes(hdr, dec->free_format_bytes) + hdr_padding(hdr);
            to_fill_bytes -= MINIMP3_MIN(to_fill_bytes, frame_size);
            i--;
        }
    }
    dec->offset = dec->index.frames[i].offset;
    dec->to_skip = position - dec->index.frames[i].sample;
do_exit:
    dec->buffer_samples  = 0;
    dec->buffer_consumed = 0;
    mp3dec_init(&dec->mp3d);
    return 0;
}

size_t mp3dec_ex_read(mp3dec_ex_t *dec, int16_t *buf, size_t samples)
{
    size_t samples_requested = samples;
    mp3dec_frame_info_t frame_info;
    memset(&frame_info, 0, sizeof(frame_info));
    if (dec->buffer_consumed < dec->buffer_samples)
    {
        size_t to_copy = MINIMP3_MIN((size_t)(dec->buffer_samples - dec->buffer_consumed), samples);
        memcpy(buf, dec->buffer + dec->buffer_consumed, to_copy*sizeof(int16_t));
        buf += to_copy;
        dec->buffer_consumed += to_copy;
        samples -= to_copy;
    }
    while (samples)
    {
        const uint8_t *dec_buf = dec->file.buffer + dec->offset;
        size_t buf_size = dec->file.size - dec->offset;
        if (!buf_size)
            break;
        dec->buffer_samples = mp3dec_decode_frame(&dec->mp3d, dec_buf, buf_size, dec->buffer, &frame_info);
        dec->buffer_consumed = 0;
        if (dec->buffer_samples)
        {
            dec->buffer_samples *= frame_info.channels;
            if (dec->to_skip)
            {
                size_t skip = MINIMP3_MIN(dec->buffer_samples, dec->to_skip);
                dec->buffer_consumed += skip;
                dec->to_skip -= skip;
            }
            size_t to_copy = MINIMP3_MIN((size_t)(dec->buffer_samples - dec->buffer_consumed), samples);
            memcpy(buf, dec->buffer + dec->buffer_consumed, to_copy*sizeof(int16_t));
            buf += to_copy;
            dec->buffer_consumed += to_copy;
            samples -= to_copy;
        } else if (dec->to_skip)
        {   /* In mp3 decoding not always can start decode from any frame because of bit reservoir,
               count skip samples for such frames */
            int frame_samples = hdr_frame_samples(dec_buf)*frame_info.channels;
            dec->to_skip -= MINIMP3_MIN(frame_samples, dec->to_skip);
        }
        dec->offset += frame_info.frame_bytes;
    }
    return samples_requested - samples;
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

static void mp3dec_close_file(mp3dec_map_info_t *map_info)
{
    if (map_info->buffer && MAP_FAILED != map_info->buffer)
        munmap((void *)map_info->buffer, map_info->size);
    map_info->buffer = 0;
    map_info->size   = 0;
}

static int mp3dec_open_file(const char *file_name, mp3dec_map_info_t *map_info)
{
    int file;
    struct stat st;
    memset(map_info, 0, sizeof(*map_info));
retry_open:
    file = open(file_name, O_RDONLY);
    if (file < 0 && (errno == EAGAIN || errno == EINTR))
        goto retry_open;
    if (file < 0 || fstat(file, &st) < 0)
    {
        close(file);
        return -1;
    }

    map_info->size = st.st_size;
retry_mmap:
    map_info->buffer = (const uint8_t *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, file, 0);
    if (MAP_FAILED == map_info->buffer && (errno == EAGAIN || errno == EINTR))
        goto retry_mmap;
    close(file);
    if (MAP_FAILED == map_info->buffer)
        return -1;
    return 0;
}
#elif defined(_WIN32)
#include <windows.h>

static void mp3dec_close_file(mp3dec_map_info_t *map_info)
{
    if (map_info->buffer)
        UnmapViewOfFile(map_info->buffer);
    map_info->buffer = 0;
    map_info->size = 0;
}

static int mp3dec_open_file(const char *file_name, mp3dec_map_info_t *map_info)
{
    memset(map_info, 0, sizeof(*map_info));

    HANDLE mapping = NULL;
    HANDLE file = CreateFileA(file_name, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (INVALID_HANDLE_VALUE == file)
        return -1;
    LARGE_INTEGER s;
    s.LowPart = GetFileSize(file, (DWORD*)&s.HighPart);
    if (s.LowPart == INVALID_FILE_SIZE && GetLastError() != NO_ERROR)
        goto error;
    map_info->size = s.QuadPart;

    mapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (!mapping)
        goto error;
    map_info->buffer = (const uint8_t*) MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, s.QuadPart);
    CloseHandle(mapping);
    if (!map_info->buffer)
        goto error;

    CloseHandle(file);
    return 0;
error:
    mp3dec_close_file(map_info);
    CloseHandle(file);
    return -1;
}
#else
#include <stdio.h>

static void mp3dec_close_file(mp3dec_map_info_t *map_info)
{
    if (map_info->buffer)
        free((void *)map_info->buffer);
    map_info->buffer = 0;
    map_info->size = 0;
}

static int mp3dec_open_file(const char *file_name, mp3dec_map_info_t *map_info)
{
    memset(map_info, 0, sizeof(*map_info));
    FILE *file = fopen(file_name, "rb");
    if (!file)
        return -1;
    long size = -1;
    if (fseek(file, 0, SEEK_END))
        goto error;
    size = ftell(file);
    if (size < 0)
        goto error;
    map_info->size = (size_t)size;
    if (fseek(file, 0, SEEK_SET))
        goto error;
    map_info->buffer = (uint8_t *)malloc(map_info->size);
    if (!map_info->buffer)
        goto error;
    if (fread((void *)map_info->buffer, 1, map_info->size, file) != map_info->size)
        goto error;
    fclose(file);
    return 0;
error:
    mp3dec_close_file(map_info);
    fclose(file);
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
    return info->samples ? 0 : -1;
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

void mp3dec_ex_close(mp3dec_ex_t *dec)
{
    if (dec->is_file)
        mp3dec_close_file(&dec->file);
    if (dec->index.frames)
        free(dec->index.frames);
    memset(dec, 0, sizeof(*dec));
}

int mp3dec_ex_open(mp3dec_ex_t *dec, const char *file_name, int seek_method)
{
    int ret;
    if ((ret = mp3dec_open_file(file_name, &dec->file)))
        return ret;
    ret = mp3dec_ex_open_buf(dec, dec->file.buffer, dec->file.size, seek_method);
    dec->is_file = 1;
    return ret;
}
#else /* MINIMP3_NO_STDIO */
void mp3dec_ex_close(mp3dec_ex_t *dec)
{
    if (dec->index.frames)
        free(dec->index.frames);
    memset(dec, 0, sizeof(*dec));
}
#endif

#endif /*MINIMP3_IMPLEMENTATION*/
