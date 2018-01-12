_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

set -e

gcc -O2 -g -Wall -Wextra -o minimp3 minimp3_test.c -lm

APP=./minimp3

set +e
for i in vectors/*.bit; do
$APP $i ${i%.*}.pcm
retval=$?
echo $i exited with code=$retval
if [ ! $retval -eq 0 ]; then
  exit 1
fi
done
