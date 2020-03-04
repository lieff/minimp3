_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

set -e

CFLAGS="-O2 -std=c89 -Wall -Wextra -Wmissing-prototypes -Werror -fno-asynchronous-unwind-tables -fno-stack-protector -U_FORTIFY_SOURCE \
-ffunction-sections -fdata-sections -Wl,--gc-sections"

echo testing mp4 mode...
gcc $CFLAGS -o minimp3 minimp3_test.c -lm
scripts/test_mode.sh 3 0 0

echo testing stream mode...
scripts/test_mode.sh 6 -1 -1

echo testing coverage x86 w sse...
gcc -coverage -O0 -m32 -std=c89 -msse2 -DMINIMP3_TEST -DMINIMP3_NO_WAV -o minimp3 minimp3_test.c -lm
scripts/test.sh
scripts/test_mode.sh 1 0 0
scripts/test_mode.sh 2 0 0
scripts/test_mode.sh 3 0 0
scripts/test_mode.sh 4 0 0
scripts/test_mode.sh 5 0 0
scripts/test_mode.sh 6 -1 -1
scripts/test_mode.sh 7 -1 -1
scripts/test_mode.sh 8 -1 -1
scripts/test_mode.sh 6 -2 -1
scripts/test_mode.sh 7 -2 -1
scripts/test_mode.sh 8 -2 -1
scripts/test_mode.sh 9 0 0
scripts/test_mode.sh 10 0 0
scripts/test_mode.sh 11 0 0
set +e
[[ "$(./minimp3)" != "error: no file names given" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 do_not_exist)" != "error: read function failed, code=-3" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -z)" != "error: unrecognized option" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m -1 vectors/l3-sin1k0db.bit)" != "error: unknown mode" ]] && echo fail && exit 1 || echo pass
[[ ! "$(./minimp3 vectors/l3-nonstandard-id3v1.bit vectors/ILL2_mono.pcm)" =~ "rate=48000 samples=1152 max_diff=17637 PSNR=15" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 vectors/l3-nonstandard-id3v1.bit - temp.pcm)" != "rate=48000 samples=1152 max_diff=0 PSNR=99.000000" ]] && echo fail && exit 1 || echo pass
rm temp.pcm

[[ "$(./minimp3 -m 2 -e 0 vectors/l3-sin1k0db.bit)" != "error: read function failed, code=-3" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 2 -e 1 vectors/l3-sin1k0db.bit)" != "error: read function failed, code=-3" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 2 -e 2 vectors/l3-sin1k0db.bit)" != "error: read function failed, code=-3" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 2 -e 3 vectors/l3-sin1k0db.bit vectors/l3-sin1k0db.pcm)" != "error: reference and produced number of samples do not match (725760/716544)" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 2 -e 2 vectors/l3-nonstandard-id3v2.bit)" != "error: read function failed, code=-3" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 2 -e 3 vectors/l3-nonstandard-id3v2.bit)" != "error: read function failed, code=-3" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -m 5 -e 1 vectors/l3-nonstandard-id3v2.bit)" != "error: read function failed, code=-3" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 5 -e 2 vectors/l3-nonstandard-id3v2.bit)" != "error: read function failed, code=-3" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -m 8 -e 0 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_open()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -e 1 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_open()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -e 2 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_open()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -e 3 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_open()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -e 4 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_open()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -e 5 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_read() readed less than expected, last_error=-3" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -m 8 -s 2304 -e 5 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_seek()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -s 2304 -e 6 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_seek()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -s 2304 -e 7 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_seek()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -s 2304 -e 8 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_seek()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -s 2304 -e 9 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_seek()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -s 2304 -e 12 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_seek()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -s 2304 -e 4 vectors/l3-nonstandard-sin1k0db_lame_vbrtag.bit)" != "error: mp3dec_ex_seek()=-3 failed" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 vectors/l3-nonstandard-id3v2-only.bit vectors/l3-nonstandard-id3v2-only.pcm)" != "rate=0 samples=0 max_diff=0 PSNR=99.000000" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 3 vectors/l3-nonstandard-id3v2-only.bit vectors/l3-nonstandard-id3v2-only.pcm)" != "rate=0 samples=0 max_diff=0 PSNR=99.000000" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -m 2 vectors/l3-nonstandard-small.bit vectors/l3-nonstandard-small.pcm)" != "rate=0 samples=0 max_diff=0 PSNR=99.000000" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 5 vectors/l3-nonstandard-small.bit vectors/l3-nonstandard-small.pcm)" != "rate=0 samples=0 max_diff=0 PSNR=99.000000" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -m 6 -s 215 -b vectors/l3-sin1k0db.bit -)" != "rate=44100 samples=725760 max_diff=0 PSNR=99.000000" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 6 -s 633 -b vectors/l3-sin1k0db.bit -)" != "rate=44100 samples=723456 max_diff=0 PSNR=99.000000" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -f 0 vectors/l3-sin1k0db.bit vectors/l3-sin1k0db.pcm)" != "error: not enough memory" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -f 1 vectors/l3-sin1k0db.bit vectors/l3-sin1k0db.pcm)" != "error: read function failed, code=-3" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -f 2 vectors/l3-sin1k0db.bit vectors/l3-sin1k0db.pcm)" != "error: read function failed, code=-2" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -f 3 vectors/l3-sin1k0db.bit vectors/l3-sin1k0db.pcm)" != "error: read function failed, code=-2" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -m 2 -f 0 vectors/l3-sin1k0db.bit)" != "error: not enough memory" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 2 -f 1 vectors/l3-sin1k0db.bit)" != "error: read function failed, code=-2" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 2 -f 2 vectors/l3-sin1k0db.bit)" != "error: read function failed, code=-2" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 2 -f 3 vectors/l3-sin1k0db.bit)" != "error: read function failed, code=-2" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -m 3 -f 1 vectors/l3-sin1k0db.bit)" != "error: read function failed, code=-2" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -m 6 -f 1 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_open()=-2 failed" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -m 8 -f 0 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_open()=-2 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -f 1 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_open()=-2 failed" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -m 8 -s 1 vectors/l3-nonstandard-vbrtag-only.bit)" != "error: mp3dec_ex_read() readed less than expected, last_error=0" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -s 1 -e 4 vectors/l3-nonstandard-vbrtag-only.bit)" != "error: mp3dec_ex_seek()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 8 -s 1 -e 5 vectors/l3-nonstandard-vbrtag-only.bit)" != "error: mp3dec_ex_seek()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 6 -s 1 -f 2 vectors/l3-nonstandard-sin1k0db_lame_vbrtag.bit)" != "error: mp3dec_ex_seek()=-2 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 vectors/l3-nonstandard-vbrtag-empty.bit)" != "rate=0 samples=0 max_diff=0 PSNR=99.000000" ]] && echo fail && exit 1 || echo pass
# Currently vbrtag with no frames flag treated as no vbrtag, not sure if any software produce such files.
# Delay and padding usage can be implemented if such software/files exists.
[[ "$(./minimp3 vectors/l3-nonstandard-vbrtag-noframes.bit)" != "rate=44100 samples=0 max_diff=0 PSNR=99.000000" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 6 vectors/l3-nonstandard-vbrtag-noframes.bit)" != "rate=44100 samples=0 max_diff=0 PSNR=99.000000" ]] && echo fail && exit 1 || echo pass

[[ "$(./minimp3 -m 9 vectors/l3-sin1k0db.pcm)" != "info: not an mp3/mpa file" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 9 vectors/l3-nonstandard-small.bit)" != "info: not an mp3/mpa file" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 11 -e 0 vectors/l3-sin1k0db.bit)" != "error: mp3dec_detect*()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 11 -e 1 vectors/l3-sin1k0db.bit)" != "error: mp3dec_detect*()=-3 failed" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -m 11 -e 2 vectors/l3-sin1k0db.bit)" != "error: mp3dec_detect*()=-3 failed" ]] && echo fail && exit 1 || echo pass
set -e

./minimp3 -m 6 -s 215 -b vectors/l3-sin1k0db.bit vectors/l3-sin1k0db.pcm
./minimp3 -m 6 -s 633 -b vectors/l3-sin1k0db.bit vectors/l3-sin1k0db_ofs633.pcm
./minimp3 -m 8 -s 215 -b vectors/l3-sin1k0db.bit vectors/l3-sin1k0db.pcm
./minimp3 -m 8 -s 633 -b vectors/l3-sin1k0db.bit vectors/l3-sin1k0db_ofs633.pcm

./minimp3 -m 6 -s 2304 vectors/l3-sin1k0db.bit vectors/l3-sin1k0db.pcm
./minimp3 -m 8 -s 2304 vectors/l3-sin1k0db.bit vectors/l3-sin1k0db.pcm
./minimp3 -m 8 -p 2000 vectors/l3-nonstandard-sin1k0db_lame_vbrtag.bit vectors/l3-nonstandard-sin1k0db_lame_vbrtag.pcm
./minimp3 -m 8 -p 725759 vectors/l3-nonstandard-sin1k0db_lame_vbrtag.bit vectors/l3-nonstandard-sin1k0db_lame_vbrtag.pcm

./minimp3 -t vectors/l3-sin1k0db.bit

gcov minimp3_test.c

echo testing x86 w/o sse...
gcc $CFLAGS -m32 -o minimp3 minimp3_test.c -lm
scripts/test.sh

echo testing x64...
gcc $CFLAGS -o minimp3 minimp3_test.c -lm
scripts/test.sh

echo testing x64 with float output...
gcc $CFLAGS -DMINIMP3_FLOAT_OUTPUT -o minimp3 minimp3_test.c -lm
scripts/test.sh
scripts/test_mode.sh 1 0 0
scripts/test_mode.sh 2 0 0
scripts/test_mode.sh 3 0 0
scripts/test_mode.sh 4 0 0
scripts/test_mode.sh 5 0 0
scripts/test_mode.sh 6 -1 -1
scripts/test_mode.sh 7 -1 -1
scripts/test_mode.sh 8 -1 -1

echo testing arm w/o neon...
arm-none-eabi-gcc $CFLAGS -mthumb -mcpu=arm9e -o minimp3_arm minimp3_test.c --specs=rdimon.specs -lm
qemu-arm ./minimp3_arm

echo testing arm w neon...
arm-none-eabi-gcc $CFLAGS -marm -mcpu=cortex-a15 -mfpu=neon -mfloat-abi=softfp -o minimp3_arm minimp3_test.c --specs=rdimon.specs -lm
qemu-arm ./minimp3_arm

echo testing arm64...
aarch64-linux-gnu-gcc $CFLAGS -static -march=armv8-a -o minimp3_arm minimp3_test.c -lm
qemu-aarch64 ./minimp3_arm

echo testing powerpc...
powerpc-linux-gnu-gcc $CFLAGS -static -o minimp3_ppc minimp3_test.c -lm
qemu-ppc ./minimp3_ppc

if [ ! "$TRAVIS" = "true" ]; then
echo testing powerpc64...
powerpc64-linux-gnu-gcc $CFLAGS -static -o minimp3_ppc minimp3_test.c -lm
qemu-ppc64 ./minimp3_ppc
fi

if [ ! "$TRAVIS" = "true" ]; then
rm emmintrin.h.gcov minimp3_arm minimp3_ppc minimp3_test.gcda minimp3_test.gcno minimp3_test.c.gcov xmmintrin.h.gcov
fi
