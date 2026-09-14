/* Decodes one MP3 bitstream and reports frames, samples and an FNV-1a checksum
   of the PCM. The checksum is the point: it is how a change is shown to be
   bit-exact, on the host and under QEMU on a cross target, without needing
   hardware. FastLED's on-device test reports the same value.

   Build with tools/build.py; it is not standalone (see -I flags there). */

#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr,
                "usage: decode <file.bit> [--pcm out.pcm] [--ref ref.pcm]\n");
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f)
    {
        fprintf(stderr, "cannot open %s\n", argv[1]);
        return 2;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = (unsigned char *)malloc((size_t)size);
    if (!buf || fread(buf, 1, (size_t)size, f) != (size_t)size)
    {
        fprintf(stderr, "cannot read %s\n", argv[1]);
        return 2;
    }
    fclose(f);

    /* Strip metadata containers before decoding. minimp3 is a bare codec: it
       will not sync past an ID3v2 header, and a trailing ID3v1 or APE tag
       looks like garbage audio. Four conformance vectors decode to nothing at
       all without this, which is a property of the container and not of the
       codec -- upstream puts the same logic in minimp3_ex.h and FastLED does
       it in fl/codec/mp3.cpp.hpp. */
    long start = 0;
    if (size >= 10 && !memcmp(buf, "ID3", 3))
    {
        /* Syncsafe: seven bits per byte, the high bit always clear. */
        long tag = ((long)(buf[6] & 0x7f) << 21) | ((long)(buf[7] & 0x7f) << 14) |
                   ((long)(buf[8] & 0x7f) << 7) | (long)(buf[9] & 0x7f);
        start = 10 + tag + ((buf[5] & 0x10) ? 10 : 0);  /* optional footer */
        if (start > size)
            start = 0;
    }
    if (size - start >= 128 && !memcmp(buf + size - 128, "TAG", 3))
        size -= 128;
    if (size - start >= 32 && !memcmp(buf + size - 32, "APETAGEX", 8))
    {
        long ape = (long)buf[size - 20] | ((long)buf[size - 19] << 8) |
                   ((long)buf[size - 18] << 16) | ((long)buf[size - 17] << 24);
        if (ape > 0 && ape + 32 <= size - start)
            size -= ape + 32;
    }

    FILE *out = 0;
    const char *refpath = 0;
    for (int i = 2; i < argc - 1; i++)
    {
        if (!strcmp(argv[i], "--pcm"))
            out = fopen(argv[i + 1], "wb");
        if (!strcmp(argv[i], "--ref"))
            refpath = argv[i + 1];
    }

    /* Held so the reference comparison can run over the whole decode rather
       than frame by frame; the ISO measure is defined over the full signal. */
    long capacity = 1 << 20, produced = 0;
    short *all = (short *)malloc((size_t)capacity * sizeof(short));

    static fl::third_party::mp3dec_t dec;
    /* The scratch is a caller-owned union rather than an internal buffer: this
       fork's decoder keeps ~1.5 KB out of its own frame that way, which was
       most of FastLED#4116's decode-stack reduction. It is static here only to
       keep it off this program's stack too. */
    static fl::third_party::mp3dec_scratch_t scratch;
    fl::third_party::mp3dec_init(&dec);

    /* FNV-1a over the little-endian sample bytes, so the value does not depend
       on the host's word order -- it has to match across x86-64 and riscv32. */
    unsigned int hash = 2166136261u;
    long long total_samples = 0;
    int frames = 0, hz = 0, channels = 0;
    long offset = start;

    for (;;)
    {
        fl::third_party::mp3d_sample_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        fl::third_party::mp3dec_frame_info_t info;
        int samples = fl::third_party::mp3dec_decode_frame_r(
            &dec, &scratch, buf + offset, (int)(size - offset), pcm, &info);
        if (info.frame_bytes <= 0)
            break;
        offset += info.frame_bytes;
        if (samples)
        {
            frames++;
            hz = info.hz;
            channels = info.channels;
            int values = samples * info.channels;
            total_samples += values;
            for (int i = 0; i < values; i++)
            {
                unsigned short v = (unsigned short)pcm[i];
                hash = (hash ^ (v & 0xff)) * 16777619u;
                hash = (hash ^ ((v >> 8) & 0xff)) * 16777619u;
            }
            if (out)
                fwrite(pcm, sizeof(pcm[0]), (size_t)values, out);
            while (produced + values > capacity)
            {
                capacity *= 2;
                all = (short *)realloc(all, (size_t)capacity * sizeof(short));
            }
            for (int i = 0; i < values; i++)
                all[produced++] = (short)pcm[i];
        }
        if (offset >= size)
            break;
    }
    if (out)
        fclose(out);
    free(buf);

    double psnr = -1.0;
    long reference_samples = -1;
    if (refpath)
    {
        FILE *rf = fopen(refpath, "rb");
        if (rf)
        {
            fseek(rf, 0, SEEK_END);
            long rbytes = ftell(rf);
            fseek(rf, 0, SEEK_SET);
            reference_samples = rbytes / 2;
            short *ref = (short *)malloc((size_t)rbytes);
            if (fread(ref, 1, (size_t)rbytes, rf) == (size_t)rbytes)
            {
                /* Over the shared prefix. A prefix-only PSNR cannot see
                   truncation or overrun, which is why the caller checks
                   produced against reference separately and why those bounds
                   are exact rather than a name allowlist. */
                long n = produced < reference_samples ? produced
                                                      : reference_samples;
                double sum = 0.0;
                for (long i = 0; i < n; i++)
                {
                    double d = (double)all[i] - (double)ref[i];
                    sum += d * d;
                }
                if (n > 0)
                {
                    double mse = sum / (double)n;
                    /* 32768 is full scale for int16; the ISO limited-accuracy
                       floor of 60 dB is defined against it. */
                    psnr = mse > 0.0 ? 10.0 * log10(32768.0 * 32768.0 / mse)
                                     : 999.0;
                }
            }
            free(ref);
            fclose(rf);
        }
    }
    free(all);

    printf("frames=%d samples=%lld hz=%d channels=%d fnv1a=0x%08x "
           "produced=%ld reference=%ld psnr=%.2f\n",
           frames, total_samples, hz, channels, hash, produced,
           reference_samples, psnr);
    return frames ? 0 : 1;
}
