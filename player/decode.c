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
#define MINIMP3_IMPLEMENTATION
#include "decode.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

static void get_spectrum(decoder *dec, int numch)
{
    int i, ch, band;
    const mp3dec_t *d = &dec->mp3d.mp3d;
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
    memset(buf, 0, bytes);
    mp3dec_ex_read(&dec->mp3d, (mp3d_sample_t*)buf, bytes/sizeof(mp3d_sample_t));
}

int open_dec(decoder *dec, const char *file_name)
{
    if (!dec || !file_name || !*file_name)
        return 0;

    memset(dec, 0, sizeof(*dec));

    mp3dec_ex_open(&dec->mp3d, file_name, MP3D_SEEK_TO_SAMPLE);
    if (!dec->mp3d.samples)
        return 0;
    return 1;
}

int close_dec(decoder *dec)
{
    mp3dec_ex_close(&dec->mp3d);
    memset(dec, 0, sizeof(*dec));
}
