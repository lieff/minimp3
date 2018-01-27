//#define MINIMP3_ONLY_MP3
//#define MINIMP3_ONLY_SIMD
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#ifndef MINIMP3_NO_WAV
static char *wav_header(int hz, int ch, int bips, int data_bytes)
{
    static char hdr[44] = "RIFFsizeWAVEfmt \x10\0\0\0\1\0ch_hz_abpsbabsdatasize";
    unsigned long nAvgBytesPerSec = bips*ch*hz >> 3;
    unsigned int nBlockAlign      = bips*ch >> 3;

    *(int *  )(hdr + 0x04) = 44 + data_bytes - 8;   // File size - 8
    *(short *)(hdr + 0x14) = 1;                     // Integer PCM format
    *(short *)(hdr + 0x16) = ch;
    *(int *  )(hdr + 0x18) = hz;
    *(int *  )(hdr + 0x1C) = nAvgBytesPerSec;
    *(short *)(hdr + 0x20) = nBlockAlign;
    *(short *)(hdr + 0x22) = bips;
    *(int *  )(hdr + 0x28) = data_bytes;
    return hdr;
}
#endif

static unsigned char *preload(FILE *file, int *data_size)
{
    unsigned char *data;
    *data_size = 0;
    if (!file)
        return 0;
    fseek(file, 0, SEEK_END);
    *data_size = (int)ftell(file);
    fseek(file, 0, SEEK_SET);
    data = (unsigned char*)malloc(*data_size);
    if (!data)
        return 0;
    if ((int)fread(data, 1, *data_size, file) != *data_size)
        exit(1);
    return data;
}

static void decode_file(FILE *file_mp3, FILE *file_ref, FILE *file_out, const int wave_out)
{
    static mp3dec_t mp3d;
    mp3dec_frame_info_t info;
    int i, data_bytes, samples, total_samples = 0, maxdiff = 0, mp3_size, ref_size;
    double MSE = 0.0, psnr;
    unsigned char *buf_mp3 = preload(file_mp3, &mp3_size), *buf_ref = preload(file_ref, &ref_size);

    mp3dec_init(&mp3d);
#ifndef MINIMP3_NO_WAV
    if (wave_out)
        fwrite(wav_header(0, 0, 0, 0), 1, 44, file_out);
#endif
    do
    {
        short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        samples = mp3dec_decode_frame(&mp3d, buf_mp3, mp3_size, pcm, &info);
        if (samples)
        {
            if (buf_ref && ref_size >= samples*info.channels*2)
            {
                total_samples += samples*info.channels;
                for (i = 0; i < samples*info.channels; i++)
                {
                    int MSEtemp = abs((int)pcm[i] - (int)(((short*)buf_ref)[i]));
                    if (MSEtemp > maxdiff)
                        maxdiff = MSEtemp;
                    MSE += MSEtemp*MSEtemp;
                }
                buf_ref  += samples*info.channels*2;
                ref_size -= samples*info.channels*2;
            }
            if (file_out)
                fwrite(pcm, samples, 2*info.channels, file_out);
        }
        buf_mp3  += info.frame_bytes;
        mp3_size -= info.frame_bytes;
    } while (info.frame_bytes);

    MSE /= total_samples ? total_samples : 1;
    if (0 == MSE)
        psnr = 99.0;
    else
        psnr = 10.0*log10(((double)0x7fff*0x7fff)/MSE);
    printf("rate=%d samples=%d max_diff=%d PSNR=%f\n", info.hz, total_samples, maxdiff, psnr);
    if (psnr < 96)
    {
        printf("PSNR compliance failed\n");
        exit(1);
    }
#ifndef MINIMP3_NO_WAV
    if (wave_out)
    {
        data_bytes = ftell(file_out) - 44;
        rewind(file_out);
        fwrite(wav_header(info.hz, info.channels, 16, data_bytes), 1, 44, file_out);
    }
#endif
    fclose(file_mp3);
    if (file_ref)
        fclose(file_ref);
    if (file_out)
        fclose(file_out);
}

int main(int argc, char *argv[])
{
    char *input_file_name  = (argc > 1) ? argv[1] : NULL;
    char *ref_file_name    = (argc > 2) ? argv[2] : NULL;
    char *output_file_name = (argc > 3) ? argv[3] : NULL;
    int wave_out = 0;
#ifndef MINIMP3_NO_WAV
    if (output_file_name)
    {
        char *ext = strrchr(output_file_name, '.');
        if (ext && !strcasecmp(ext+1, "wav"))
            wave_out = 1;
    }
#endif
    if (!input_file_name)
    {
        printf("error: no file names given\n");
        return 1;
    }
    decode_file(fopen(input_file_name, "rb"), fopen(ref_file_name, "rb"), output_file_name ? fopen(output_file_name, "wb") : NULL, wave_out);
    return 0;
}
