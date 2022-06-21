OpenSim Creator Documentation
=============================

OpenSim Creator is a UI for building OpenSim models. It is a standalone UI that
is designed as a proof-of-concept, with the intent that some of its features may
be merged into the official `OpenSim GUI`_. These pages are the official documentation
for OpenSim Creator.

.. figure:: _static/screenshot.png
    :width: 60%

    Screenshot of OpenSim Creator's ``osim`` editor screen.

.. warning::
    These documentation pages are work-in-progress 🚧. If they aren't helpful
    enough then feel free to open a ticket on OpenSim Creator's `GitHub`_ page.
    This documentation is kept in the project repository `here <https://github.com/ComputationalBiomechanicsLab/opensim-creator/tree/main/docs>`_ so, if you know
    what you're doing, you can also try to submit a PR.


Getting Started
---------------

* **Get an OpenSim Creator release.** Grab it from the  `📥 releases <https://github.com/ComputationalBiomechanicsLab/opensim-creator/releases>`_ page on GitHub. Make sure you download a release that is suitable for your machine. Install it according to the installer's instructions.

* **Boot OpenSim Creator**:

  * **On Windows**: Search for "osc" in the start menu and open that. Alternatively, browse to your install location (default: ``C:\Program Files\osc\bin\osc.exe``).
  * **On Mac**: Search for "osc" in the finder (Super+Space, search, enter). Alternatively, browse to your install location (default: ``/Applications/osc.app``).
  * **On Linux**: Search for "osc" in your desktop. Alternatively, browse to your install location (default: ``/opt/osc (VERSION)/bin/osc``).

* **Explore the UI**. You'll probably want to initially explore and experiment with the UI by loading some of the example files, which are shown on OpenSim Creator's splash screen. I'd recommend ``double_pendulum.osim`` if you want something basic, or ``ToyDropLanding.osim`` if you want something "meatier".

* **Work through the tutorials**. Once you feel comfortable with the basics (opening/creating files, moving around the UI), then you are ready to start 📖 :ref:`tut1`.


Table of Contents
-----------------

.. toctree::
   :maxdepth: 2
   :caption: Tutorials
   :numbered:

   tut1
   tut2
   tut3
   tut4

.. toctree::
    :maxdepth: 2
    :caption: Challenges

    chal1

.. toctree::
    :maxdepth: 2
    :caption: Documentation Pages

.. toctree::
    :caption: Other Links

    OpenSim Creator GitHub <https://github.com/ComputationalBiomechanicsLab/opensim-creator>
    OpenSim GitHub <https://github.com/opensim-org/opensim-core>
    OpenSim Documentation <https://simtk-confluence.stanford.edu/display/OpenSim/Documentation>


.. _Indices and Tables:

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`


.. _OpenSim GUI: https://github.com/opensim-org/opensim-gui
.. _GitHub: https://github.com/ComputationalBiomechanicsLab/opensim-creator
