.. _installation:

Installation
============

The easiest way to install OpenSim Creator is to download a prebuilt installer
from the `GitHub Releases Page`_.

OpenSim Creator is also regularly built from source using GitHub Actions, so if
you want a bleeding-edge---but unreleased---build of OpenSim Creator, check
out the `GitHub Actions Page`_ (**note**: you must be logged into GitHub to see
artifact download links).


Installing on Windows (10 or newer)
-----------------------------------

- Download an installer ``exe`` from the `GitHub Releases Page`_
- Run the ``.exe``, continue past any security warnings
- Follow the familiar ``next``, ``next``, ``finish`` installation wizard
- Run OpenSim Creator by typing ``OpenSim Creator`` in your start menu, or browse to
  its installation location (defaults to ``C:\Program Files\OpenSimCreator\``).


Installing on MacOS (Ventura or newer)
--------------------------------------

- Download a ``dmg``-based installer from the `GitHub Releases Page`_
- Double click the ``dmg`` file to mount it
- Drag the ``osc`` icon into your ``Applications`` directory
- Browse to the ``Applications`` directory in the MacOS ``Finder`` directory
- Right-click the ``osc`` application, click ``open``, continue past any
  security warnings to run ``osc`` for the first time
- After running it the first time, you can boot it as normal (e.g. ``Command+Space``,
  ``osc``, ``Enter``)

.. note::
    You may need to right-click the ``osc`` application to open it twice.

    The reason why is because MacOS _really_ doesn't like running unsigned
    binaries, but the OpenSim Creator project is too cheap to join the Apple
    developer program.


Installing on Ubuntu (20 or newer)
----------------------------------

- Download a ``deb``-based installer from the `GitHub Releases Page`_
- Double-click the ``.deb`` package and install it through your package manager UI.
- **Alternatively**, you can install the ``deb`` file through the command line with:

    apt-get install -yf ~/Downloads/osc-X.X.X_amd64.deb  #  or similar

- Once installed, the ``osc`` or ``OpenSim Creator`` shortcuts should be available
  from your desktop, or you can browse to the installation location (``/opt/osc``)


Building from Source
--------------------

See :ref:`buildingfromsource` for an explanation of how to do this.

.. _GitHub Releases Page: https://github.com/ComputationalBiomechanicsLab/opensim-creator/releases
.. _GitHub Actions Page: https://github.com/ComputationalBiomechanicsLab/opensim-creator/actions