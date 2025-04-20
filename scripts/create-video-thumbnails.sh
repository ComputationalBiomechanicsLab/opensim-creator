#!/usr/bin/env bash

# `create-video-thumbnails.sh`: Used in `files.opensimcreator.com`
# to create thumbnail images for each video file, so that the
# website doesn't have to load the video file in the gallery page.
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

