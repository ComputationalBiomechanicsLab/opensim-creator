FROM ubuntu:18.04

# Avoid interactive prompts during package installs
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y software-properties-common

# Add toolchain PPA for gcc-15
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get install -y git ninja-build pkg-config xdg-desktop-portal-gtk libx11-dev libxext-dev libgl1-mesa-dev xvfb gcc-15 g++-15 wget

# Install newer CMake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.28.3/cmake-3.28.3-linux-x86_64.sh -O /tmp/cmake-install.sh
RUN chmod +x /tmp/cmake-install.sh
RUN /tmp/cmake-install.sh --skip-license --prefix=/usr/local
RUN rm /tmp/cmake-install.sh

# Install `uv`
RUN wget https://astral.sh/uv/install.sh -O /tmp/uv-install.sh
RUN chmod +x /tmp/uv-install.sh
RUN UV_INSTALL_DIR="/usr/local/bin" /tmp/uv-install.sh
RUN rm /tmp/uv-install.sh

# Install Python 3.12 ready for `uv` to use
RUN uv python install 3.12

# Make gcc-15/g++-15 the default C/C++ compilers
RUN update-alternatives --install /usr/bin/cc cc /usr/bin/gcc-15 100 \
 && update-alternatives --install /usr/bin/c++ c++ /usr/bin/g++-15 100

# Set the default shell
cmd ["/bin/bash"]
