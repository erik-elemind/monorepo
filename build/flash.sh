#!/bin/bash

# process args
while getopts f:d: flag
do
    case "${flag}" in
        f) hexfile=${OPTARG};;
        d) device=${OPTARG};;
    esac
done

if [ -z "$hexfile" ]; then
    echo "please provide hex file."
    exit 1;
fi

if [ -z "$device" ]; then
    echo "please provide device."
    exit 1;
fi

# use this filename for the script
jlinkscriptfile=".flash.jlink"

# create the script:
# halt, flash the file, reset, and go
echo "h"                  > $jlinkscriptfile
echo "loadfile $hexfile" >> $jlinkscriptfile
echo "r"                 >> $jlinkscriptfile
echo "g"                 >> $jlinkscriptfile
echo "exit"              >> $jlinkscriptfile

# execute it
JLinkExe -NoGui 1 -autoconnect 1 -if SWD -speed 4000 -device ${device} ${jlinkscriptfile}

# leave no trace
rm $jlinkscriptfile
