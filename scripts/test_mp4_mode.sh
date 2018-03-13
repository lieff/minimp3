_FILENAME=${0##*/}
CUR_DIR=${0/${_FILENAME}}
CUR_DIR=$(cd $(dirname ${CUR_DIR}); pwd)/$(basename ${CUR_DIR})/

pushd $CUR_DIR/..

APP=./minimp3

set +e
for i in vectors/l3-compl.bit vectors/l3-he_32khz.bit vectors/l3-he_44khz.bit vectors/l3-he_48khz.bit \
vectors/l3-hecommon.bit vectors/l3-he_mode.bit vectors/l3-si.bit vectors/l3-si_block.bit vectors/l3-si_huff.bit \
vectors/l3-sin1k0db.bit vectors/l3-test45.bit vectors/l3-test46.bit vectors/M2L3_bitrate_16_all.bit \
vectors/M2L3_bitrate_22_all.bit vectors/M2L3_bitrate_24_all.bit vectors/M2L3_compl24.bit vectors/M2L3_noise.bit; do
$APP $i ${i%.*}.pcm
retval=$?
echo $i exited with code=$retval
if [ ! $retval -eq 0 ]; then
  exit 1
fi
done
