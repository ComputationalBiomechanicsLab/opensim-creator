FROM ubuntu:22.04

ARG CMAKE_VERSION=3.29.3
ARG CMAKE_BUILD=3.29

# Avoid interactive prompts during package installs
ENV DEBIAN_FRONTEND=noninteractive

# Copy apt requirements file into the container
COPY ubuntu22_apt.txt /tmp/ubuntu22_apt.txt

# Install system dependencies (+cleanup)
RUN apt-get update && \
    apt-get install -y software-properties-common curl ca-certificates && \
    apt-get install -y $(sed 's/#.*//' /tmp/ubuntu22_apt.txt | xargs) && \
    rm -rf /var/lib/apt/lists/* /tmp/ubuntu22_apt.txt

# Install newer cmake (Ubuntu 22's is a little old)
RUN apt-get update && apt-get install -y \
    ca-certificates gnupg software-properties-common
RUN apt-key adv --fetch-keys 'https://apt.kitware.com/keys/kitware-archive-latest.asc' && \
    echo 'deb https://apt.kitware.com/ubuntu/ jammy main' | tee /etc/apt/sources.list.d/kitware.list && \
    apt-get update && \
    apt-get install -y cmake

# Make gcc-12 the default C/C++ compiler
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-12 100 \
 && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-12 100

# Set the default shell
cmd ["/bin/bash"]
