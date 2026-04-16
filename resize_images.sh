#!/bin/bash

mkdir -p assets_small

for img in assets/*.jpg; do
    filename=$(basename "$img")

    echo "Processing $filename..."

    magick "$img" \
        -resize 1024x1024\> \
        -strip \
        -quality 80 \
        "assets_small/$filename"

done

echo "Done resizing."
