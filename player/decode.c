#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <math.h>
#include "decode.h"
#define MINIMP3_IMPLEMENTATION
#include "../minimp3.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void get_spectrum(decoder *dec, int numch)
{
    int i, ch, band;
    const mp3dec_t *d = &dec->mp3d;
    // Average spectrum power for 32 frequency band
    for (ch = 0; ch < numch; ch++)
    {
        for (band = 0; band < 32; band++)
        {
            float band_power = 0;
            for (i = 0; i < 9; i++)
            {
                float sample = d->mdct_overlap[ch][i + band*9];
                band_power += sample*sample;
            }
            // with averaging
            //dec->spectrum[band][ch] += (band_power/9 - spectrum[band][ch]) * 0.25;

            // w/o averaging
            dec->spectrum[band][ch] = band_power/9;
        }
    }
    // Calculate dB scale from power for better visualization
    for (ch = 0; ch < numch; ch++)
    {
        for (band = 0; band < 32; band++)
        {
            float power = dec->spectrum[band][ch];
            float db_offset = 100;      // to shift dB values from [0..-inf] to [max..0]
            float db = 10*log10f(power + 1e-15) + db_offset;
            if (db < 0) db = 0;
            //printf("% .5f\t", db);
        }
        //printf("\n");
    }
}

void decode_samples(decoder *dec, uint8_t *buf, int bytes)
{
    if (dec->pcm_bytes - dec->pcm_copied)
    {
        int to_copy = MIN(dec->pcm_bytes - dec->pcm_copied, bytes);
        memcpy(buf, (uint8_t*)dec->pcm + dec->pcm_copied, to_copy);
        buf   += to_copy;
        bytes -= to_copy;
        dec->pcm_copied += to_copy;
    }
    if (!bytes)
        return;
    do
    {
        int samples = mp3dec_decode_frame(&dec->mp3d, dec->mp3_buf + dec->pos, dec->mp3_size - dec->pos, dec->pcm, &dec->info);
        dec->pos += dec->info.frame_bytes;
        if (samples)
        {
            get_spectrum(dec, dec->info.channels);
            dec->pcm_bytes = samples*2*dec->info.channels;
            dec->pcm_copied = MIN(dec->pcm_bytes, bytes);
            memcpy(buf, dec->pcm, dec->pcm_copied);
            buf   += dec->pcm_copied;
            bytes -= dec->pcm_copied;
            if (!dec->mp3_rate)
                dec->mp3_rate = dec->info.hz;
            if (!dec->mp3_channels)
                dec->mp3_channels = dec->info.channels;
            if (dec->mp3_rate != dec->info.hz || dec->mp3_channels != dec->info.channels)
                break;
        }
    } while (dec->info.frame_bytes && bytes);
    if (bytes)
        memset(buf, 0, bytes);
}

int open_dec(decoder *dec, const char *file_name)
{
    if (!dec || !file_name || !*file_name)
        return 0;

    memset(dec, 0, sizeof(*dec));

    struct stat st;
retry_open:
    dec->file = open(file_name, O_RDONLY);
    if (dec->file < 0 && (errno == EAGAIN || errno == EINTR))
        goto retry_open;
    if (dec->file < 0 || fstat(dec->file, &st) < 0)
    {
        close_dec(dec);
        return 0;
    }

    dec->mp3_size = st.st_size;
retry_mmap:
    dec->mp3_buf = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, dec->file, 0);
    if (MAP_FAILED == dec->mp3_buf && (errno == EAGAIN || errno == EINTR))
        goto retry_mmap;
    if (MAP_FAILED == dec->mp3_buf)
    {
        close_dec(dec);
        return 0;
    }
    mp3dec_init(&dec->mp3d);
    return 1;
}

int close_dec(decoder *dec)
{
    if (dec->mp3_buf)
        free(dec->mp3_buf);
    if (dec->mp3_buf && MAP_FAILED != dec->mp3_buf)
        munmap(dec->mp3_buf, dec->mp3_size);
    if (dec->file)
        close(dec->file);
    memset(dec, 0, sizeof(*dec));
}
