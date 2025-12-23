#!/bin/bash

# takes the huge icons (64x64) and resizes them to large (32x32), medium (16x16) and small (8x8)
# usage: ./icon-resize.sh path/to/icons

echo "Converting 32x32..."
for f in ${1}/huge/*.png; do
    echo "   $f"
    convert "$f" -filter Lanczos -colorspace sRGB -type TrueColorAlpha -define png:color-type=6 -define png:bit-depth=8 -define png:compression-level=9 -define png:filter=none -interlace none -resize 32x32 "${1}/large/$(basename "$f")"
done
echo "Converting 16x16..."
for f in ${1}/huge/*.png; do
    echo "   $f"
    convert "$f" -filter Lanczos -colorspace sRGB -type TrueColorAlpha -define png:color-type=6 -define png:bit-depth=8 -define png:compression-level=9 -define png:filter=none -interlace none -resize 16x16 "${1}/medium/$(basename "$f")"
done
echo "Converting 8x8..."
for f in ${1}/huge/*.png; do
    echo "   $f"
    convert "$f" -filter Lanczos -colorspace sRGB -type TrueColorAlpha -define png:color-type=6 -define png:bit-depth=8 -define png:compression-level=9 -define png:filter=none -interlace none -resize 8x8 "${1}/small/$(basename "$f")"
done


#magick input.png -resize 16x16 \
#    -filter Lanczos \
#    -define filter:blur=1 \
#    -colorspace sRGB \
#    -type TrueColorAlpha \
#    -define png:color-type=6 -define png:bit-depth=8 -define png:compression-level=9 -`define png:filter=none -interlace none
#    -strip \
#    output.png