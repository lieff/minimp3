minimp3
==========

[![Build Status](https://travis-ci.org/lieff/minimp3.svg)](https://travis-ci.org/lieff/minimp3)

Minimalistic MP3 decoder. It's designed to be small, fast (with sse/neon suport) and accurate (ISO conformant).
Here is rough benchmark measured with perf (i7-6700K, IO included, no CPU heat to address speedstep):


| Vector      | Hz    | Samples| Sec    | Clocktics | Clocktics per second | PSNR | Max diff |
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
| source: 84kb | 70kb |
| no vector opts | sse/neon intrinsics |


Keyj minimp3 benchmark/conformance test:


| Vector      | Hz    | Samples| Sec    | Clocktics | Clocktics per second | PSNR | Max diff |
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

