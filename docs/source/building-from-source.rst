.. _buildingfromsource:


Building From Source
====================

.. note::

    The build instructions here are for general users who just want to build OpenSim Creator.

    Because everyone's C++ build environment is *slightly* different, there are
    no catch-all build instructions that will work for everyone. Instead, we
    recommend reading + running the automated build scripts (located at ``scripts/``
    in the source tree), or reading :ref:`Setup Development Environment`, which
    concretely covers tips-and-tricks for Visual Studio or QtCreator.


Building on Windows (10 or newer)
---------------------------------

Windows Environment Setup
^^^^^^^^^^^^^^^^^^^^^^^^^

The OpenSim Creator build requires that the development environment has ``git``,
a C++-20 compiler, cmake, ``WiX``/``NSIS`` (if packaging an installer), and ``python``.
Here is a step-by-step guide for setting up a typical development environment:

1. Get ``git``:
    1. Download and install ``git`` from https://git-scm.com/downloads
    2. Make sure to add it to the ``PATH``. Usually, the installer asks if you
       want this. If it doesn't ask, then you may need to add it manually (google:
       "Modify windows PATH", add your ``git`` install, which defaults to ``C:\Program Files\Git\bin``).
    3. Verify it's installed by opening a terminal (``Shift+Right-Click`` -> ``Open Powershell window here``) and run ``git``

2. Get Visual Studio >=2022 (C++ compiler - note: **Visual Studio is not the same as Visual Studio Code**):
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

4. Get ``NSIS`` (if `OSC_PACKAGE_WITH_NSIS` is `ON`, and you plan on packaging an installer ``.exe``):
    1. Download and install it from https://nsis.sourceforge.io/Download

5. Get ``WiX`` (can be disabled with -DOSC_PACKAGE_WITH_WIX=OFF, builds the ``.msi`` installer):
    1. Download and install WiX3 (e.g. ``wix314.exe``) from https://github.com/wixtoolset/wix3/releases
    2. Avoid using newer WiX versions because GitHub runner images etc. currently still use WiX3 (see: https://github.com/actions/runner-images/tree/main/images/windows)

6. Get ``python`` and ``pip``:
    1. Download from https://www.python.org/downloads/
    2. Make sure ``python`` and ``pip`` are added to the ``PATH`` (the installer usually prompts this)
    3. Verify they are installed by opening a terminal (``Shift+Right-Click`` -> ``Open Powershell window here``) and run ``python --help`` and ``pip --help``

7. Clone the ``opensim-creator`` source code repository:
    1. Open a terminal, ``cd`` to your workspace directory (e.g. ``Desktop``),
       and run ``git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator``
    2. The resulting ``opensim-creator`` directory should contain all necessary
       source code to build the project (incl. third_party code etc.)

8. Install ``pip`` package dependencies:
    1. Using either a virtual environment (google it), or your base ``python``
       installation, ``cd`` into the ``opensim-creator`` directory in a terminal
       and install python dependencies with:

.. code-block:: bash

    pip install -r requirements/all_requirements.txt


Windows Build
^^^^^^^^^^^^^

Assuming your environment has been set up correctly (explained above), the
easiest way to build OpenSim Creator is with the python script located at
``scripts/build.py`` in the source code repository. The steps are:

1. Open a PowerShell terminal (``Shift+Right-Click`` -> ``Open Powershell window here``)
2. Either ``cd`` into the ``opensim-creator`` directory (if cloned when you setup
   the environment, above), or clone it with ``git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator``.
3. Run the build script: ``python scripts/build.py``. **Note**: this can
   take a long time, grab a coffee ☕
4. The ``build/`` directory should contain the built installer


Building on MacOS (Sonoma or newer)
------------------------------------

1. Get ``brew``:
    1. Go to https://brew.sh/ and follow installation instructions
2. Get ``git``:
    1. Can be installed via ``brew``: ``brew install git``
3. Get C++23-compatible compiler (e.g. ``clang`` via brew, or newer XCodes):
    1. OpenSim Creator is a C++23 project, so you'll have to use a more recent XCode (>=15), or
       install a newer ``clang`` from brew (e.g. ``brew install clang``)
4. Get ``cmake``:
    1. Can be installed via ``brew``: ``brew install cmake``
5. Get ``python`` and ``pip``:
    1. Can be installed via ``brew``: ``brew install python``
6. Build OpenSim Creator in a terminal:
    1. Clone ``opensim-creator``: ``git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator``
    2. ``cd`` into the source dir: ``cd opensim-creator``
    3. If you have multiple C++ compilers, make sure that the ``CC`` and ``CXX`` environment variables
       point to compilers that are compatible with C++23. E.g. ``export CXX=$(brew --prefix llvm@15)/bin/clang++``
    4. Run the build script: ``python scripts/build.py`` (**warning**: can take a long time)
7. Done:
    1. The ``build/`` directory should contain the built installer


Building on Ubuntu (22.04 or newer)
-----------------------------------

1. Get ``git``:
    1. Install ``git`` via your package manager (e.g. ``apt-get install git``)
2. Get a C++23-compatible compiler:
    1. E.g. on Ubuntu 22.04, install ``g++-12`` or ``clang++``` via your package manager (e.g. ``apt-get install g++-12``)
3. Get ``cmake``:
    1. Install ``cmake`` via your package manager (e.g. ``apt-get install cmake``)
4. Get ``python`` and ``pip``:
    1. Install ``python3`` and ``pip3`` via your package manager (e.g. ``apt-get install python3 pip3``)
5. Use ``git`` to get OpenSim Creator's (+ dependencies') source code:
    1. Clone ``opensim-creator``: ``git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator``
    2. ``cd`` into the source dir: ``cd opensim-creator``
    3. The remaining build steps are performed in the source directory
6. Get python libraries:
    1. ``cd`` into the ``opensim-creator`` source directory (if you haven't already)
    2. Install all necessary python libraries into your current python environment with ``pip install -r requirements/all_requirements.txt``
7. Build OpenSim Creator from source:
    1. ``cd`` into the ``opensim-creator`` source directory (if you haven't already)
    2. Run the build script, you can use the ``CC`` and ``CXX`` environment variables to choose
       your C++ compiler if you're using the non-default one, e.g. ``CC=gcc-12 CXX=g++-12 python ./scripts/build.py``
    3. You can also accelerate it by setting the number of threads: ``OSC_BUILD_CONCURRENCY=20 python ./scripts/build.py``
8. Done:
    1. After the build is complete, the ``build/`` directory should contain the built installer
