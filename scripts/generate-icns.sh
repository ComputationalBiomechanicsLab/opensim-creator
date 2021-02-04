#!/usr/bin/env bash

inkscape=/Applications/Inkscape.app/Contents/MacOS/inkscape

for sz in 16 32 128 256 512; do
    ${inkscape} -w $sz -h $sz ../logo.svg --export-filename icon_${sz}x${sz}.png
    ${inkscape} -w $((2*sz)) -h $((2*sz)) ../logo.svg --export-filename icon_${sz}x${sz}@2x.png
done
iconutil --convert icns --output logo.icns .
