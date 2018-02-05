#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#define MINIMP3_IMPLEMENTATION
#include "../minimp3.h"
#include "decode.h"

static void add_audio(decoder *dec, short *buf, int bytes)
{
    char *buf_new = 0;
    if (dec->mp3_allocated < (dec->mp3_size + bytes))
    {
        int allocated = dec->mp3_allocated;
        dec->mp3_allocated = dec->mp3_allocated ? dec->mp3_allocated*2 : 4*1024*1024;
        buf_new = (char *)malloc(dec->mp3_allocated);
        memcpy(buf_new, dec->mp3_buf, allocated);
    }
    EnterCriticalSection(&dec->mp3_lock);
    if (buf_new)
    {
        if (dec->mp3_buf)
            free(dec->mp3_buf);
        dec->mp3_buf = (short *)buf_new;
    }
    memcpy((char*)dec->mp3_buf + dec->mp3_size, buf, bytes);
    dec->mp3_size += bytes;
    LeaveCriticalSection(&dec->mp3_lock);
}

static void *load_mp3_thread(void *lpThreadParameter)
{
    static mp3dec_t mp3d;
    mp3dec_frame_info_t info = { 0, };
    struct stat st;
    decoder *dec = (decoder *)lpThreadParameter;
    unsigned char *inputBuffer, *buf_mp3;
    int file, mp3_size, samples;
retry_open:
    file = open(dec->file_name, O_RDONLY);
    if (file < 0 && (errno == EAGAIN || errno == EINTR))
        goto retry_open;
    free((void*)dec->file_name);
    dec->file_name = 0;
    if (file < 0 || fstat(file, &st) < 0)
        return (void*)(size_t)errno;

    dec->mp3_file_size = st.st_size;
retry_mmap:
    inputBuffer = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, file, 0);
    if (MAP_FAILED == inputBuffer && (errno == EAGAIN || errno == EINTR))
        goto retry_mmap;
    if (MAP_FAILED == inputBuffer)
    {
        close(file);
        return (void*)(size_t)errno;
    }
    buf_mp3 = inputBuffer;
    mp3_size = dec->mp3_file_size;
    mp3dec_init(&mp3d);
    do
    {
        short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        samples = mp3dec_decode_frame(&mp3d, buf_mp3, mp3_size, pcm, &info);
        if (samples)
        {
            add_audio(dec, pcm, samples*2*info.channels);
            if (!dec->mp3_rate)
                dec->mp3_rate = info.hz;
            if (!dec->mp3_channels)
                dec->mp3_channels = info.channels;
            if (dec->mp3_rate != info.hz || dec->mp3_channels != info.channels)
                break;
        }
        buf_mp3  += info.frame_bytes;
        mp3_size -= info.frame_bytes;
    } while (info.frame_bytes);

    munmap(inputBuffer, st.st_size);
    close(file);
    return 0;
}

int preload_mp3(decoder *dec, const char *file_name)
{
    if (dec->mp3_open_thread)
    {
        thread_wait(dec->mp3_open_thread);
        thread_close(dec->mp3_open_thread);
        dec->mp3_open_thread = 0;
    }
    EnterCriticalSection(&dec->mp3_lock);
    if (dec->mp3_buf)
    {
        free(dec->mp3_buf);
        dec->mp3_buf = 0;
        dec->mp3_size = 0;
        dec->mp3_pos = 0;
        dec->mp3_allocated = 0;
        dec->mp3_rate = 0;
        dec->mp3_channels = 0;
        dec->mp3_duration = 0;
    }
    LeaveCriticalSection(&dec->mp3_lock);

    if (!file_name || !*file_name)
        return 0;

    dec->file_name = strdup(file_name);
    if (!dec->file_name)
        return 0;
    dec->mp3_open_thread = thread_create(load_mp3_thread, dec);
    if (!dec->mp3_open_thread)
    {
        free((void*)dec->file_name);
        dec->file_name = 0;
        return 0;
    }
    return 1;
}
