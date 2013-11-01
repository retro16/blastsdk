#!/bin/bash

if [ -z "$1" ] || [ "$1" == "--help" ] || [ "$1" == "-h" ]; then
cat <<EOF
Extracts security code from a Sega CD CD-ROM or a Sega CD ISO.
The region is automatically detected and the correct C file is generated.

Usage: $0 FILE

Examples:
  $0 /dev/cdrom          Extracts security code from CD-ROM
  $0 my_game.iso         Extracts security code from an image
EOF
exit 0
fi

make -s -C bin2c

if ! [ -r "$1" ]; then
echo "Cannot open $1 for reading."
exit 2
fi

dd if="$1" of=extract.tmp bs=512 count=3 skip=1 &>/dev/null

# Detect region and size
bin2c/bin2c extract.tmp extract.tmp.c
if grep '0x05,0x64,0x60,0x0f,0x00,0x00' extract.tmp.c &>/dev/null; then
echo "Europe boot code detected"
OUTFILE="bls/scdboot_eu"
SIZE=1390
elif grep '0x05,0x7a,0x60,0x0f,0x00,0x00' extract.tmp.c &>/dev/null; then
echo "US boot code detected"
OUTFILE="bls/scdboot_us"
SIZE=1412
else
echo "Japan boot code detected"
OUTFILE="bls/scdboot_jp"
SIZE=342
fi

dd if=extract.tmp of="$OUTFILE" bs=$SIZE count=1 &>/dev/null
bin2c/bin2c "$OUTFILE"

rm extract.tmp extract.tmp.c "$OUTFILE" bin2c/bin2c

echo "C code generated. Please run './setup force' to recompile blsbuild with correct data."
