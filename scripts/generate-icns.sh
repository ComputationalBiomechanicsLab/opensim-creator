#!/usr/bin/env bash

inkscape=/Applications/Inkscape.app/Contents/MacOS/inkscape

mkdir osc.iconset
cd osc.iconset

for sz in 16 32 128 256 512; do
    #${inkscape} -w $sz -h $sz resources/logo.svg --export-filename icon_${sz}x${sz}.png
    #${inkscape} -w $((2*sz)) -h $((2*sz)) resources/logo.svg --export-filename icon_${sz}x${sz}@2x.png

    convert ../resources/textures/logo.png -resize ${sz}x${sz} icon_${sz}x${sz}.png
    convert ../resources/textures/logo.png -resize $((2*sz))x$((2*sz)) icon_${sz}x${sz}@2x.png
done
cd -
iconutil --convert icns --output resources/osc.icns osc.iconset
rm -rf osc.iconset
