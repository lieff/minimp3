_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

set -e

if [ ! -d "fate-suite.ffmpeg.org" ]; then
  wget -np -r --level=1 http://fate-suite.ffmpeg.org/mp3-conformance/
  cp vectors/l3-he_mode16.pcm fate-suite.ffmpeg.org/mp3-conformance/he_mode.pcm
fi

gcc -O2 -g -o minimp3 minimp3_test.c -lm

APP=./minimp3

set +e
for i in fate-suite.ffmpeg.org/mp3-conformance/*.bit; do
$APP $i ${i%.*}.pcm
retval=$?
echo $i exited with code=$retval
if [ ! $retval -eq 0 ]; then
  exit 1
fi
done
