FROM almalinux:8.10

# Copy package list into container
COPY almalinux8_packages.txt /tmp/almalinux8_packages.txt

# Enable PowerTools (necessary to install ninja-build)
RUN dnf install -y epel-release 'dnf-command(config-manager)' && \
    dnf config-manager --set-enabled powertools

# Install system dependencies (+cleanup)
RUN dnf update -y && \
    dnf groupinstall "Development Tools" -y && \
    dnf install -y $(sed 's/#.*//' /tmp/almalinux8_packages.txt | xargs) && \
    dnf clean all && \
    rm /tmp/almalinux8_packages.txt

# Automatically activate GCC 13 when running a container
ENV BASH_ENV=/opt/rh/gcc-toolset-13/enable \
    ENV=/opt/rh/gcc-toolset-13/enable \
    PROMPT_COMMAND=". /opt/rh/gcc-toolset-13/enable"

# Set the default shell
CMD ["/bin/bash"]
