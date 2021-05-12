#ifdef MINIMP3_TEST
static int malloc_num = 0, fail_malloc_num = -1;
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
static void *local_malloc(size_t size)
{
    /*printf("%d malloc(%d)\n", malloc_num, (int)size);*/
    if (fail_malloc_num == malloc_num)
        return 0;
    malloc_num++;
    return malloc(size);
}
#define malloc local_malloc
void *local_realloc(void *ptr, size_t new_size)
{
    /*printf("%d realloc(%d)\n", malloc_num, (int)new_size);*/
    if (fail_malloc_num == malloc_num)
        return 0;
    malloc_num++;
    return realloc(ptr, new_size);
}
#define realloc local_realloc
void *local_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset)
{
    /*printf("%d mmap(%d)\n", malloc_num, (int)length);*/
    if (fail_malloc_num == malloc_num)
        return MAP_FAILED;
    malloc_num++;
    return mmap(addr, length, prot, flags, fd, offset);
}
#define mmap local_mmap
#endif

/*#define MINIMP3_ONLY_MP3*/
/*#define MINIMP3_ONLY_SIMD*/
/*#define MINIMP3_NONSTANDARD_BUT_LOGICAL*/
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ALLOW_MONO_STEREO_TRANSITION
#include "minimp3_ex.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#if defined(_MSC_VER)
    #define strcasecmp(str1, str2) _strnicmp(str1, str2, strlen(str2))
#else
    #include <strings.h>
#endif

#define MODE_LOAD     0
#define MODE_LOAD_BUF 1
#define MODE_LOAD_CB  2
#define MODE_ITERATE  3
#define MODE_ITERATE_BUF 4
#define MODE_ITERATE_CB  5
#define MODE_STREAM     6
#define MODE_STREAM_BUF 7
#define MODE_STREAM_CB  8
#define MODE_DETECT     9
#define MODE_DETECT_BUF 10
#define MODE_DETECT_CB  11

static int16_t read16le(const void *p)
{
    const uint8_t *src = (const uint8_t *)p;
    return ((src[0]) << 0) | ((src[1]) << 8);
}

#ifndef MINIMP3_NO_WAV
static char *wav_header(int hz, int ch, int bips, int data_bytes)
{
    static char hdr[44] = "RIFFsizeWAVEfmt \x10\0\0\0\1\0ch_hz_abpsbabsdatasize";
    unsigned long nAvgBytesPerSec = bips*ch*hz >> 3;
    unsigned int nBlockAlign      = bips*ch >> 3;

    *(int32_t *)(void*)(hdr + 0x04) = 44 + data_bytes - 8;   /* File size - 8 */
    *(int16_t *)(void*)(hdr + 0x14) = 1;                     /* Integer PCM format */
    *(int16_t *)(void*)(hdr + 0x16) = ch;
    *(int32_t *)(void*)(hdr + 0x18) = hz;
    *(int32_t *)(void*)(hdr + 0x1C) = nAvgBytesPerSec;
    *(int16_t *)(void*)(hdr + 0x20) = nBlockAlign;
    *(int16_t *)(void*)(hdr + 0x22) = bips;
    *(int32_t *)(void*)(hdr + 0x28) = data_bytes;
    return hdr;
}
#endif

static unsigned char *preload(FILE *file, int *data_size)
{
    unsigned char *data;
    *data_size = 0;
    if (!file)
        return 0;
    if (fseek(file, 0, SEEK_END))
        return 0;
    *data_size = (int)ftell(file);
    if (*data_size < 0)
        return 0;
    if (fseek(file, 0, SEEK_SET))
        return 0;
    data = (unsigned char*)malloc(*data_size);
#define FAIL_MEM(data) \
    if (!(data)) \
    { \
        printf("error: not enough memory\n"); \
        exit(1); \
    }
    FAIL_MEM(data);
    if ((int)fread(data, 1, *data_size, file) != *data_size)
        exit(1);
    return data;
}

static int io_num, fail_io_num = -1;
static int wave_out = 0, mode = 0, position = 0, portion = 0, seek_to_byte = 0;

static size_t read_cb(void *buf, size_t size, void *user_data)
{
    /*printf("%d read_cb(%d)\n", io_num, (int)size);*/
    if (fail_io_num == io_num++)
        return -1;
    return fread(buf, 1, size, (FILE*)user_data);
}

static int seek_cb(uint64_t position, void *user_data)
{
    /*printf("%d seek_cb(%d)\n", io_num, (int)position);*/
    if (fail_io_num == io_num++)
        return -1;
    return fseek((FILE*)user_data, position, SEEK_SET);
}

typedef struct
{
    mp3dec_t *mp3d;
    mp3dec_file_info_t *info;
    size_t allocated;
} frames_iterate_data;

static int frames_iterate_cb(void *user_data, const uint8_t *frame, int frame_size, int free_format_bytes, size_t buf_size, uint64_t offset, mp3dec_frame_info_t *info)
{
    (void)buf_size;
    (void)offset;
    (void)free_format_bytes;
    frames_iterate_data *d = user_data;
    d->info->channels = info->channels;
    d->info->hz       = info->hz;
    d->info->layer    = info->layer;
    /*printf("%d %d %d\n", frame_size, (int)offset, info->channels);*/
    if ((d->allocated - d->info->samples*sizeof(mp3d_sample_t)) < MINIMP3_MAX_SAMPLES_PER_FRAME*sizeof(mp3d_sample_t))
    {
        if (!d->allocated)
            d->allocated = 1024*1024;
        else
            d->allocated *= 2;
        mp3d_sample_t *alloc_buf = realloc(d->info->buffer, d->allocated);
        if (!alloc_buf)
            return MP3D_E_MEMORY;
        d->info->buffer = alloc_buf;
    }
    int samples = mp3dec_decode_frame(d->mp3d, frame, frame_size, d->info->buffer + d->info->samples, info);
    if (samples)
    {
        d->info->samples += samples*info->channels;
    }
    return 0;
}

static int progress_cb(void *user_data, size_t file_size, uint64_t offset, mp3dec_frame_info_t *info)
{
    (void)user_data;
    (void)file_size;
    (void)offset;
    (void)info;
    return MP3D_E_USER;
}

static void decode_file(const char *input_file_name, const unsigned char *buf_ref, int ref_size, FILE *file_out)
{
    mp3dec_t mp3d;
    int i, res = -1, data_bytes, total_samples = 0, maxdiff = 0;
    int no_std_vec = strstr(input_file_name, "nonstandard") || strstr(input_file_name, "ILL");
    uint8_t *buf = 0;
    double MSE = 0.0, psnr;

    mp3dec_io_t io;
    mp3dec_file_info_t info;
    memset(&info, 0, sizeof(info));
    io.read = read_cb;
    io.seek = seek_cb;
    if (MODE_LOAD == mode)
    {
        res = mp3dec_load(&mp3d, input_file_name, &info, 0, 0);
    } else if (MODE_LOAD_BUF == mode)
    {
        int size = 0;
        FILE *file = fopen(input_file_name, "rb");
        uint8_t *buf = preload(file, &size);
        fclose(file);
        res = buf ? mp3dec_load_buf(&mp3d, buf, size, &info, 0, 0) : MP3D_E_IOERROR;
        free(buf);
    } else if (MODE_LOAD_CB == mode)
    {
        uint8_t *io_buf = malloc(MINIMP3_IO_SIZE);
        FAIL_MEM(io_buf);
        FILE *file = fopen(input_file_name, "rb");
        io.read_data = io.seek_data = file;
        res = file ? mp3dec_load_cb(&mp3d, &io, io_buf, MINIMP3_IO_SIZE, &info, 0, 0) : MP3D_E_IOERROR;
        fclose((FILE*)io.read_data);
        free(io_buf);
    } else if (MODE_ITERATE == mode)
    {
        frames_iterate_data d = { &mp3d, &info, 0 };
        mp3dec_init(&mp3d);
        res = mp3dec_iterate(input_file_name, frames_iterate_cb, &d);
    } else if (MODE_ITERATE_BUF == mode)
    {
        int size = 0;
        FILE *file = fopen(input_file_name, "rb");
        uint8_t *buf = preload(file, &size);
        fclose(file);
        frames_iterate_data d = { &mp3d, &info, 0 };
        mp3dec_init(&mp3d);
        res = buf ? mp3dec_iterate_buf(buf, size, frames_iterate_cb, &d) : MP3D_E_IOERROR;
        free(buf);
    } else if (MODE_ITERATE_CB == mode)
    {
        uint8_t *io_buf = malloc(MINIMP3_IO_SIZE);
        FAIL_MEM(io_buf);
        FILE *file = fopen(input_file_name, "rb");
        io.read_data = io.seek_data = file;
        frames_iterate_data d = { &mp3d, &info, 0 };
        mp3dec_init(&mp3d);
        res = file ? mp3dec_iterate_cb(&io, io_buf, MINIMP3_IO_SIZE, frames_iterate_cb, &d) : MP3D_E_IOERROR;
        fclose((FILE*)io.read_data);
        free(io_buf);
    } else if (MODE_STREAM == mode || MODE_STREAM_BUF == mode || MODE_STREAM_CB == mode)
    {
        mp3dec_ex_t dec;
        size_t readed;
        if (MODE_STREAM == mode)
        {
            res = mp3dec_ex_open(&dec, input_file_name, (seek_to_byte ? MP3D_SEEK_TO_BYTE : MP3D_SEEK_TO_SAMPLE) | MP3D_ALLOW_MONO_STEREO_TRANSITION);
        } else if (MODE_STREAM_BUF == mode)
        {
            int size = 0;
            FILE *file = fopen(input_file_name, "rb");
            buf = preload(file, &size);
            fclose(file);
            res = buf ? mp3dec_ex_open_buf(&dec, buf, size, (seek_to_byte ? MP3D_SEEK_TO_BYTE : MP3D_SEEK_TO_SAMPLE) | MP3D_ALLOW_MONO_STEREO_TRANSITION) : MP3D_E_IOERROR;
        } else if (MODE_STREAM_CB == mode)
        {
            FILE *file = fopen(input_file_name, "rb");
            io.read_data = io.seek_data = file;
            res = file ? mp3dec_ex_open_cb(&dec, &io, (seek_to_byte ? MP3D_SEEK_TO_BYTE : MP3D_SEEK_TO_SAMPLE) | MP3D_ALLOW_MONO_STEREO_TRANSITION) : MP3D_E_IOERROR;
        }
        if (res)
        {
            printf("error: mp3dec_ex_open()=%d failed\n", res);
            exit(1);
        }
        info.samples = dec.samples;
        info.buffer  = malloc(dec.samples*sizeof(mp3d_sample_t));
        FAIL_MEM(info.buffer);
        info.hz      = dec.info.hz;
        info.layer   = dec.info.layer;
        info.channels = dec.info.channels;
        if (position < 0 && -2 != position)
        {
#ifdef _WIN32
            LARGE_INTEGER t;
            QueryPerformanceCounter(&t);
            srand(t.QuadPart);
#else
            srand(time(0));
#endif
            position = info.samples > 500 ? (uint64_t)(info.samples - 500)*rand()/RAND_MAX : 0;
            printf("info: seek to %d/%d\n", position, (int)info.samples);
        }
        if (position)
        {
            if (-2 == position)
                position = 0;
            if (!seek_to_byte)
            {
                info.samples -= MINIMP3_MIN(info.samples, (size_t)position);
                int skip_ref = MINIMP3_MIN((size_t)ref_size, position*sizeof(int16_t));
                buf_ref  += skip_ref;
                ref_size -= skip_ref;
            }
            res = mp3dec_ex_seek(&dec, position);
            if (res)
            {
                printf("error: mp3dec_ex_seek()=%d failed\n", res);
                exit(1);
            }
        }
        if (portion < 0)
        {
            portion = (uint64_t)(info.samples + 150)*rand()/RAND_MAX;
            printf("info: read by %d samples\n", portion);
        }
        if (0 == portion)
            portion = info.samples;
        int samples = info.samples, samples_readed = 0;
        while (samples)
        {
            int to_read = MINIMP3_MIN(samples, portion);
            readed = mp3dec_ex_read(&dec, info.buffer + samples_readed, portion);
            samples -= readed;
            samples_readed += readed;
            if (readed != (size_t)to_read)
            {
                if (seek_to_byte && readed < (size_t)to_read)
                    break;
                printf("error: mp3dec_ex_read() readed less than expected, last_error=%d\n", dec.last_error);
                exit(1);
            }
        }
        readed = mp3dec_ex_read(&dec, info.buffer, 1);
        if (readed)
        {
            printf("error: mp3dec_ex_read() readed more than expected, last_error=%d\n", dec.last_error);
            exit(1);
        }
        if (seek_to_byte)
        {
            info.samples = samples_readed;
        }
        mp3dec_ex_close(&dec);
        if (MODE_STREAM_BUF == mode && buf)
            free(buf);
        if (MODE_STREAM_CB == mode)
            fclose((FILE*)io.read_data);
    } else if (MODE_DETECT == mode || MODE_DETECT_BUF == mode || MODE_DETECT_CB == mode)
    {
        if (MODE_DETECT == mode)
        {
            res = mp3dec_detect(input_file_name);
        } else if (MODE_DETECT_BUF == mode)
        {
            int size = 0;
            FILE *file = fopen(input_file_name, "rb");
            buf = preload(file, &size);
            fclose(file);
            res = buf ? mp3dec_detect_buf(buf, size) : MP3D_E_IOERROR;
        } else if (MODE_DETECT_CB == mode)
        {
            uint8_t *io_buf = malloc(MINIMP3_BUF_SIZE);
            FAIL_MEM(io_buf);
            FILE *file = fopen(input_file_name, "rb");
            io.read_data = io.seek_data = file;
            res = file ? mp3dec_detect_cb(&io, io_buf, MINIMP3_BUF_SIZE) : MP3D_E_IOERROR;
            free(io_buf);
        }
        if (MP3D_E_USER == res)
        {
            printf("info: not an mp3/mpa file\n");
            exit(1);
        } else if (res)
        {
            printf("error: mp3dec_detect*()=%d failed\n", res);
            exit(1);
        }
        printf("info: mp3/mpa file detected\n");
        exit(0);
    } else
    {
        printf("error: unknown mode\n");
        exit(1);
    }
    if (res && MP3D_E_DECODE != res)
    {
        printf("error: read function failed, code=%d\n", res);
        exit(1);
    }
#ifdef MINIMP3_FLOAT_OUTPUT
    int16_t *buffer = malloc(info.samples*sizeof(int16_t));
    FAIL_MEM(buffer);
    mp3dec_f32_to_s16(info.buffer, buffer, info.samples);
    free(info.buffer);
#else
    int16_t *buffer = info.buffer;
#endif
#ifndef MINIMP3_NO_WAV
    if (wave_out && file_out)
        fwrite(wav_header(0, 0, 0, 0), 1, 44, file_out);
#endif
    total_samples += info.samples;
    if (buf_ref)
    {
        size_t ref_samples = ref_size/2;
        int len_match = ref_samples == info.samples;
        int relaxed_len_match = len_match || (ref_samples + 1152) == info.samples || (ref_samples + 2304) == info.samples;
        int seek_len_match = (ref_samples <= info.samples) || (ref_samples + 2304) >= info.samples;
        if ((((!relaxed_len_match && MODE_STREAM != mode && MODE_STREAM_BUF != mode && MODE_STREAM_CB != mode) || !seek_len_match) && (3 == info.layer || 0 == info.layer) && !no_std_vec) || (no_std_vec && !len_match))
        {   /* some standard vectors are for some reason a little shorter */
            printf("error: reference and produced number of samples do not match (%d/%d)\n", (int)ref_samples, (int)info.samples);
            exit(1);
        }
        int max_samples = MINIMP3_MIN(ref_samples, info.samples);
        for (i = 0; i < max_samples; i++)
        {
            int MSEtemp = abs((int)buffer[i] - (int)(int16_t)read16le(&buf_ref[i*sizeof(int16_t)]));
            if (MSEtemp > maxdiff)
                maxdiff = MSEtemp;
            MSE += (float)MSEtemp*(float)MSEtemp;
        }
    }
    if (file_out)
        fwrite(buffer, info.samples, sizeof(int16_t), file_out);
    if (buffer)
        free(buffer);

#ifndef LIBFUZZER
    MSE /= total_samples ? total_samples : 1;
    if (0 == MSE)
        psnr = 99.0;
    else
        psnr = 10.0*log10(((double)0x7fff*0x7fff)/MSE);
    printf("rate=%d samples=%d max_diff=%d PSNR=%f\n", info.hz, total_samples, maxdiff, psnr);
    if (psnr < 96)
    {
        printf("error: PSNR compliance failed\n");
        exit(1);
    }
#endif
#ifndef MINIMP3_NO_WAV
    if (wave_out && file_out)
    {
        data_bytes = ftell(file_out) - 44;
        rewind(file_out);
        fwrite(wav_header(info.hz, info.channels, 16, data_bytes), 1, 44, file_out);
    }
#endif
}

static int self_test(const char *input_file_name)
{
    int ret, size = 0;
    mp3dec_t mp3d;
    mp3dec_ex_t dec;
    mp3dec_frame_info_t frame_info;
    mp3dec_file_info_t finfo;
    mp3dec_io_t io;
    FILE *file = fopen(input_file_name, "rb");
    uint8_t *buf = preload(file, &size);
    fclose(file);
    int samples = mp3dec_decode_frame(&mp3d, buf, size, 0, &frame_info);
    free(buf);
#define ASSERT(c) if (!(c)) { printf("failed, line=%d\n", __LINE__); exit(1); }
    ASSERT(1152 == samples);

    ret = mp3dec_detect_buf(0, size);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_detect_buf(buf, (size_t)-1);
    ASSERT(MP3D_E_PARAM == ret);

    ret = mp3dec_load_buf(0, buf, size, &finfo, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_load_buf(&mp3d, 0, size, &finfo, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_load_buf(&mp3d, buf, (size_t)-1, &finfo, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_load_buf(&mp3d, buf, size, 0, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);

    memset(&finfo, 0xff, sizeof(finfo));
    ret = mp3dec_load_buf(&mp3d, buf, 0, &finfo, 0, 0);
    ASSERT(0 == ret && 0 == finfo.samples);

    ret = mp3dec_detect_cb(&io, 0, size);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_detect_cb(&io, buf, (size_t)-1);
    ASSERT(MP3D_E_PARAM == ret);

    ret = mp3dec_load_cb(0, &io, buf, size, &finfo, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_load_cb(&mp3d, &io, 0, size, &finfo, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_load_cb(&mp3d, &io, buf, (size_t)-1, &finfo, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_load_cb(&mp3d, &io, buf, size, 0, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_load_cb(&mp3d, &io, buf, 0, &finfo, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);

    ret = mp3dec_iterate_buf(0, size, frames_iterate_cb, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_iterate_buf(buf, (size_t)-1, frames_iterate_cb, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_iterate_buf(buf, size, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_iterate_buf(buf, 0, frames_iterate_cb, 0);
    ASSERT(0 == ret);

    ret = mp3dec_iterate_cb(0, buf, size, frames_iterate_cb, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_iterate_cb(&io, 0, size, frames_iterate_cb, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_iterate_cb(&io, buf, (size_t)-1, frames_iterate_cb, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_iterate_cb(&io, buf, size, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_iterate_cb(&io, buf, 0, frames_iterate_cb, 0);
    ASSERT(MP3D_E_PARAM == ret);

    memset(&dec, 0, sizeof(dec));
    ret = mp3dec_ex_seek(&dec, 10); /* seek with zero initialized decoder - no-op without fail */
    ASSERT(0 == ret);
    ret = mp3dec_ex_read(&dec, (mp3d_sample_t*)buf, 10); /* read with zero initialized decoder - reads zero samples */
    ASSERT(0 == ret);
    mp3dec_ex_close(&dec); /* close zero initialized decoder - should not crash */

    ret = mp3dec_ex_open_buf(0, buf, size, MP3D_SEEK_TO_SAMPLE);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_ex_open_buf(&dec, 0, size, MP3D_SEEK_TO_SAMPLE);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_ex_open_buf(&dec, buf, (size_t)-1, MP3D_SEEK_TO_SAMPLE);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_ex_open_buf(&dec, buf, size, MP3D_SEEK_TO_SAMPLE | (MP3D_FLAGS_MASK + 1));
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_ex_open_buf(&dec, buf, 0, MP3D_SEEK_TO_SAMPLE);
    ASSERT(0 == ret);
    ret = mp3dec_ex_read(&dec, (mp3d_sample_t*)buf, 10);
    ASSERT(0 == ret);
    mp3dec_ex_close(&dec);

    ret = mp3dec_ex_open_cb(0, &io, MP3D_SEEK_TO_SAMPLE);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_ex_open_cb(&dec, 0, MP3D_SEEK_TO_SAMPLE);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_ex_open_cb(&dec, &io, MP3D_SEEK_TO_SAMPLE | (MP3D_FLAGS_MASK + 1));
    ASSERT(MP3D_E_PARAM == ret);

    ret = mp3dec_ex_seek(0, 0);
    ASSERT(MP3D_E_PARAM == ret);

    ret = mp3dec_ex_read(0, (mp3d_sample_t*)buf, 10);
    ASSERT(0 == ret); /* invalid param case, no decoder structure to report an error */
    ret = mp3dec_ex_read(&dec, 0, 10);
    ASSERT(0 == ret && MP3D_E_PARAM == dec.last_error); /* invalid param case */
    ret = mp3dec_ex_read_frame(0, (mp3d_sample_t**)buf, &frame_info, 10);
    ASSERT(0 == ret); /* invalid param case, no decoder structure to report an error */
    ret = mp3dec_ex_read_frame(&dec, 0, &frame_info, 10);
    ASSERT(0 == ret && MP3D_E_PARAM == dec.last_error); /* invalid param case */
    ret = mp3dec_ex_read_frame(&dec, (mp3d_sample_t**)buf, 0, 10);
    ASSERT(0 == ret && MP3D_E_PARAM == dec.last_error); /* invalid param case */

    ret = mp3dec_load(0, input_file_name, &finfo, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_load(&mp3d, 0, &finfo, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_load(&mp3d, input_file_name, 0, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_load(&mp3d, "not_foud", &finfo, 0, 0);
    ASSERT(MP3D_E_IOERROR == ret);

    memset(&mp3d, 0xff, sizeof(mp3d));
    memset(&finfo, 0xff, sizeof(finfo));
    ret = mp3dec_load(&mp3d, input_file_name, &finfo, progress_cb, 0);
    ASSERT(MP3D_E_USER == ret && 2304 == finfo.samples && 44100 == finfo.hz && 2 == finfo.channels && 3 == finfo.layer);
    ASSERT(NULL != finfo.buffer);
    free(finfo.buffer);

    ret = mp3dec_iterate(0, frames_iterate_cb, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_iterate(input_file_name, 0, 0);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_iterate("not_foud", frames_iterate_cb, 0);
    ASSERT(MP3D_E_IOERROR == ret);

    ret = mp3dec_ex_open(0, input_file_name, MP3D_SEEK_TO_SAMPLE);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_ex_open(&dec, 0, MP3D_SEEK_TO_SAMPLE);
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_ex_open(&dec, input_file_name, MP3D_SEEK_TO_SAMPLE | (MP3D_FLAGS_MASK + 1));
    ASSERT(MP3D_E_PARAM == ret);
    ret = mp3dec_ex_open(&dec, "not_foud", MP3D_SEEK_TO_SAMPLE);
    ASSERT(MP3D_E_IOERROR == ret);

    file = fopen(input_file_name, "rb");
    io.read = read_cb;
    io.seek = seek_cb;
    io.read_data = io.seek_data = file;

    ret = mp3dec_ex_open_cb(&dec, &io, MP3D_SEEK_TO_SAMPLE);
    ASSERT(0 == ret);
    ASSERT(5 == io_num);
    ASSERT(725760 == dec.samples); /* num samples detected */
    fail_io_num = 5;
    mp3d_sample_t sample;
    size_t readed = mp3dec_ex_read(&dec, &sample, 1);
    ASSERT(0 == readed);
    ASSERT(MP3D_E_IOERROR == dec.last_error);
    readed = mp3dec_ex_read(&dec, &sample, 1);
    ASSERT(0 == readed);
    ASSERT(MP3D_E_IOERROR == dec.last_error); /* stays in error state */
    ret = mp3dec_ex_seek(&dec, 0);
    ASSERT(0 == ret);
    ASSERT(0 == dec.last_error); /* error state reset */
    readed = mp3dec_ex_read(&dec, &sample, 1);
    ASSERT(1 == readed);
    mp3dec_ex_close(&dec);

    ret = mp3dec_ex_open_cb(&dec, &io, MP3D_SEEK_TO_SAMPLE | MP3D_DO_NOT_SCAN);
    ASSERT(0 == ret);
    ASSERT(0 == dec.samples); /* num samples do not detected because of MP3D_DO_NOT_SCAN */
    readed = mp3dec_ex_read(&dec, &sample, 1);
    ASSERT(1 == readed);
    mp3dec_ex_close(&dec);

    fclose((FILE*)io.read_data);
    printf("passed\n");
    return 0;
}

#ifdef LIBFUZZER
int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    decode_file(Data, Size, 0, 0, 0, 0);
    return 0;
}
#else

#if defined(__arm__) || defined(__aarch64__) || defined(__PPC__)
int main2(int argc, char *argv[]);
int main2(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    int i, ref_size, do_self_test = 0;
    for(i = 1; i < argc; i++)
    {
        if (argv[i][0] != '-')
            break;
        switch (argv[i][1])
        {
        case 'm': i++; if (i < argc) mode = atoi(argv[i]); break;
        case 's': i++; if (i < argc) position = atoi(argv[i]); break;
        case 'p': i++; if (i < argc) portion  = atoi(argv[i]); break;
        case 'e': i++; if (i < argc) fail_io_num = atoi(argv[i]); break;
#ifdef MINIMP3_TEST
        case 'f': i++; if (i < argc) fail_malloc_num = atoi(argv[i]); break;
#endif
        case 'b': seek_to_byte = 1; break;
        case 't': do_self_test = 1; break;
        default:
            printf("error: unrecognized option\n");
            return 1;
        }
    }
    char *ref_file_name    = (argc > (i + 1)) ? argv[i + 1] : NULL;
    char *output_file_name = (argc > (i + 2)) ? argv[i + 2] : NULL;
    FILE *file_out = NULL;
    if (output_file_name)
    {
        file_out = fopen(output_file_name, "wb");
#ifndef MINIMP3_NO_WAV
        char *ext = strrchr(output_file_name, '.');
        if (ext && !strcasecmp(ext + 1, "wav"))
            wave_out = 1;
#endif
    }
    FILE *file_ref = ref_file_name ? fopen(ref_file_name, "rb") : NULL;
    unsigned char *buf_ref = preload(file_ref, &ref_size);
    if (file_ref)
        fclose(file_ref);
#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
    while (__AFL_LOOP(1000)) {
#endif
    char *input_file_name  = (argc > i) ? argv[i] : NULL;
    if (!input_file_name)
    {
        printf("error: no file names given\n");
        return 1;
    }
    if (do_self_test)
        return self_test(input_file_name);
    decode_file(input_file_name, buf_ref, ref_size, file_out);
#ifdef __AFL_HAVE_MANUAL_CONTROL
    }
#endif
    if (buf_ref)
        free(buf_ref);
    if (file_out)
        fclose(file_out);
    return 0;
}

#if defined(__arm__) || defined(__aarch64__) || defined(__PPC__)
static const char *g_files[] = {
    "vectors/ILL2_center2.bit",
    "vectors/ILL2_dual.bit",
    "vectors/ILL2_dynx22.bit",
    "vectors/ILL2_dynx31.bit",
    "vectors/ILL2_dynx32.bit",
    "vectors/ILL2_ext_switching.bit",
    "vectors/ILL2_layer1.bit",
    "vectors/ILL2_layer3.bit",
    "vectors/ILL2_mono.bit",
    "vectors/ILL2_multilingual.bit",
    "vectors/ILL2_overalloc1.bit",
    "vectors/ILL2_overalloc2.bit",
    "vectors/ILL2_prediction.bit",
    "vectors/ILL2_samples.bit",
    "vectors/ILL2_scf63.bit",
    "vectors/ILL2_tca21.bit",
    "vectors/ILL2_tca30.bit",
    "vectors/ILL2_tca30_PC.bit",
    "vectors/ILL2_tca31_mtx0.bit",
    "vectors/ILL2_tca31_mtx2.bit",
    "vectors/ILL2_tca31_PC.bit",
    "vectors/ILL2_tca32_PC.bit",
    "vectors/ILL2_wrongcrc.bit",
    "vectors/ILL4_ext_id1.bit",
    "vectors/ILL4_sync.bit",
    "vectors/ILL4_wrongcrc.bit",
    "vectors/ILL4_wrong_length1.bit",
    "vectors/ILL4_wrong_length2.bit",
    "vectors/l1-fl1.bit",
    "vectors/l1-fl2.bit",
    "vectors/l1-fl3.bit",
    "vectors/l1-fl4.bit",
    "vectors/l1-fl5.bit",
    "vectors/l1-fl6.bit",
    "vectors/l1-fl7.bit",
    "vectors/l1-fl8.bit",
    "vectors/l2-fl10.bit",
    "vectors/l2-fl11.bit",
    "vectors/l2-fl12.bit",
    "vectors/l2-fl13.bit",
    "vectors/l2-fl14.bit",
    "vectors/l2-fl15.bit",
    "vectors/l2-fl16.bit",
    "vectors/l2-nonstandard-fl1_fl2_ff.bit",
    "vectors/l2-nonstandard-free_format.bit",
    "vectors/l2-nonstandard-test32-size.bit",
    "vectors/l2-test32.bit",
    "vectors/l3-compl.bit",
    "vectors/l3-he_32khz.bit",
    "vectors/l3-he_44khz.bit",
    "vectors/l3-he_48khz.bit",
    "vectors/l3-hecommon.bit",
    "vectors/l3-he_free.bit",
    "vectors/l3-he_mode.bit",
    "vectors/l3-nonstandard-big-iscf.bit",
    "vectors/l3-nonstandard-compl-sideinfo-bigvalues.bit",
    "vectors/l3-nonstandard-compl-sideinfo-blocktype.bit",
    "vectors/l3-nonstandard-compl-sideinfo-size.bit",
    "vectors/l3-nonstandard-sideinfo-size.bit",
    "vectors/l3-si.bit",
    "vectors/l3-si_block.bit",
    "vectors/l3-si_huff.bit",
    "vectors/l3-sin1k0db.bit",
    "vectors/l3-test45.bit",
    "vectors/l3-test46.bit",
    "vectors/M2L3_bitrate_16_all.bit",
    "vectors/M2L3_bitrate_22_all.bit",
    "vectors/M2L3_bitrate_24_all.bit",
    "vectors/M2L3_compl24.bit",
    "vectors/M2L3_noise.bit"
};
int main()
{
    size_t i;
    char buf[256];
    char *v[3];
    v[2] = buf;
    for (i = 0; i < sizeof(g_files)/sizeof(g_files[0]); i++)
    {
        int ret;
        const char *file = g_files[i];
        size_t len = strlen(file);
        strcpy(buf, file);
        buf[len - 3] = 'p';
        buf[len - 2] = 'c';
        buf[len - 1] = 'm';
        v[1] = (char*)file;
        printf("%s\n", file);
        ret = main2(3, v);
        if (ret)
            return ret;
    }
    return 0;
}
#endif
#endif
