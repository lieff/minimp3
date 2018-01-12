_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

APP=./minimp3

for i in vectors/*.bit; do
perf stat -e cycles $APP $i ${i%.*}.pcm
done

