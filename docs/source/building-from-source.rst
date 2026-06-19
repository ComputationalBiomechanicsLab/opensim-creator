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
a C++-20 compiler, cmake, ``WiX`` (if packaging an installer), and ``python``. These
days, the Visual Studio installer provides the C++ compiler and cmake. Here is a
step-by-step guide for setting up a typical development environment:

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

3. Get ``WiX`` (can be disabled with -DOSC_CONFIGURE_PACKAGING=OFF, builds the ``.msi`` installer):
    1. Download and install WiX3 (e.g. ``wix314.exe``) from https://github.com/wixtoolset/wix3/releases
    2. Avoid using newer WiX versions because GitHub runner images etc. currently still use WiX3 (see: https://github.com/actions/runner-images/tree/main/images/windows)

4. Get ``python`` 3.12 and ``pip``:
    1. Download Python 3.12 from https://www.python.org/downloads/
    2. Make sure ``python`` and ``pip`` are added to the ``PATH`` (the installer usually prompts this)
    3. Verify they are installed by opening a terminal (``Shift+Right-Click`` -> ``Open Powershell window here``) and run ``python --help`` and ``pip --help``

5. Clone the ``opensim-creator`` source code repository:
    1. Open a terminal, ``cd`` to your workspace directory (e.g. ``Desktop``),
       and run ``git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator``
    2. The resulting ``opensim-creator`` directory should contain all necessary
       source code to build the project (incl. third_party code etc.)

6. Create a local python virtual environment with ``pip`` dependencies installed into it:
    1. Open a terminal, ``cd`` to the ``opensim-creator`` directory
    2. Run ``py -3.12 -m venv .venv`` then activate it with ``call .\.venv\Scripts\activate``, then install
       everything into it with ``pip install -r requirements/all_requirements.txt``


Windows Build
^^^^^^^^^^^^^

Assuming your environment has been set up correctly (explained above), the
easiest way to build OpenSim Creator is with an end-to-end build script. The steps
are:

1. Open a batch (``cmd.exe``) terminal.
2. Change into the ``opensim-creator`` source code directory (e.g. with ``cd opensim-creator``).
3. Activate the Visual Studio environment with ``call .\third_party\opynsim\scripts\env_vs-x64.bat``.
4. Build OpenSim Creator's dependencies with ``cd third_party && cmake --workflow --preset Development && cd ..``.
5. Build OpenSim Creator with ``cmake --workflow --preset Development``.
6. The build directory (``build\Development``) should contain the build outputs (e.g. the installer).


Building on MacOS (Sonoma or newer)
------------------------------------

MacOS Enrivonment Setup
^^^^^^^^^^^^^^^^^^^^^^^

1. Get ``brew``:
    1. Go to https://brew.sh/ and follow installation instructions
2. Get ``git``:
    1. Can be installed via ``brew``: ``brew install git``
3. Get C++23-compatible compiler (e.g. ``clang`` via brew, or newer XCodes):
    1. OpenSim Creator is a C++23 project, so you'll have to use a more recent XCode (>=15), or
       install a newer ``clang`` from brew (e.g. ``brew install clang``)
4. Get ``cmake``:
    1. Can be installed via ``brew``: ``brew install cmake``
5. Get ``python`` 3.12 and ``pip``:
    1. Recommended to use ``uv`` to install/manage multiple Python versions.
6. Clone the ``opensim-creator`` source code repository:
    1. Open a terminal, ``cd`` to your workspace directory (e.g. ``Desktop``),
       and run ``git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator``
    2. The resulting ``opensim-creator`` directory should contain all necessary
       source code to build the project (incl. third_party code etc.)
7. Create a local python virtual environment with ``pip`` dependencies installed into it:
    1. Open a terminal, ``cd`` to the ``opensim-creator`` directory
    2. Run ``python3.12 -m venv .venv``, then activate it with ``source .venv/bin/activate`` and then
       install all Python dependencies with ``pip install -r requirements/all_requirements.txt``.


MacOS Build
^^^^^^^^^^^

1. Build OpenSim Creator in a terminal:
    1. If you have multiple C++ compilers, make sure that the ``CC`` and ``CXX`` environment variables
       point to compilers that are compatible with C++23. E.g. ``export CXX=$(brew --prefix llvm@15)/bin/clang++``
    2. Build OpenSim Creator's C++ dependencies: ``cd third_party/ && cmake --workflow --preset Development && cd -``
    3. Build OpenSim Creator: ``cmake --workflow --preset Development``.


Building on Ubuntu (24.04 or newer)
-----------------------------------

Ubuntu Environment Setup
^^^^^^^^^^^^^^^^^^^^^^^^

1. Get ``git``:
    1. Install ``git`` via your package manager (e.g. ``apt-get install git``)
2. Use ``git`` to get OpenSim Creator's (+ dependencies') source code:
    1. Clone ``opensim-creator``: ``git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator``
    2. ``cd`` into the source dir: ``cd opensim-creator``
    3. The remaining build steps are performed in the source directory
3. Get ``apt`` dependencies:
    1. ``apt`` dependencies are listed in the ``third_party/opynsim/docker/`` directory with an ``_packages.txt`` suffix
4. Create a local Python 3.12 virtual environment:
    1. ``cd`` into the ``opensim-creator`` source directory (if you haven't already)
    2. Create a virtual environment (e.g. ``python3.12 -m venv .venv``) and activate it (``source .venv/bin/activate``)
    3. Install all necessary Python libraries into the virtual environment (e.g. ``pip install -r reuiqrements/all_requirements.txt``)

Ubuntu Build
^^^^^^^^^^^^

1. Build OpenSim Creator from source:
    1. ``cd`` into the ``opensim-creator`` source directory (if you haven't already)
    2. If necessary, set the ``CC`` and ``CXX`` environment variables to choose your desired
       C/C++ compilers (e.g. ``export CC=gcc-15 CXX=g++-15``).
    3. Build OpenSim Creator's dependencies: ``cd third_party/ && cmake --workflow --preset Development && cd -``
    4. Build OpenSim Creator: ``cmake --workflow --preset Development``
