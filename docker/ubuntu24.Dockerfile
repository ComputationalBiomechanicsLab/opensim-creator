FROM ubuntu:24.04

# Avoid interactive prompts during package installs
ENV DEBIAN_FRONTEND=noninteractive

# Copy package list into container
COPY ubuntu24_packages.txt /tmp/ubuntu24_packages.txt

# Install system dependencies (+cleanup)
RUN apt-get update && \
    apt-get install -y $(sed 's/#.*//' /tmp/ubuntu24_packages.txt | xargs) && \
    rm -rf /var/lib/apt/lists/* /tmp/ubuntu24_packages.txt

# Set the default shell
cmd ["/bin/bash"]
