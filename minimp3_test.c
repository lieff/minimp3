#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include <stdio.h>
#include <math.h>

/*static char *wav_header(int hz, int ch, int bips, int data_bytes)
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
}*/

static void decode_file(FILE *file_mp3, FILE *file_ref, FILE *file_out)
{
    static mp3dec_t mp3d;
    mp3dec_frame_info_t info;
    int i, /*data_bytes, */samples, total_samples = 0, nbuf = 0, maxdiff = 0;
    double MSE = 0.0, psnr;
    unsigned char buf[4096];

    mp3dec_init(&mp3d);

    //fwrite(wav_header(0, 0, 0, 0), 1, 44, file_wav);

    do
    {
        short pcm[2*1152], pcm2[2*1152];
        nbuf += fread(buf + nbuf, 1, sizeof(buf) - nbuf, file_mp3);
        samples = mp3dec_decode_frame(&mp3d, buf, nbuf, pcm, &info);
        if (samples)
        {
            int readed = fread(pcm2, 1, 2*info.channels*samples, file_ref);
            if (readed < 0)
                exit(1);
            total_samples += samples*info.channels;
            for (i = 0; i < samples*info.channels; i++)
            {
                int MSEtemp = abs((int)pcm[i] - (int)pcm2[i]);
                if (MSEtemp > maxdiff)
                    maxdiff = MSEtemp;
                MSE += MSEtemp*MSEtemp;
            }
            if (file_out)
                fwrite(pcm, samples, 2*info.channels, file_out);
        }
        memmove(buf, buf + info.frame_bytes, nbuf -= info.frame_bytes);
    } while (info.frame_bytes);

    MSE /= total_samples;
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

    //data_bytes = ftell(file_wav) - 44;
    //rewind(file_wav);
    //fwrite(wav_header(info.hz, info.channels, 16, data_bytes), 1, 44, file_wav);
    fclose(file_mp3);
    fclose(file_ref);
    if (file_out)
        fclose(file_out);
}

int main(int argc, char *argv[])
{
    char *input_file_name  = (argc > 1) ? argv[1] : NULL;
    char *ref_file_name    = (argc > 2) ? argv[2] : NULL;
    char *output_file_name = (argc > 3) ? argv[3] : NULL;
    if (!input_file_name || !ref_file_name)
    {
        printf("error: no file names given\n");
        return 1;
    }
    decode_file(fopen(input_file_name, "rb"), fopen(ref_file_name, "rb"), output_file_name ? fopen(output_file_name, "wb") : NULL);
    return 0;
}
