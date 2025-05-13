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


Installing on MacOS (Sonoma 14.5 or newer)
------------------------------------------

- Download a ``dmg``-based installer from the `GitHub Releases Page`_
- Double click the ``dmg`` file to mount it
- Drag the ``osc`` icon into your ``Applications`` directory
- Run ``osc`` from your applications folder

.. warning::

  OpenSim Creator's build process does not sign (notarize) its binaries, because
  we'd have to organize and pay a subscription for that service.

  For you, this means that OpenSim Creator requires extra steps that depend on
  your version of MacOS (Apple changes the procedure regularly - $$$).

  **On Sequoia**: open a terminal and run ``xattr -cr /path/to/opensimcreator.dmg`` to
  clear any quarantine flags that MacOS added when the dmg was downloaded. Mount the
  dmg (e.g. by double-clicking it), drag and drop ``osc`` into the ``Applications``
  directory. Try to run the application. MacOS will fail to open it this first
  time. Open the computer's system settings, go to ``Privacy & Security``, scroll
  down, there should be something like ``osc was blocked to protect your
  mac``, click ``Open Anyway``. After doing this the first time, subsequent runs
  shouldn't require these steps.

  **On MacOSes older than Sequoia**: Mount the dmg (e.g. by double-clicking it), drag
  and drop ``osc`` to the ``Applications`` directory. Open the ``Applications`` directory
  and right-click ``osc``, click ``open``. MacOS will ask you whether you're sure
  you want to open it (you are). After doing this the first time, subsequent runs
  shouldn't require these steps.


Installing on Ubuntu (22.04 or newer)
-------------------------------------

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
