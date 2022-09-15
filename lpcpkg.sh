#!/bin/bash

# This helper script is used to generate an OTA file for the LPC image.
# It requires that the python modules for nrfutil and intelhex be installed.
# Usage:
# lpcpkg.sh -f <path/to/lpc/firmware.hex>

# process args
while getopts f: flag
do
    case "${flag}" in
        f) hexfile=${OPTARG};;
    esac
done

if [ ! -f "$hexfile" ]; then 
    echo "error: file does not exist."
    exit 1;
fi

# setup file names
file_base="${hexfile%.*}"
file_ext="${hexfile##*.}"
file_bin="$file_base.bin"
file_offset4kb="$file_base.offset4kb.$file_ext"
file_otazip="$file_base.ota.zip"

# check the extension
if [ "$file_ext" != "hex" ]; then
    echo "error: file must be a .hex file"
    exit 1
fi

# fail on error
set -e

# echo on
set -x

# convert to bin, so that we can use bin2hex to apply the offset for us
hex2bin.py "$hexfile" "$file_bin"

# convert back to hex, with offset
# the offset is required because nrfutil ignores the first 4kb of the hex file
# see ticket filed on the nordic developer forums here:
# https://devzone.nordicsemi.com/f/nordic-q-a/76171/nrfutil-package-generate-drops-portion-of-hexfile-at-address-0
bin2hex.py --offset=4096 "$file_bin" "$file_offset4kb"

# generate the signed OTA package
# note the use of --external-app since this image is meant for an external MCU
# rather than the NRF itself.
# for the sd-req field, see "nrfutil pkg generate --help" for the complete list.
# 0xCB is for s132_nrf52_7.0.1.
# the application-version field is not used, but is required by nrfutil.
nrfutil pkg generate \
--hw-version 52 \
--application-version 0 \
--external-app \
--key-file ./source_code/private-key.pem \
--sd-req 0xCB \
--application "$file_offset4kb" \
"$file_otazip"

# leave no trace
rm "$file_bin"
rm "$file_offset4kb"
