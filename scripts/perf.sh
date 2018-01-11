_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

APP=./minimp3

for i in fate-suite.ffmpeg.org/mp3-conformance/*.bit; do
perf stat -e cycles $APP $i ${i%.*}.pcm
done

