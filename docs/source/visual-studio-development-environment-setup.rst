Visual Studio Development Environment Setup
===========================================

.. note::

    This guide was written against Visual Studio 2026


TODO: prerequisite check: ensure Python is installed and on the PATH
WITHOUT A VIRTUAL ENVIRONMENT ALREADY ACTIVATED (IT'LL SCREW YOU LATER).
Even if you're in ``(base)`` you're in a virtual environment and it's game over
you __must__ ``deactivate`` the environment AND THEN make the local venv.


Download and Install Visual Studio
----------------------------------

.. figure:: _static/visual-studio-development-environment-setup/1_download-link.jpg
    :width: 80%

    Visual Studio 2026 can be downloaded from the `Visual Studio Downloads`_ page. The
    free Community Edition is fine for developing ``opynsim``.

.. figure:: _static/visual-studio-development-environment-setup/2_enable-python-development.jpg
    :width: 80%

    When installing Visual Studio, you will be greeted with an options screen where you
    select which workloads you are developing. For the best experience when developing
    ``opynsim``, select "Python Development" (pictured) and "Desktop development with C++"
    (next figure).

.. figure:: _static/visual-studio-development-environment-setup/3_enable-cpp-development.jpg
    :width: 80%

    The "Desktop development with C++" option should be selected in addition to the Python
    development option (previous step). These options provide the necessary tools and compilers
    to build ``opynsim`` with the least hassle.

.. figure:: _static/visual-studio-development-environment-setup/4_install.jpg
    :width: 80%

    Once you have selected the desired workflows (at least "Python Development" and
    "Desktop development with C++"), install Visual Studio.


Using CLion on Windows?
~~~~~~~~~~~~~~~~~~~~~~~

.. image:: _static/visual-studio-development-environment-setup/Clion.svg
    :align: center

If you're planning on developing ``opynsim`` with CLion on Windows, you're done here 🎉! You
should now be able to select Visual Studio as a toolchain when opening ``opynsim`` in
CLion.


Open OPynSim in Visual Studio
-----------------------------

.. figure:: _static/visual-studio-development-environment-setup/5_clone-repo.jpg
    :width: 80%

    After Visual Studio is installed it will open its splash screen, which is where
    you select which project you will work on. ``opynsim`` is a cross-platform hybrid
    (C++/Python) CMake project, which Visual Studio can either open as a folder (if you
    already have it on your computer), or clone externally (pictured).

    This guide clones the ``opynsim`` repository via Visual Studio (pictured), just
    to keep everything contained within one application.

.. figure:: _static/visual-studio-development-environment-setup/6_enter-repo-details.jpg
    :width: 80%

    Fill out ``opynsim``'s details in the "Clone a Repository" prompt. The URL of ``opynsim``'s
    central repository is ``https://github.com/opynsim/opynsim``. Optionally, choose
    a path where it should be checked out to (I prefer my ``Desktop``, for example).

.. figure:: _static/visual-studio-development-environment-setup/7_switch-to-folder-view.jpg
    :width: 80%

    Once the repository is cloned, Visual Studio will then open and show the "What's New" tab
    (you can close this). Click the folder view (annotated) to open the ``opynsim`` project as
    a folder.


Setup Python Virtual Environment
--------------------------------

 TODO: be VERY VERY CAREFUL around here: some users might have miniforge/condaforge and
 therefore already be in a virtual environment. This won't work later on when they run ``opynsim_debugger.exe``
 because those virtual environments aren't on the PATH for Visual Studio.

This section sets up Visual Studio with the correct Python virtual environment for developing
``opynsim``, which contains the necessary python packages.

.. figure:: _static/visual-studio-development-environment-setup/8_open-terminal.jpg
    :width: 80%

    Open a terminal from the view menu.

.. figure:: _static/visual-studio-development-environment-setup/9_run-setup-venv.jpg
    :width: 80%

    Run virtual environment setup script by running ``python scripts/setup_venv.py`` from the
    terminal. This will create a local ``.venv`` directory in your ``opynsim`` source directory
    with the appropriate packages that ``opynsim`` uses.

.. figure:: _static/visual-studio-development-environment-setup/10_add-python-environment.jpg
    :width: 80%

    Tell Visual Studio to use the virtual environment. Click the ``Add Environment`` button
    (annotated) to begin configuring Visual Studio to use the environment.

    **Note**: If you can't see the Python toolbar, then open a python file (e.g. ``opynsim/opynsim/__init__.py``)
    in Visual Studio, which usually makes it show the bar.

    **Note #2**: you might not need to do this. If Visual Studio already lists ``.venv`` as an
    available environment, it has probably already auto-detected the environment. Confirm this
    with the next step (list the environments, ensure it's listing the correct location). If it
    is wrong, add a new environment with a different name (e.g. ``.venv (opynsim)``).

.. figure:: _static/visual-studio-development-environment-setup/11_select-local-venv.jpg
    :width: 80%

    Select "Existing Environment" and set prefix path to the ``.venv`` directory that was created
    in the previous step. This lets Visual Studio know which virtual environment it should use.

.. figure:: _static/visual-studio-development-environment-setup/12_validate-python-environment.jpg
    :width: 80%

    To confirm that the python virtual environment has been setup, click the "Open the Python Environments Window"
    and ensure that ``.venv`` is listed and connects to the local virtual environment you created in
    the previous steps. Ensure Visual Studio uses the environment by selecting it in the toolbar
    at the top of Visual Studio (annotated).


Setup Terminal Environment
--------------------------

This step sets up the terminal in Visual Studio to use the python virtual environment
and also have access to the C++ environment (compiler, cmake, etc.).


.. figure:: _static/visual-studio-development-environment-setup/13_open-options.jpg
    :width: 80%

    Open the Visual Studio options menu from the tools menu.

.. figure:: _static/visual-studio-development-environment-setup/14_add-development-terminal.jpg
    :width: 80%

    Search for terminal, click ``Add`` below the list of terminals.

.. figure:: _static/visual-studio-development-environment-setup/15_development-terminal-details.jpg
    :width: 80%

    Fill out the terminal details. ``opynsim`` includes a batch script (``scripts/env_vs-x64.bat``),
    which sets up a batch terminal (``cmd.exe``) with the necessary configuration (points to ``.venv``,
    uses Visual Studio's C++ compiler, etc.).

    The name can be anything (we use ``cmd.exe (C++, x64; Python, .venv)``). The shell location must be
    ``C:\WINDOWS\system32\cmd.exe``. The arguments must be ``/k ""scripts/env_vs-x64.bat""``.

.. figure:: _static/visual-studio-development-environment-setup/16_after-adding-and-defaulting-terminal.jpg
    :width: 80%

    After adding the terminal, set it as the default (annotated).


Build OPynSim's C++ Dependencies
--------------------------------

.. figure:: _static/visual-studio-development-environment-setup/17_activate-development-terminal.jpg
    :width: 80%

    Open the new terminal in your terminal window.

.. figure:: _static/visual-studio-development-environment-setup/18_build-dependencies.jpg
    :width: 80%

    Use the new terminal window to build ``opynsim``'s C++ dependencies. The command for doing this
    is ``cd third_party && cmake --workflow --preset Development && cd ..``.

.. figure:: _static/visual-studio-development-environment-setup/19_after-all-dependencies-built.jpg
    :width: 80%

    Once the third-party C++ dependencies are built, you should see something resembling a completion
    message. Now that they're built, you should now (finally) be able to configure and build ``opynsim``
    itself.


Build OPynSim
-------------

.. figure:: _static/visual-studio-development-environment-setup/20_run-development-workflow.jpg
    :width: 80%

    Now build ``opynsim``. We have provided a ``Development`` workflow preset that builds and tests
    ``opynsim``. It can be ran from the ``Project > Run CMake Workflow > Development`` menu option

.. figure:: _static/visual-studio-development-environment-setup/21_ran-development-workflow.jpg
    :width: 80%

    Once the workflow has ran, it should run all the unit tests and they shouuld all pass. Congratulations 🎉
    you are now building ``opynsim``.


Developing Python for OPynSim in Visual Studio
----------------------------------------------

If you want to run and debug python in the project, you need something to run. For this
demonstration, we're going to run ``debugscript.py``, because we know it's a runnable script
(rather than, say, a library), and we know that the C++ debugger can also connect to it
(later section). That said, you can run any python script in the project with this same
technique.

.. figure:: _static/visual-studio-development-environment-setup/22_run-python-debugger.jpg
    :width: 80%

    To run a script, open it (``debugscript.py`` is an example that's annotated on the right-hand side)
    and make sure that Visual Studio is configured such that the play button runs the current document
    (annotated).

    Running scripts this way automatically runs the python debugger, so you can also set breakpoints
    (pictured).

.. figure:: _static/visual-studio-development-environment-setup/23_python-debugging-example.jpg
    :width: 80%

    When the python debugger is paused, you will have access to the running environment. Visual
    Studio includes many debug panels that can be enabled/disabled and dragged around. Pictured
    here is the ``Locals`` panel (bottom-right), which lists local variables; ``Call Stack``, which
    shows the sequence of function calls that led to this point; and ``Immediate Window``, which lets you
    run Python in the paused process.


Developing C++ for OPynSim in Visual Studio
-------------------------------------------

If you want to run and debug C++ in the project, you can either attach to an existing process
(not described here, Google it), or run a native application (.exe) directly from Visual Studio.
``opynsim`` provides a ``opynsim_debugger.exe`` application that you can run within Visual Studio,
which runs the ``debugscript.py`` for you. The advantage of this is that you can write python and
C++ and use either debugger on the same code.

.. figure:: _static/visual-studio-development-environment-setup/24_run-native-debugger.jpg
    :width: 80%

    TODO: caption

.. figure:: _static/visual-studio-development-environment-setup/25_native-debugging-example.jpg
    :width: 80%

    TODO: caption

TODO: mention the "cant find python312.dll" issue, usually related to the user Using
a virtual environment.


.. _Visual Studio Downloads: https://visualstudio.microsoft.com/downloads/
