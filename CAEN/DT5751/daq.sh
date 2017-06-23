#!/bin/bash

# get path where this script locates. It won't work for symbol links
dir="$(cd $(dirname "$0") && pwd)"

# check $NICEDAT
if [ "X$NICEDAT" = X ]; then NICEDAT=~/data/nice; fi
if [ ! -d $NICEDAT ]; then 
  echo Environment variable NICEDAT is set to $NICEDAT,
  echo which does not exist. Please set it to a valid location!
  read -n1 -r -p "Press any key to quit..." key
  exit
fi

# go to NICE data directory and get latest run number
cd $NICEDAT
subdir=`ls -d1 ?????? | tail -1`
cd $NICEDAT/$subdir
file=`ls -1 run_??????.000001 | tail -1`
six=${file:4:6}
run=`expr $six + 1`
# create a new subdir every 100 run
if [ `expr $run%100|bc` -eq 0 ]; then
  subdir=`printf %06d $run`
  mkdir -p $NICEDAT/$subdir
  cd $NICEDAT/$subdir
fi

# ask user to specify run number
echo "Type <Enter> to start run $run, or"
echo "an integer in [1, $run) to overwrite an old run:"
read -e -n ${#run} input
if [ "X$input" != X ]; then # not <Enter>
  while true; do # display promp until a valid input
    if [[ "$input" =~ ^[0-9]+$ ]]; then # positive integer
      if [ $input -eq $run ]; then break; fi
      if [ $input -lt $run ]; then
	echo; echo "Overwrite run $input? (y/N)"
	read -n1 answer
	if [ X$answer != Xy ]; then
	  echo; echo "Please input a new run number in [1, $run]:"
	  read -e -n ${#run} input
	  continue
	fi
	break
      fi
    fi
    echo; echo "Invalid run number: $input,"
    echo "please input an integer in [1, $run]:"
    read -e -n ${#run} input
  done
  run=$input
fi

# ask user to specify maximal number of events
echo; echo Type the maximal number of events in run $run, or
echo "<Enter> to skip the setting (One can use \"Ctrl+c\" to stop a run later):"
read -e input
if [ "X$input" != X ]; then # not <Enter>
  while true; do # display promp until a valid input
    if [[ "$input" =~ ^[0-9]+$ ]]; then
      if [ $input -le 4294967295 ]; then break; fi
    fi
    echo; echo Invalid number of events: $input,
    echo please input a positive integer:
    read -e input
  done
  n=$input
fi

# copy daq.cfg
cfgfile=`printf "run_%06d.000001.cfg" $run`
if [ ! -f $cfgfile ]; then cp $dir/daq.cfg $cfgfile; fi

# edit cfg file if nano or $EDITOR exists
if hash nano 2>/dev/null; then
  nano $cfgfile
elif hash $EDITOR 2>/dev/null; then
  $EDITOR $cfgfile
else
  echo "Cannot find a text editor to open $cfgfile"
  read -n1 -r -p "Press any key to quit..." key
  exit
fi

# strip off comment and blank lines from the cfg file
sed -i '/^#.*$/d' $cfgfile
sed -i '/^\s*$/d' $cfgfile

# run executable
name=$(basename $0)
exe=${name%.sh}.exe
echo $dir/$exe $cfgfile $run $n
$dir/$exe $cfgfile $run $n
read -n1 -r -p "Press any key to quit..." key
