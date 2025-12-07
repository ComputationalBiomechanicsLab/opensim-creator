.. _Setup Development Environment:


Development Environment
=======================

These are some generic tips that might be handy when setting up your own development environment.


Visual Studio 2022
------------------

- Open a terminal/powershell window
- Run ``git clone https://github.com/ComputationalBiomechanicsLab/opensim-creator`` to get a copy of OpenSim Creator's sourcecode
- ``cd opensim-creator``
- Run ``./scripts/setup_deps_windows-vs.bat RelWithDebInfo`` to get a complete build of
  OSC's dependencies.
- In Visual Studio 2020, open ``opensim-creator`` as a folder project
- Later versions of Visual Studio (i.e. 2017+) should have in-built CMake support
  that automatically detects that the folder is a CMake project
- Right-click the ``CMakeLists.txt`` file to edit settings or build the project
    - You may need to set your configure command arguments to point to the dependencies
      install (e.g. ``-DCMAKE_PREFIX_PATH=$(projectDir)/third_party-install-Debug``)
- Use the ``Switch between solutions and available views`` button in the
  ``Solution Explorer`` hierarchy tab to switch to the ``CMake Targets View``
- Right-click the ``osc`` CMake target and ``Set As Startup Project``, so that
  pressing ``F5`` will then build+run ``osc.exe``
- (optional): switch the solution explorer view to a ``Folder View`` after doing
  this: the CMake view is crap for developing osc
- You should now be able to build+run ``osc`` from ``Visual Studio``
- To run tests, open the ``Test Explorer`` tab, which should list all of the
  ``googletest`` tests in the project

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

QtCreator
---------

- Run the appropriate (OS-dependent) buildscript (described above)
- Open QtCreator and then open the ``opensim-creator`` source directory as a folder
- For selecting a "kit", QtCreator *usually* detects that ``build/`` already
  exists (side-effect of running the buildscript). You *may* need to "import existing
  kit/build" and then select ``build/``, though
- Once QtCreator knows your source dir (``opensim-creator/``) and build/kit
  (``opensim-creator/build/``), it should be good to go
