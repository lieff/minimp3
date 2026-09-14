# Helix MP3 — benchmark reference only

This is RealNetworks' Helix fixed-point MP3 decoder, kept here as the
performance reference the fixed-point work in this repository was measured
against. It is **not** part of the library and nothing outside `helix/`
includes it.

## Licence — different from the rest of this repository

Everything outside this directory is CC0, inherited from upstream minimp3.
The code in this directory is **not**. It is licensed under the RealNetworks
Public Source License (`RPSL.txt`) and the RealNetworks Community Source
Licence, which are not permissive licences and are not compatible with CC0.

It is here because a benchmark needs its reference to be reproducible, and a
number quoted against a decoder nobody can run is not a measurement. Anyone
redistributing this repository, or vendoring any part of it, must read
`RPSL.txt` and satisfy themselves independently. Do not copy this directory
into a product.

Tracking: FastLED/license#14.
