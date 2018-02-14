_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

set -e

gcc -coverage -O0 -m32 -std=c89 -msse2 -DMINIMP3_TEST -DMINIMP3_NO_WAV -o minimp3 minimp3_test.c -lm
scripts/test.sh
set +e
./minimp3
./minimp3 do_not_exist
set -e
gcov minimp3_test.c

gcc -O2 -m32 -std=c89 -Wall -Wextra -Wmissing-prototypes -Werror -fno-asynchronous-unwind-tables -fno-stack-protector \
-ffunction-sections -fdata-sections -Wl,--gc-sections -o minimp3 minimp3_test.c -lm
scripts/test.sh

gcc -O2 -std=c89 -Wall -Wextra -Wmissing-prototypes -Werror -fno-asynchronous-unwind-tables -fno-stack-protector \
-ffunction-sections -fdata-sections -Wl,--gc-sections -o minimp3 minimp3_test.c -lm
scripts/test.sh

arm-none-eabi-gcc -O2 -std=c89 -Wall -Wextra -Wmissing-prototypes -Werror -fno-asynchronous-unwind-tables -fno-stack-protector \
-mthumb -mcpu=cortex-m4 \
-ffunction-sections -fdata-sections -Wl,--gc-sections -o minimp3_arm minimp3_test.c --specs=rdimon.specs -lm
qemu-arm ./minimp3_arm

arm-none-eabi-gcc -O2 -std=c89 -Wall -Wextra -Wmissing-prototypes -Werror -fno-asynchronous-unwind-tables -fno-stack-protector \
-marm -mcpu=cortex-a15 -mfpu=neon -mfloat-abi=softfp \
-ffunction-sections -fdata-sections -Wl,--gc-sections -o minimp3_arm minimp3_test.c --specs=rdimon.specs -lm
qemu-arm ./minimp3_arm

aarch64-linux-gnu-gcc -O2 -std=c89 -Wall -Wextra -Wmissing-prototypes -Werror -fno-asynchronous-unwind-tables -fno-stack-protector \
-static -march=armv8-a \
-ffunction-sections -fdata-sections -Wl,--gc-sections -o minimp3_arm minimp3_test.c -lm
qemu-aarch64 ./minimp3_arm

if [ ! "$TRAVIS" = "true" ]; then
rm emmintrin.h.gcov minimp3_arm minimp3_test.gcda minimp3_test.gcno minimp3_test.c.gcov xmmintrin.h.gcov
fi
