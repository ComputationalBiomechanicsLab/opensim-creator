FROM quay.io/pypa/manylinux_2_34_x86_64

RUN yum install -y epel-release && yum install -y ninja-build

# Make Python 3.12 the default
RUN alternatives --install /usr/bin/python3 python3 /opt/python/cp312-cp312/bin/python3 100

# default shell
cmd ["/bin/bash"]

