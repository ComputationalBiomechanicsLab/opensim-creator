.. _installation:

Installation
============

The easiest way to install OpenSim Creator is to download a prebuilt installer
from the `GitHub Releases Page`_.


Installing on Windows (10 or newer)
-----------------------------------

- Download an installer ``exe`` from the `GitHub Releases Page`_. **Note**: As of mid-2025,
  Microsoft Edge may now prevent you from running the installer. This is because OpenSim
  Creator hasn't arranged binary signing yet (sorry). The solution to this issue is to download
  OpenSim Creator via another browser (e.g. Firefox, Google Chrome).
- Run the ``.exe``, continue past any security warnings
- Follow the familiar ``next``, ``next``, ``finish`` installation wizard
- Run OpenSim Creator by typing ``OpenSim Creator`` in your start menu, or browse to
  its installation location (defaults to ``C:\Program Files\OpenSimCreator\``).


Installing on MacOS (Sonoma 14.5 or newer)
------------------------------------------

- Download a ``dmg``-based installer from the `GitHub Releases Page`_
- Double click the ``dmg`` file to mount it
- Drag the ``OpenSim Creator`` icon into your ``Applications`` directory
- Run ``OpenSim Creator`` from your applications folder

.. warning::

  **>= v0.6.0**: OpenSim Creator now codesigns and notarizes its binaries, so
  installation should be as easy as dragging and dropping the application.

  **< 0.6.0**, OpenSim Creator's build process did not sign (notarize) its
  binaries, because it requires organizing and paying for an Apple Developer
  account. Unsigned binaries can require additional installation steps:

  **On Sequoia**: open a terminal and run ``xattr -cr /path/to/opensimcreator.dmg`` to
  clear any quarantine flags that MacOS added when the dmg was downloaded. Mount the
  dmg (e.g. by double-clicking it), drag and drop ``OpenSim Creator`` into the ``Applications``
  directory. Try to run the application. MacOS will fail to open it this first
  time. Open the computer's system settings, go to ``Privacy & Security``, scroll
  down, there should be something like ``OpenSim Creator was blocked to protect your
  mac``, click ``Open Anyway``. After doing this the first time, subsequent runs
  shouldn't require these steps.

  **On MacOSes older than Sequoia**: Mount the dmg (e.g. by double-clicking it), drag
  and drop ``OpenSim Creator`` to the ``Applications`` directory. Open the ``Applications`` directory
  and right-click ``OpenSim Creator``, click ``open``. MacOS will ask you whether you're sure
  you want to open it (you are). After doing this the first time, subsequent runs
  shouldn't require these steps.


Installing on Ubuntu (22.04 or newer)
-------------------------------------

- Download a ``deb``-based installer from the `GitHub Releases Page`_
- Double-click the ``.deb`` package and install it through your package manager UI.
- **Alternatively**, you can install the ``deb`` file through the command line with:

    apt-get install -yf ~/Downloads/opensimcreator-X.X.X_amd64.deb  #  or similar

- Once installed, the ``osc`` or ``OpenSim Creator`` shortcuts should be available
  from your desktop, or you can browse to the installation location (``/opt/osc``)


Building from Source
--------------------

See :ref:`buildingfromsource` for an explanation of how to do this.

.. _GitHub Releases Page: https://github.com/ComputationalBiomechanicsLab/opensim-creator/releases
.. _Apple Gatekeeper Documentation: https://support.apple.com/en-us/102445
