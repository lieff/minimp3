#!/usr/bin/env bash
cd "${0%/*}"

afl/afl-fuzz -m 50 -i- -o findings/ ./fuzz
