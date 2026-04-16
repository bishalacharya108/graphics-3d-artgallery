#!/bin/bash

mkdir -p assets_small

for img in assets/*.jpg; do
    filename=$(basename "$img")
    echo "Processing $filename..."

    magick "$img" \
        -resize 640x640\> \
        -strip \
        -gaussian-blur 0x0.5 \
        -sampling-factor 4:2:0 \
        -interlace Plane \
        -quality 70 \
        -colors 128 \
        -dither FloydSteinberg \
        "assets_small/$filename"
done

echo "Done."
