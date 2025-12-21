#!/bin/bash

# takes the huge icons (64x64) and resizes them to large (32x32), medium (16x16) and small (8x8)
# usage: ./icon-resize.sh path/to/icons

echo "Converting 32x32..."
for f in ${1}/huge/*.png; do
    echo "   $f"
    convert "$f" -resize 32x32 "${1}/large/$(basename "$f")"
done
echo "Converting 16x16..."
for f in ${1}/huge/*.png; do
    echo "   $f"
    convert "$f" -resize 16x16 "${1}/medium/$(basename "$f")"
done
echo "Converting 8x8..."
for f in ${1}/huge/*.png; do
    echo "   $f"
    convert "$f" -resize 8x8 "${1}/small/$(basename "$f")"
done
