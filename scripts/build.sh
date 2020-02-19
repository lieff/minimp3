_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

set -e

CFLAGS="-O2 -std=c89 -Wall -Wextra -Wmissing-prototypes -Werror -fno-asynchronous-unwind-tables -fno-stack-protector -U_FORTIFY_SOURCE \
-ffunction-sections -fdata-sections -Wl,--gc-sections"

echo testing mp4 mode...
gcc $CFLAGS -DMP4_MODE -o minimp3 minimp3_test.c -lm
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
set +e
[[ "$(./minimp3)" != "error: no file names given" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 do_not_exist)" != "error: read function failed, code=-3" ]] && echo fail && exit 1 || echo pass
[[ "$(./minimp3 -z)" != "error: unrecognized option" ]] && echo fail && exit 1 || echo pass
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
[[ "$(./minimp3 -m 8 -e 5 vectors/l3-sin1k0db.bit)" != "error: mp3dec_ex_read() readed less than expected, last_error=-5" ]] && echo fail && exit 1 || echo pass
set -e
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
