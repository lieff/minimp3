_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

set -e

CFLAGS="-O2 -std=c89 -Wall -Wextra -Wmissing-prototypes -Werror -fno-asynchronous-unwind-tables -fno-stack-protector \
-ffunction-sections -fdata-sections -Wl,--gc-sections"

echo testing mp4 mode...
gcc $CFLAGS -DMP4_MODE -o minimp3 minimp3_test.c -lm
scripts/test_mp4_mode.sh

echo testing coverage x86 w sse...
gcc -coverage -O0 -m32 -std=c89 -msse2 -DMINIMP3_TEST -DMINIMP3_NO_WAV -o minimp3 minimp3_test.c -lm
scripts/test.sh
set +e
./minimp3
./minimp3 do_not_exist
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
