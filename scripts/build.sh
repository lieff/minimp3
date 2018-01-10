set -e

if [ ! -d "fate-suite.ffmpeg.org" ]; then
  wget -np -r --level=1 http://fate-suite.ffmpeg.org/mp3-conformance/
fi

gcc -Dminimp3_test -O2 -g -o minimp3 minimp3.c

APP=./minimp3

set +e
for i in fate-suite.ffmpeg.org/mp3-conformance/*.bit; do
$($APP $i $i.out)
retval=$?
echo $i exited with code=$retval
cmp -s $i $i.out > /dev/null
if [ $? -eq 1 ]; then
    echo test failed
    exit 1
fi
done
