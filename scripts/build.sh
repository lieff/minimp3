_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

set -e

gcc -coverage -O0 -o minimp3 minimp3_test.c -lm
gcov minimp3_test.c

scripts/test.sh

gcc -O2 -g -Wall -Wextra -fno-asynchronous-unwind-tables -fno-stack-protector -ffunction-sections \
-fdata-sections -Wl,--gc-sections -o minimp3 minimp3_test.c -lm

scripts/test.sh
