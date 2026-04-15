OPynSim As a C++ Library
========================


.. warning::

    **The OPynSim C++ API is unstable.** This guide is aimed at C++ developers that
    accept the risks.

    The OPynSim C++ API (i.e. anything in ``libopynsim/`` or namespaced with ``opyn::``)
    and the oscar C++ API (``liboscar/``, ``osc::``) are **internal** APIs. They
    exist to service the OPynSim python API (public) and OpenSim Creator's implementation
    (internal). Therefore, if we think it makes sense to refactor/break those
    APIs in order to better-service those needs, we will.

    All of this is to say, it's possible to use these C++ APIs in your downstream
    project, but you should probably freeze the OPynSim version you use. Otherwise,
    upgrades to OPynSim might break your C++ code. Any issues/emails/requests with
    content like 'your change broke my C++ code' will receive a response along the
    lines of 'read the friendly manual'.

Doxygen Documentation
---------------------

Go `here <../doxygen/html>`_ to view the (very patchy and unofficial) Doxygen
documentation for the ``libopynsim`` API.


Walkthrough
-----------

OPynSim is an infrastructural project that's designed to be built from source
on Linux, Windows, and macOS. There's a cmake build for the 3rd-party dependencies
in ``third_party`` and there's a cmake build for the OPynSim project in the root
of the repository.

Here's an example bash script for building and installing everything from source:

.. code-block:: bash

    #!/usr/bin/env bash

    build_type=Release          # alternatively: RelWithDebInfo/Debug/MinSizeRel
    build_dir=${PWD}/build      # build   it in OPynSim's source directory
    install_dir=${PWD}/install  # install it in OpynSim's source directory

    # Get OPynSim's source code (incl. all third-party code)
    git clone https://github.com/opynsim/opynsim

    # Change into the OPynSim directory
    cd opynsim/

    # Build + install OPynSim's third-party dependencies
    cmake -G Ninja -S third_party/ -B ${build_dir}/third_party -DCMAKE_BUILD_TYPE=${build_type} -DCMAKE_INSTALL_PREFIX=${install_dir}
    cmake --build ${build_dir}/third_party

    # Build + install OPynSim (excl. Python bindings)
    cmake -G Ninja -S . -B ${build_dir} -DCMAKE_BUILD_TYPE=${build_type} -DCMAKE_PREFIX_PATH=${install_dir} -DCMAKE_INSTALL_PREFIX=${install_dir} -DBUILD_TESTING=OFF -DOPYN_BUILD_PYTHON_BINDINGS=OFF
    cmake --build ${build_dir} --target install

Once you've done that, ``${install_dir}`` will then contain a native install of
OPynSim and all of its dependencies. Downstream cmake projects can use any of
these dependencies, or OPynSim, with ``find_package``:

.. code-block:: cmake

    # Example CMakeLists.txt

    cmake_minimum_required(VERSION 3.25)
    project(your-project VERSION 0.0.1 LANGUAGES CXX)

    # `find_package` requires that the caller either installed the
    # distribution to a standard global location (e.g. to `/usr/local/`) or
    # the caller has set `CMAKE_PREFIX_PATH` to point at a custom
    # location (e.g. `-DCMAKE_PREFIX_PATH=${install_dir}`).
    find_package(opynsim REQUIRED)

    add_executable(your-executable your.cpp)
    target_link_libraries(your-executable PRIVATE opynsim)

Once your project has an appropriate ``CMakeLists.txt``, it should successfully
configure, provided it's called with the appropriate arguments, for example:

.. code-block:: bash

    #!/usr/bin/env bash

    # Configure your project, but ensure `find_package` can find the OPynSim installation
    cmake -S your-project -B build -DCMAKE_PREFIX_PATH=${install_dir}

    # Build your project and run it
    cmake --build build && ./build/your-executable


Installing Extra Stuff
~~~~~~~~~~~~~~~~~~~~~~

The walkthrough outlines the most straightforward way to build and install OPynSim
from source, but downstream projects tend to require additional dependencies. The
way we recommend doing it depends on what you're planning.

If you are building for a specific architecture and operating system, and you
aren't too bothered about tainting your system with additional libraries, the
easiest way is to just install stuff on your system (e.g.
``apt-get install tensorflow-dev``). Operating system maintainers such as the Debian
foundation, Canonical, and Red Hat have already built and tested these packages
for you. Once installed, it's usually just a matter of using cmake's ``find_package`` to
pull it into your project.

If you are building across multiple architectures and operating systems, then you
either have to handle system setup (installing libraries, etc.) on a system-by-system
basis, or build from source for each target. OPynSim opts for a source build. CMake
standardizes the configure-build-install pattern. E.g. if I you wanted to add glm
to my project then you'd build it and then install it into the same directory as
OPynSim was installed:

.. code-block:: bash

    #!/usr/bin/bash

    # Get the source code
    git clone https://github.com/g-truc/glm

    # Configure the build
    cmake -S glm -B glm-build -DCMAKE_INSTALL_PREFIX=${install_dir}

    # Build + install it into ${install_dir}
    cmake --build glm-build --target install


As with OPynSim, you would then provide a ``-DCMAKE_PREFIX_PATH=${install_dir}``
which would make ``find_package`` capable of finding ``glm``.
