FROM almalinux:8.10

# Enable PowerTools (necessary to install ninja-build)
RUN dnf install -y epel-release 'dnf-command(config-manager)' && \
    dnf config-manager --set-enabled powertools

# Install OpenSim Creator development dependencies
RUN dnf update -y && \
    dnf groupinstall "Development Tools" -y && \
    dnf install -y git cmake pkgconfig ninja-build dpkg-dev python3.12 python3.12-pip python3.12-devel gcc-toolset-15-gcc gcc-toolset-15-gcc-c++ libX11-devel libXext-devel xorg-x11-server-Xvfb mesa-libGL-devel mesa-dri-drivers && \
    dnf clean all

# Automatically activate GCC 15 when running a container
ENV BASH_ENV=/opt/rh/gcc-toolset-15/enable \
    ENV=/opt/rh/gcc-toolset-15/enable \
    PROMPT_COMMAND=". /opt/rh/gcc-toolset-15/enable"

# Set the default shell
CMD ["/bin/bash"]
