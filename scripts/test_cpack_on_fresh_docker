#!/usr/bin/env bash

cpack
docker run --rm -it -w $PWD -v $PWD:$PWD ubuntu /bin/bash -c "apt-get update && apt-get install -yf ./osmv_0.0.1_amd64.deb && apt-get install libglx0 libglu1-mesa libopengl0 && bash"