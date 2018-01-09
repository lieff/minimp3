#pragma once
#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

typedef struct
{
    int frame_bytes;
    int channels;
    int hz;
    int layer;
    int bitrate_kbps;
} mp3dec_frame_info_t;

typedef struct
{
    float mdct_overlap[2][9*32];
    float qmf_state[15*2*32];
    int reserv;
    int free_format_bytes;
    unsigned char header[4];
    unsigned char reserv_buf[511];
} mp3dec_t;

void mp3dec_init(mp3dec_t *dec);
int mp3dec_decode_frame(mp3dec_t *dec, const unsigned char *mp3, int mp3_bytes, short *pcm, mp3dec_frame_info_t *info);

#ifdef __cplusplus
}
#endif //__cplusplus
