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
- Run ``osc`` from your applications folder

.. warning::

  OpenSim Creator's build process does not sign (notarize) its binaries, because
  we'd have to organize and pay a subscription for that service.

  For you, what this means is that OSC will appear as from **"untrusted developer"**
  when booting for the first time. MacOS's gatekeeper technology will then stop
  OSC from booting. As a user, you can skip past this security warning, but Apple
  has a habit of changing the skipping process every once in a while. Here is a
  list of workarounds:

  - **Check the official documentation**: Apple usually documents the process of
    running unsigned executables in its `Apple Gatekeeper Documentation`_
  - **On MacOS Sequoia (ish)**: Try to run OSC once. It will fail. Then open your
    computer's system settings, click ``Privacy & Security``, scroll down. You
    should see something like ``OSC was blocked to protect your mac``, click
    ``Open Anyway``
  - **On older MacOSes**: Open finder, go to ``Applications``, right-click the
    OSC/OpenSimCreator application, click open. It should ask you whether you
    want to open it anyway. Some newer(ish) MacOSes won't ask you until you
    right-click the application a second time.


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
.. _Apple Gatekeeper Documentation: https://support.apple.com/en-us/102445