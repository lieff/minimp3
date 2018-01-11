minimp3
==========

[![Build Status](https://travis-ci.org/lieff/minimp3.svg)](https://travis-ci.org/lieff/minimp3)

Minimalistic MP3 decoder. It's designed to be small, fast (with sse/neon suport) and accurate (ISO conformant).
Here is rough benchmark measured with perf (i7-6700K, IO included, no CPU heat to address speedstep):


| Vector      | Hz    | Samples| Sec    | Clocktics | Clocktics per second | PSNR |
| ----------- | ----- | ------ | ------ | --------- | ------ | ------ |
|compl.bit    | 48000 | 248832 | 5.184  | 25242198  | 4.869M | 124.22 |
|he_32khz.bit | 32000 | 172800 | 5.4    | 16148873  | 2.990M | 139.67 |
|he_44khz.bit | 44100 | 472320 | 10.710 | 41977782  | 3.919M | 144.04 |
|he_48khz.bit | 48000 | 172800 | 3.6    | 16127644  | 4.479M | 139.67 |
|hecommon.bit | 44100 | 69120  | 1.567  | 6133060   | 3.913M | 133.93 |
|he_free.bit  | 44100 | 156672 | 3.552  | 12423560  | 3.496M | 137.48 |
|he_mode.bit  | 44100 | 262656 | 5.955  | 18489271  | 3.104M | 118.00 |
|si.bit       | 44100 | 135936 | 3.082  | 13070375  | 4.240M | 120.30 |
|si_block.bit | 44100 | 73728  | 1.671  | 7148739   | 4.275M | 125.18 |
|si_huff.bit  | 44100 | 86400  | 1.959  | 8595200   | 4.387M | 107.98 |
|sin1k0db.bit | 44100 | 725760 | 16.457 | 55247025  | 3.357M | 111.03 |
