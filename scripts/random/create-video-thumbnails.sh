#!/usr/bin/env bash
#
# Creates thumbnail images for each video file in www.opensimcreator.com's gallery
# page. This is used in `files.opensimcreator.com` so that the  website doesn't have
# to load the video file (instead, it loads a smaller gallery screenshot).
#
# On Windows, you can use MSYS2 with `ffmpeg` installed to
# do this.
#
# This script's usually called with `files.opensimcreator.com`
# as the working directory.

for video_file in $(ls videos/*.{webm,mp4}); do
    video_filename=$(basename -- "${video_file}")
    thumbnail_filename="videos/${video_filename%.*}_thumbnail.jpg"
    if [[ ! -e "${thumbnail_filename}" ]]; then
    	ffmpeg -i "${video_file}" -s 480x270 -q:v 2 -frames:v 1 -y "${thumbnail_filename}"
    fi
done
