minimp3
==========

[![Build Status](https://travis-ci.org/lieff/minimp3.svg)](https://travis-ci.org/lieff/minimp3)

Minimalistic MP3 decoder single header library. It's designed to be small, fast (with sse/neon support) and accurate (ISO conformant).
Here is rough benchmark measured with perf (i7-6700K, IO included, no CPU heat to address speedstep):


| Vector      | Hz    | Samples| Sec    | Clockticks | Clockticks per second | PSNR | Max diff |
| ----------- | ----- | ------ | ------ | --------- | ------ | ------ | - |
|compl.bit    | 48000 | 248832 | 5.184  | 25242198  | 4.869M | 124.22 | 1 |
|he_32khz.bit | 32000 | 172800 | 5.4    | 16148873  | 2.990M | 139.67 | 1 |
|he_44khz.bit | 44100 | 472320 | 10.710 | 41977782  | 3.919M | 144.04 | 1 |
|he_48khz.bit | 48000 | 172800 | 3.6    | 16127644  | 4.479M | 139.67 | 1 |
|hecommon.bit | 44100 | 69120  | 1.567  | 6133060   | 3.913M | 133.93 | 1 |
|he_free.bit  | 44100 | 156672 | 3.552  | 12423560  | 3.496M | 137.48 | 1 |
|he_mode.bit  | 44100 | 262656 | 5.955  | 18489271  | 3.104M | 118.00 | 1 |
|si.bit       | 44100 | 135936 | 3.082  | 13070375  | 4.240M | 120.30 | 1 |
|si_block.bit | 44100 | 73728  | 1.671  | 7148739   | 4.275M | 125.18 | 1 |
|si_huff.bit  | 44100 | 86400  | 1.959  | 8595200   | 4.387M | 107.98 | 1 |
|sin1k0db.bit | 44100 | 725760 | 16.457 | 55247025  | 3.357M | 111.03 | 1 |


## Compare with keyj's [minimp3](http://keyj.emphy.de/minimp3/)

Feature compare:

| Keyj minimp3 | Current |
| ------------ | ------- |
| Fixed point  | Float point |
| source: 84kb | 68kb |
| no vector opts | sse/neon intrinsics |


Keyj minimp3 benchmark/conformance test:


| Vector      | Hz    | Samples| Sec    | Clockticks | Clockticks per second | PSNR | Max diff |
| ----------- | ----- | ------ | ------ | --------- | ------  | ----- | - |
|compl.bit    | 48000 | 248832 | 5.184  | 31849373  | 6.143M  | 71.50 | 41 |
|he_32khz.bit | 32000 | 172800 | 5.4    | 26302319  | 4.870M  | 71.63 | 24 |
|he_44khz.bit | 44100 | 472320 | 10.710 | 41628861  | 3.886M  | 71.63 | 24 |
|he_48khz.bit | 48000 | 172800 | 3.6    | 25899527  | 7.194M  | 71.63 | 24 |
|hecommon.bit | 44100 | 69120  | 1.567  | 20437779  | 13.039M | 71.58 | 25 |
|he_free.bit  | 44100 | 0 | 0  | -  | - | -  | - |
|he_mode.bit  | 44100 | 262656 | 5.955  | 30988984  | 5.203M  | 71.78 | 27 |
|si.bit       | 44100 | 135936 | 3.082  | 24096223  | 7.817M  | 72.35 | 36 |
|si_block.bit | 44100 | 73728  | 1.671  | 20722017  | 12.394M | 71.84 | 26 |
|si_huff.bit  | 44100 | 86400  | 1.959  | 21121376  | 10.780M | 27.80 | 65535 |
|sin1k0db.bit | 44100 | 730368 | 16.561 | 55569636  | 3.355M  | 0.15  | 58814 |

Keyj minimp3 conformance test fails on all vectors (PSNR < 96db), free format is not supported.

## Usage

Firstly we need initialize decoder structure:
```
    static mp3dec_t mp3d;
    mp3dec_init(&mp3d);
```
Then we decode input stream frame-by-frame:
```
    /*typedef struct
    {
        int frame_bytes;
        int channels;
        int hz;
        int layer;
        int bitrate_kbps;
    } mp3dec_frame_info_t;*/
    mp3dec_frame_info_t info;
    short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    /*unsigned char *input_buf; - input byte stream*/
    samples = mp3dec_decode_frame(&mp3d, input_buf, buf_size, pcm, &info);
```
The mp3dec_decode_frame() decode 1 full MP3 frame from input buffer.  

The input buffer must be sufficient to hold 1 MP3 frame.  

Decoder analyse input buffer to properly sync with the MP3 stream, and skip ID3 and invalid data.  
Short buffer may cause false sync and produce squealing artefacts. The more input data size, the more reliable sync procedure.  
It is recommended to have up to 10 consecutive MP3 frames (~ 16 KB) in the input buffer.  

Size of consumed MP3 data returned in the mp3dec_frame_info_t::frame_bytes field.  
Application must remove mp3dec_frame_info_t::frame_bytes bytes from input buffer before next decoder invocation.  

Decode function returns # of decoded samples; following cases are possible:  

- 0    - No MP3 data was found in the input buffer  
  384  - Layer 1  
  576  - MPEG 2 Layer 3  
  1152 - otherwise  

Description of returned #samples and mp3dec_frame_info_t::frame_bytes combination:  

-  
  #samples >  0 && frame_bytes >  0  -  succesful decode  
  #samples == 0 && frame_bytes >  0  -  decoder skip ID3 or invalid data  
  #samples == 0 && frame_bytes == 0  -  insufficied data, application must supply more data  
  #samples >  0 && frame_bytes == 0  -  impossible combination  

Also if frame_bytes == 0 other info fields can be uninitialized/not updated, if info.frame_bytes != 0 all info fields available such as info.hz = sample rate, info.channels = mono(1)/stereo(2), info.bitrate_kbps = bitrate in kbits.  

Application may call mp3dec_init() when changing decode position, however, it is not necessary.  

As a special case, decoder support already splitted MP3 streams (for ex, after MP4 demux). In this case input buffer must contains exactly 1 non-free-format frame.  

## Seeking

You can just seek to any byte in the middle of stream and call mp3dec_decode_frame, it works almost always, but not 100% guaranteed. If granule data accidentally detected as valid mp3 header, short audio artifacts is possible.
However, if file known to be cbr, frames have equal size and do not have id3 tags we can decode first frame and calculate all frame positions: info.frame_bytes*N.
Note that because of padding, frames can differ in size even in cbr case.
