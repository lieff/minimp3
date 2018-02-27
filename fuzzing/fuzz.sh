#!/usr/bin/env bash
cd "${0%/*}"

afl/afl-fuzz -m none -i ../vectors/fuzz -o findings ./fuzz
