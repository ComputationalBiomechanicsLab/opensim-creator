.. _buildingfromsource:


Building From Source
====================

.. note::
    
    The build instructions here are for general users who just want to build OSC.

    Because everyone's C++ build environment is *slightly* different, there are
    no catch-all build instructions that will work for everyone. Instead, we
    recommend reading + running the automated build scripts (located at ``scripts/``
    in the source tree), or reading :ref:`Setup Development Environment`, which
    concretely covers tips-and-tricks for Visual Studio or QtCreator.


Building on Windows (10 or newer)
---------------------------------

Windows Environment Setup
^^^^^^^^^^^^^^^^^^^^^^^^^

The OpenSim Creator build requires that the development environment has ``git``
(for submodule downloads), a C++-20 compiler, cmake, ``NSIS`` (if packaging an
installer), and ``python`` (to run ``build_windows.py`` and building this
documentation). Here is a step-by-step guide for setting up a typical development
environment:

1. Get ``git``:
    1. Download and install ``git`` from https://git-scm.com/downloads
    2. Make sure to add it to the ``PATH``. Usually, the installer asks if you
       want this. If it doesn't ask, then you may need to add it manually (google:
       "Modify windows PATH", add your ``git`` install, which defaults to ``C:\Program Files\Git\bin``).
    3. Verify it's installed by opening a terminal (``Shift+Right-Click`` -> ``Open Powershell window here``) and run ``git``

2. Get a C++20-compatible compiler (``Visual Studio 17 2022``):
    1. Download and install it from https://visualstudio.microsoft.com/downloads/
    2. Make sure to select C/C++ development in the installer wizard when it asks
       you which parts you would like to install

3. Get ``cmake``:
    1. Download and install it from https://cmake.org/download/
    2. Make sure to add it to the ``PATH``. Usually, the installer asks if you want
       this. If it doesn't ask, then you may need to add it manually (google:
       "Modify windows PATH", add your ``cmake`` install: ``C:\Program Files\CMake\bin``).
    3. Verify it's installed by opening a terminal (``Shift+Right-Click`` -> ``Open Powershell window here``)
       and run ``cmake``.

4. Get ``NSIS`` (if packaging an installer):
    1. Download and install it from https://nsis.sourceforge.io/Download

5. Get ``python`` and ``pip``:
    1. Download from https://www.python.org/downloads/
    2. Make sure ``python`` and ``pip`` are added to the ``PATH`` (the installer usually prompts this)
    3. Verify they are installed by opening a terminal (``Shift+Right-Click`` -> ``Open Powershell window here``) and run ``python --help`` and ``pip --help``

6. Clone the ``opensim-creator`` source code (+ submodules) repository:
    1. Open a terminal, ``cd`` to your workspace directory (e.g. ``Desktop``),
       and run ``git clone --recursive https://github.com/ComputationalBiomechanicsLab/opensim-creator``
    2. The resulting ``opensim-creator`` directory should contain all necessary
       source code to build the project (incl. dependencies, submodules, etc.)

6. Install ``pip`` package dependencies (if building these documentation pages):
    1. Using either a virtual environment (google it), or your base ``python``
       installation, ``cd`` into the ``opensim-creator`` directory in a terminal
       and install the documentation package dependencies with:

.. code-block:: bash

    pip install -r docs/requirements.txt
    pip install -r docs/requirements-dev.txt


Windows Build
^^^^^^^^^^^^^

Assuming your environment has been set up correctly (explained above), the
easiest way to build OpenSim Creator is with the python script located at
``scripts/build_windows.py`` in the source code repository. The steps are:

1. Open a PowerShell terminal (``Shift+Right-Click`` -> ``Open Powershell window here``)
2. Either ``cd`` into the ``opensim-creator`` directory (if cloned when you setup
   the environment, above), or clone it with ``git clone --recursive https://github.com/ComputationalBiomechanicsLab/opensim-creator``.
3. Run the build script: ``python scripts/build_windows.py``. **Note**: this can
   take a long time, grab a coffee ☕
4. The ``osc-build`` directory should contain the built installer


Building on MacOS (Ventura or newer)
------------------------------------

1. Get ``brew``:
    1. Go to https://brew.sh/ and follow installation instructions
2. Get ``git``: 
    1. Can be installed via ``brew``: ``brew install git``
3. Get C++20-compatible compiler (e.g. ``clang`` via brew, or newer XCodes):
    1. OpenSim Creator is a C++20 project, so you'll have to use a more recent XCode (>14), or
       install a newer ``clang`` from brew (e.g. ``brew install clang``)
4. Get ``cmake``:
    1. Can be installed via ``brew``: ``brew install cmake``
5. Get ``python`` and ``pip`` (*optional*: you only need this if you want to build documentation):
    1. Can be installed via ``brew``: ``brew install python``
6. Build OpenSim Creator in a terminal:
    1. Clone ``opensim-creator``: ``git clone --recursive https://github.com/ComputationalBiomechanicsLab/opensim-creator``
    2. ``cd`` into the source dir: ``cd opensim-creator``
    3. If you have multiple C++ compilers, make sure that the ``CC`` and ``CXX`` environment variables
       point to compilers that are compatible with C++20. E.g. ``export CXX=$(brew --prefix llvm@15)/bin/clang++``
    4. Run the build script: ``scripts/build_mac.sh`` (**warning**: can take a long time)
7. Done:
    1. The ``osc-build`` directory should contain the built installer


Building on Ubuntu (20 or newer)
--------------------------------

1. Get ``git``:
    1. Install ``git`` via your package manager (e.g. ``apt-get install git``)

2. Get a C++20-compatible compiler:
    1. Install ``g++`` / ``clang++``` via your package manager (e.g. ``apt-get install g++``)
    2. They must be new enough to compile C++20 (e.g. clang >= clang-11)
    3. If they aren't new enough, most OSes provide a way to install a newer compiler
       toolchain (e.g. ``apt-get install clang-11``). You can configure which compiler is used
       to build OpenSim Creator by setting the ``CC`` and ``CXX`` environment variables. E.g.
       ``CC=clang-11 CXX=clang++-11 ./scripts/build_debian-buster.sh``
3. Get C++20-compatible standard library headers (usually required on Ubuntu 20):
    1. ``sudo apt-get install libstdc++-10-dev``
4. Get ``cmake``:
    1. Install ``cmake`` via your package manager (e.g. ``apt-get install cmake``)
    2. If your cmake is too old, build one from source, see: https://askubuntu.com/a/865294
5. Get ``python`` and ``pip`` (*optional*: you only need this if you want to build documentation):
    1. Install ``python3`` and ``pip3`` via your package manager (e.g. ``apt-get install python3 pip3``)
6. Build OpenSim Creator in a terminal:
    1. Clone ``opensim-creator``: ``git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator --recursive``
    2. ``cd`` into the source dir: ``cd opensim-creator``
    3. If you have multiple C++ compilers, make sure that the ``CC`` and ``CXX`` environment variables point to
       compilers that are compatible with C++20. E.g. ``export CC=clang-12``, ``export CXX=clang++-12``
    4. Run the build script: ``scripts/build_debian-buster.sh``
7. Done:
    1. The ``osc-build`` directory should contain the built installer
