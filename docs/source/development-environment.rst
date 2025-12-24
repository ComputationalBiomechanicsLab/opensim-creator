.. _Setup Development Environment:


Development Environment
=======================

These are some generic tips that might be handy when setting up your own development environment.


Visual Studio 2022/2026
-----------------------

- Visual Studio installs typically bundle almost everything you need to build OpenSim
  Creator (excl. Python).
- The OpenSim Creator build uses ``CMakePresets.json`` files for both the main build
  (in the repository root) and the dependencies build (in ``third_party/``). Visual
  Studio natively understands how to load folders containing these files, and should
  automatically present each preset as a user-selectable configuration (e.g. ``Development``
  and ``Release`` will be listed as options).

- If you want to build OpenSim Creator via the Visual Studio UI, then you should:

  - Clone the OpenSim Creator GitHub repository (``https://github.com/ComputationalBiomechanicsLab/opensim-creator``)
    via the Visual Studio UI. Close it after cloning (after cloning, you need to open ``third_party`` because
    the main build depends on things installed by the ``third_party`` build).
  - Open ``third_party/`` as a folder in VS, which should configure the dependencies build
    with a preset from ``CMakePresets.json`` (you can change which via a dropdown at the top
    of the VS UI).
  - Build the third-party dependencies, which should install the dependencies in a
    preset location within the OpenSim Creator build directory
  - Open the top-level ``opensim-creator`` folder in VS. CMake should configure it 
    without any errors. **Note**: you need to use the same configuration for both
    the dependencies and main build.

- If you want to build OpenSim Creator via the terminal, then you should:

  - Open a terminal (``cmd.exe``) window
  - Run ``git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator`` to get a copy of OpenSim Creator's sourcecode
  - ``cd opensim-creator``
  - Activate the Visual Studio development environment by running ``call scripts\env_vs-x64.bat``. This
    gives the terminal access to tools like ``cmake``, ``ninja``, ``cl``, etc.
  - ``cd third_party && cmake --workflow --preset Development && cd ..``. This builds
    the third-party dependencies with the ``Development`` preset. See ``CMakePresets.json``
    for a list of available presets, and beware that the preset must match the main build's
    preset.
  - ``cmake --workflow --preset Development`` in the ``opensim-creator`` directory, which
    will build OpenSim Creator
  - After this, you have a complete build. You can (e.g.) open the directory in Visual Studio
    and start developing.

- When working in the Visual Studio UI:

  - Use the ``Switch between solutions and available views`` button in the
    ``Solution Explorer`` hierarchy tab to switch to the ``CMake Targets View``
  - Right-click the ``osc`` CMake target and ``Set As Startup Project``, so that
    pressing ``F5`` will then build+run ``osc.exe``
  - (optional): switch the solution explorer view to a ``Folder View`` after doing
    this: the CMake view is crap for developing OpenSim Creator
  - You should now be able to build+run ``osc`` from ``Visual Studio``
  - To run tests, open the ``Test Explorer`` tab, which should list all of the
    ``googletest`` tests in the project.


Note: Debugging on Visual Studio
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``googletest`` catches and reports Windows SEH exceptions. This is handy when (e.g.)
running the test suites from a commandline, where you *probably* just want a report
saying which ones have failed, but it is problematic if you specifically are running
the test suite to debug a runtime error that the test is exercising.

To disable this behavior, you have to provide ``--gtest_catch_exceptions=0`` to the
test suite executable. In Visual Studio you can do this by:

- Setting the executable as your startup (``F5`` et. al.) project
- Then go to ``Debug > Debug and Launch Settings for testopensimcreator.exe`` (for example)
- And then add an `args` element to the `configurations` list

Example:

.. code-block::
    :caption: config.json

    {
      "version": "0.2.1",
      "defaults": {},
      "configurations": [
        {
          "type": "default",
          "project": "CMakeLists.txt",
          "projectTarget": "testopensimcreator.exe (tests\\testopensimcreator\\testopensimcreator.exe)",
          "name": "testopensimcreator.exe (tests\\testopensimcreator\\testopensimcreator.exe)",
          "args": [
            "--gtest_catch_exceptions=0"
          ]
        }
      ]
    }
