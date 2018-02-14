_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

afl-clang-fast -fsanitize=address,undefined -fno-sanitize-recover=address,undefined -o minimp3_fuzz minimp3_test.c -lm
afl-fuzz -d -m none -i vectors/fuzz -o fuzz_out -- ./minimp3_fuzz @@