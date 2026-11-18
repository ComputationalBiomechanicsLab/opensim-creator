TASK: Write a "building from source" guide for the OPynSim project. Follow these instructions:

- The writing style should match OPynSim's or OpenSim Creator's writing style (technical, but not
  overly dry, and with humanizing notes of sarcasm/funniness occasionally).
- The target audience for the guide is junior C++ software developers and engineers with some basic
  knowledge of Python. The guide shouldn't be dumbed down too much for the latter demograph, though, it's
  up to them to figure out the basics of using a command line.
- The guide should focus on very concrete steps. Run this, install that, consider these customization points,
  and so on.
- The markup must be as Sphinx restructured text.

I will list the required sections below, with bullet points for suggested content.


Building From Source
====================

- Write an overview of how the OPynSim project source build is structured. Mainly, that the caller must first
  prepare their environment (CMake, C++ compiler, Python environment), then they build the `third_party` vendored
  C++ dependencies, then they build the main project. The final step (main project build) may be done directly
  in an IDE, such as CLion or Visual Studio, which have in-built CMake support.
- The build requires setting up a suitable Python environment. For development, we use Python 3.12 with dependencies
  installed from `requirements/all_requirements.txt`.
- The native (C++) builds uniformly use CMake presets and workflows. Three presets are available: `Development`, `ErrorCheck`,
  and `Release-$OS-$ARCH`. Developers should initially use `Development`, people building custom releases should use
  `Release-$OS-$ARCH`.

Development Environment Setup
-----------------------------

- Describe that it's necessary to setup a development environment (toolchain) in order to build OPynSim. This is a
  normal step for developing a project.
- Different developers use different tools. This section should try to cover these combinations:
  - Windows, macOS, Ubuntu/Linux
- Development environment setup happens _before_ configuring/building the OPynSim project (next section), so it
  doesn't need to strongly refer to a particular IDE or anything like that.

Windows
~~~~~~~

On Windows, most users are familiar with installing applications one-by-one with installers, so the guide
should follow that.

- Install ``git``
- Install Visual Studio
- Install Python and pip from Python.org
- Clone source code repository
- Create Python3.12 virtual environment
- Build vendored third-party dependencies

macOS
~~~~~

On macOS, most users are familiar with installing applications one-by-one with dmg installers, or (ocassionally)
running a couple of commands from the terminal, so the guide should follow that.

- Install git
- Install XCode
- Install cmake
- Install Python3.12
- Clone source code repository
- Create Python virtual environment
- Build+install vendored third-party dependencies

Ubuntu
~~~~~~

On Ubuntu, users are usually ok with running things from the command-line, so the guide is permitted to just
say "run `apt-get install X Y Z`".

- Install git
- Install g++
- Install cmake
- Install Python3.12
- Clone source code repository
- Create Python virtual environment
- Build+install vendored third-party dependencies

Build/Run Project
-----------------

With a development environment setup and third-party vendored libraries built+installed, you should then be able
to configure and build OPynSim in your development environment of choice (e.g. Visual Studio, CLion, Neovim), or
via the command-line.

Windows (Visual Studio)
~~~~~~~~~~~~~~~~~~~~~~~

TODO

Windows (cmd.exe)
~~~~~~~~~~~~~~~~~

TODO

CLion (all OSes)
~~~~~~~~~~~~~~~~

TODO

Bash (Linux/macOS)
~~~~~~~~~~~~~~~~~~

TODO

Neovim (Linux/macOS)
~~~~~~~~~~~~~~~~~~~~

TODO
