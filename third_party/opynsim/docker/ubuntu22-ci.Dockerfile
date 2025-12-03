FROM ubuntu:22.04

ARG CMAKE_VERSION=3.29.3
ARG CMAKE_BUILD=3.29

# Avoid interactive prompts during package installs
ENV DEBIAN_FRONTEND=noninteractive

# Install system dependencies and build essentials
RUN apt-get update && apt-get install -y \
    software-properties-common \
    build-essential \
    curl \
    gcc-12 \
    g++-12 \
    ninja-build \
    git \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Make gcc-12 the default
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-12 100 \
 && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-12 100

# add deadsnakes ppa for python 3.12
run add-apt-repository ppa:deadsnakes/ppa -y && \
    apt-get update && apt-get install -y \
    python3.12 \
    python3.12-dev \
    python3.12-venv \
    && rm -rf /var/lib/apt/lists/*

# make python3 point to python3.12
run update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.12 1

# optionally install pip for python 3.12
run curl -ss https://bootstrap.pypa.io/get-pip.py | python3

# install newer cmake
RUN apt-get update && apt-get install -y \
    ca-certificates gnupg software-properties-common
RUN apt-key adv --fetch-keys 'https://apt.kitware.com/keys/kitware-archive-latest.asc' && \
    echo 'deb https://apt.kitware.com/ubuntu/ jammy main' | tee /etc/apt/sources.list.d/kitware.list && \
    apt-get update && \
    apt-get install -y cmake

# default shell
cmd ["/bin/bash"]

