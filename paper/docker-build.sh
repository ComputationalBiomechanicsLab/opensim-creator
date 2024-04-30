#!/usr/bin/env bash

sudo docker run \
    --rm \
    --volume $PWD/paper:/data \
    --user $(id -u):$(id -g) \
    --env JOURNAL=joss \
    openjournals/inara
firefox paper/paper.pdf
