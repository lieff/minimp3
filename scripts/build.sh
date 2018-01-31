_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

set -e

gcc -coverage -O0 -m32 -std=c89 -msse2 -DMINIMP3_TEST -DMINIMP3_NO_WAV -o minimp3 minimp3_test.c -lm
scripts/test.sh
gcov minimp3_test.c

gcc -O2 -g -std=c89 -Wall -Wextra -Wmissing-prototypes -Werror -fno-asynchronous-unwind-tables -fno-stack-protector \
-ffunction-sections -fdata-sections -Wl,--gc-sections -o minimp3 minimp3_test.c -lm
scripts/test.sh

arm-none-eabi-gcc -O2 -g -std=c89 -Wall -Wextra -Wmissing-prototypes -Werror -fno-asynchronous-unwind-tables -fno-stack-protector \
-mthumb -mcpu=cortex-m4 \
-ffunction-sections -fdata-sections -Wl,--gc-sections -o minimp3_arm minimp3_test.c --specs=rdimon.specs -lm
qemu-arm ./minimp3_arm

#arm-none-eabi-gcc -O2 -g -std=c89 -Wall -Wextra -Wmissing-prototypes -Werror -fno-asynchronous-unwind-tables -fno-stack-protector \
#-marm -mcpu=cortex-a15 -mfpu=neon -mfloat-abi=softfp \
#-ffunction-sections -fdata-sections -Wl,--gc-sections -o minimp3_arm minimp3_test.c --specs=rdimon.specs -lm
#qemu-arm ./minimp3_arm
