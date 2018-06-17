#!/bin/sh
# take data for DC offset calibration
val=10000
run=0
while [[ $val -lt 65536 ]]; do
  cfg=`printf "run_%06d.000001.cfg" $run`
  sed -r -e '/^channel_DC_offset/ s/[0-9]+/'$val'/' \
    -e '/^channel_enable_recording/ s/no/yes/' \
    -e '/^channel_enable_trigger/ s/no/yes/' daq.cfg > $cfg
  echo ./daq.exe $cfg $run 1
  ./daq.exe $cfg $run 1
  val=$((val+1024))
  run=$((run+1))
done

root -l -b -q "offset.C($((run-1)))"

rm -f run_*
