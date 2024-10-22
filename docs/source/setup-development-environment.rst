.. _Setup Development Environment:


Setup Development Environment
=============================

These are some generic tips that might be handy when setting up your own development environment.


Visual Studio 2022
------------------

- Run ``python scripts/build_windows.py --skip-osc`` (described above) to get a
  complete build of OSC's dependencies.
- In Visual Studio 2020, open ``opensim-creator`` as a folder project
- Later versions of Visual Studio (i.e. 2017+) should have in-built CMake support
  that automatically detects that the folder is a CMake project
- Right-click the ``CMakeLists.txt`` file to edit settings or build the project
    - You may need to set your configure command arguments to point to the dependencies
      install (e.g. ``-DCMAKE_PREFIX_PATH=$(projectDir)/osc-dependencies-install``)
- Use the ``Switch between solutions and available views`` button in the
  ``Solution Explorer`` hierarchy tab to switch to the ``CMake Targets View``
- Right-click the ``osc`` CMake target and ``Set As Startup Project``, so that
  pressing ``F5`` will then build+run ``osc.exe``
- (optional): switch the solution explorer view to a ``Folder View`` after doing
  this: the CMake view is crap for developing osc
- You should now be able to build+run ``osc`` from ``Visual Studio``
- To run tests, open the ``Test Explorer`` tab, which should list all of the
  ``googletest`` tests in the project


QtCreator
---------

- Run the appropriate (OS-dependent) buildscript (described above)
- Open QtCreator and then open the ``opensim-creator`` source directory as a folder
- For selecting a "kit", QtCreator *usually* detects that ``osc-build`` already
  exists (side-effect of running the buildscript). You *may* need to "import existing
  kit/build" and then select ``osc-build``, though
- Once QtCreator knows your source dir (``opensim-creator/``) and build/kit
  (``opensim-creator/osc-build``), it should be good to go
