_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

set -e

gcc -coverage -O0 -DMINIMP3_TEST -DMINIMP3_NO_WAV -o minimp3 minimp3_test.c -lm
scripts/test.sh
gcov minimp3_test.c

gcc -O2 -g -Wall -Wextra -fno-asynchronous-unwind-tables -fno-stack-protector -ffunction-sections \
-fdata-sections -Wl,--gc-sections -o minimp3 minimp3_test.c -lm
scripts/test.sh
