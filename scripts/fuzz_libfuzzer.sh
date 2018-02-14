_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

clang-5.0 -g -fsanitize=address,undefined -fsanitize-coverage=trace-pc-guard -DLIBFUZZER -o minimp3_libfuzz minimp3_test.c -lFuzzer -lstdc++ -lm
./minimp3_libfuzz -max_len=1024 vectors/fuzz