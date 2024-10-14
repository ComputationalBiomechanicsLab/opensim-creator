.. _tut7:


🪄 Preview Experimental Data
=============================

.. warning::

    **Previewing Experimental Data is 🪄 experimental 🪄.**.

    We invite users to try this UI, and believe--even though it's still
    in-development--that it provides valuable tooling for working with
    OpenSim's various experimental data sources.

    The "Preview Experimental Data" workflow is currently capable of loading
    typical (i.e. ``sto`` / ``mot`` / ``trc``) files that are used in the
    wider OpenSim ecosystem. It can also load a model and its associated
    trajectory, and associate an ``ExternalLoads`` for visualization. The main
    reason that it's experimental is because it's missing some nice-to-haves,
    rather than because it's fundamentally broken.

In this tutorial, we will be using the preview experimental data workflow to
visualize, validate, and connect experimental data to an OpenSim model. This
is typically required when you have external experimental data that you want
to use with an OpenSim solver.

.. figure:: _static/tut7_preview-experimental-data-ui.jpeg
    :width: 60%

    A screenshot of the preview experimental data UI. Here, the ``subject01.osim``
    model file from the `OpenSim Models Repository`_ is loaded, which shows
    the model file; additionally, the ``subject01_walk.trc`` (markers, blue spheres) and
    ``subject01_walk_grf.mot`` (ground-reaction forces, orange arrows) are
    overlaid as raw data files. The model's trajectory (``subject01_walk_IK.mot``),
    as calculated from OpenSim's inverse kinematic (IK) solver, was loaded against
    the model, showing it's computed motion. This is useful for verifying that
    the various experimental data files, and solver results, make sense when combined.


Prerequisites
-------------

* **This is a standalone tutorial**. The preview experimental data workflow is
  a standalone UI for handling experimental data. You don't necessarily need
  to know how to build or handle an OpenSim model in order to use it
  effectively, but those skills are useful for debugging a model-specific issue.

* **For your own work**, you will need to have your experimental data files in
  a format that's compatible with OpenSim (e.g. ``trc``, ``sto``). Typically, this
  requires either using an OpenSim-aware data exporter, or writing a script
  that converts your experimental data from your source format into an
  OpenSim-compatible format. The data formats used by OpenSim are typically
  simple plaintext formats, which means that you can generally (e.g.) use a
  one-off python script to generate them. We recommend looking at some existing
  OpenSim-compatible data files (e.g. this `Example MOT File`_
  from the `OpenSim Models Repository`_) for inspiration.


Topics Covered by this Tutorial
-------------------------------

- A high-level overview of the preview experimental data workflow UI.
- A concrete walkthrough of using the preview experimental data UI in
  a typical experimental workflow.


Opening the Preview Experimental Data UI
----------------------------------------

The preview experimental data UI is an independent "workflow" UI that can be
accessed from OpenSim Creator's splash screen, in the ``Workflows`` section:

.. figure:: _static/tut7_preview-experimental-data-from-splash.jpeg
    :width: 60%

    How to open the preview experimental data UI from the splash screen. It's
    also accessible from the ``File`` menu.


Preview Experimental Data UI Overview
-------------------------------------

.. figure:: _static/tut7_preview-experimental-data-ui.jpeg
    :width: 60%

    The preview experimental data UI. In its current (🪄 experimental) iteration,
    it has buttons for loading a model, the model's associated trajectory, raw
    data files (unassociated to the model), and OpenSim XML files (e.g. ``ExternalLoads``).

The preview experimental data UI provides similar panels to the model editor
UI (e.g. ``Coordinates``, ``Navigator``), but with some key differences that
are tailored towards visualizing and debugging experimental data:

- **It can load a model trajectory**. The ``load model trajectory`` button in the top
  toolbar lets you load a trajectory (e.g. ``sto``) against the current. This
  is useful for (e.g.) debugging whether the output from OpenSim's Inverse
  Kinematics (IK) solver matches your experimental data.

- **It can load raw experimental data files**. The ``load raw data file`` button
  in the top toolbar lets you load raw data files into the scene. The data series
  in the raw data file can then be clicked, inspected, scrubbed, etc. to
  visualize how OpenSim understands them. Because they are "raw" data files
  they are unconnected to any frame in the model and always display in ground.

- **It can load associated OpenSim XML files**. The ``load OpenSim XML`` button
  in the top toolbar reads any ``<OpenSimDocument>`` and puts it in the model's
  ``componentset``, which associates it with the model. This is useful for (e.g.)
  associating an ``ExternalLoads`` to a model which, when force-vector visualization
  is enabled in a 3D viewer, lets you view when/where those forces are applied to
  the model.

- **It has a time scrubber**. There's a time scrubber (slider) in the top toolbar,
  which lets you set/modify the currently-viewed time. This doesn't involve any
  kind of solver or simulation (e.g. forward-dynamics). It only sets the current
  model's time, so that (e.g.) any associated motions, raw data, or ``ExternalLoads``
  reflect their impact at that point in time. This is useful for ensuring data
  behaves as-expected over time.

- **It can reload all of the above with a single click**. The ``reload all``
  button in the top toolbar is designed to reload everything in one click and
  scrub to the currently-scrubbed-to time. This is useful for debugging/fixing/editing
  the external files in an external editor, followed by reloading.

In combination, these features let you set up a single workplace where you can
work on/with experimental data to solve your research problems. The next section
describes, concretely, how they interplay in an example workflow.


Walkthrough: Markers to Motion
------------------------------

In this section, we will go through a typical workflow that uses the preview
experimental data UI.

This walkthrough makes more sense if we use a little bit of 🧙‍♂️ **roleplay** to
explain each step's context. Imagine that you've already recorded some experimental
data and have made/acquired ``subject01.osim``. You are now at the stage in your
project where problems like loading/validating raw data and linking it to the
OpenSim model are bottlenecks.


Download Raw Data and Models
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note::

  🧙‍♂️ **Roleplay**: you painstakingly collected this data from the lab, probably.

In this section, we will be using experimental data from the `OpenSim Models Repository`_.
Specifically, the ``Gait10dof18musc``'s ``OutputReference`` data (`Gait10dof18musc Model Direct Link`_).

1. **Download the data**: go to the ``TODO JUST PACKAGE IT UP BECAUSE IT'S A PITA
   FOR USERS TO HAVE TO FIGURE OUT GITHUB``
2. **Unzip it somewhere**: todo


.. _Load Raw Marker Data:
Load Raw Marker Data
^^^^^^^^^^^^^^^^^^^^

.. note::

  🧙‍♂️ **Roleplay**: you want to check the marker data before using it with
  OpenSim's IK solver. Does it have reasonable locations, reasonable motion,
  and correct labels?

The first step is to load the raw marker data (``.trc``) file into the UI. To do
that, you will need to:

1. Click the ``load raw data file`` button in the toolbar
2. Select the ``subject01_walk.trc`` file (full path: ``Pipelines\Gait10dof18musc\OutputReference\ExperimentalData\subject01_walk.trc``).

Once loaded, it should look something like this:

.. figure:: _static/tut7_walkthrough-after-marker-data-loaded.png
    :width: 60%

    The preview experimental data UI after loading ``subject01_walk.trc``. The
    UI shows the marker locations as blue spheres. The time range for scrubbing
    can be adjusted using the min/max boxes either side of the scrubbing slider.

With the markers loaded, you can now:

- **Inspect whether they match your expectations**. The top toolbar lets you scrub
  to different times in raw data, which helps with visualizing the motion of the
  model.
- **Double-check marker labels**. You can hover/click the markers to see their
  name, or view the names in the ``Navigator`` panel. This can be handy for
  double-checking that the marker data was labelled correctly. The OpenSim IK
  solver generally assumes that the data labels match the names of ``OpenSim::Marker`` s
  in your model.
- **Edit the .trc file**. If there's any problems, you can edit the underlying
  ``.trc`` file using a text editor (e.g. Visual Studio Code, Notepad++) and then
  click ``Reload All`` in the Preview Experimental Data UI to reload the data. This
  is useful for tweaking labels, reversing axes, etc.


Load IK Result
^^^^^^^^^^^^^^

.. note::

  🧙‍♂️ **Roleplay**: you were satisfied with the marker data and used it with OpenSim's
  IK solver in a process described `here <https://opensimconfluence.atlassian.net/wiki/spaces/OpenSim/pages/53089741/Tutorial+3+-+Scaling+Inverse+Kinematics+and+Inverse+Dynamics>`_.
  You now want to inspect the IK result.

After confirming that the marker data looks reasonable, you can then use it with
your model and OpenSim's IK solver to yield a trajectory. You can overlay the
model + trajectory in the preview experimental data UI with the following steps:

1. **Load the model**: Click ``load model`` and choose ``Pipelines\Gait10dof18musc\OutputReference\subject01.osim``,
   which, in this example, was the model that was used with the IK solver.

2. **Load the trajectory**: Click ``load trajectory/states`` and choose ``Pipelines\Gait10dof18musc\OutputReference\IK\subject01_walk_IK.mot``,
   which, in this example, is the result from OpenSim's IK solver.

Once loaded, you should be able to see the raw marker data, your model, and its
motion all overlaid:

.. figure:: _static/tut7_walkthrough-after-IK.png
    :width: 60%

    The preview experimental data UI after loading the raw marker data, the model,
    and the IK result. This is useful for visually inspecting how closely the model
    trajectory from IK matches the input marker locations.


Load Raw Ground Reaction Forces
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note::

  🧙‍♂️ **Roleplay**: your experiment also recorded ground reaction forces (GRFs) on
  a force plate. You want to make sure that the GRFs are synchronized with the rest
  of the data and point in the right direction.


This step is identical to `Load Raw Marker Data`_ :

1. Click the ``load raw data file`` button in the toolbar
2. Select the GRF data, ``subject01_walk_grf.mot`` (full path: ``Pipelines\Gait10dof18musc\OutputReference\ExperimentalData\subject01_walk_grf.mot``).

Once loaded, you should be able to see the marker data, your model, the model's motion,
and your GRF vectors overlaid:

.. figure:: _static/tut7_walkthrough-after-grfs-loaded.png
    :width: 60%

    The preview experimental data UI after loading the marker data, model, IK
    trajectory, and GRFs. The UI shows the GRFs are orange arrows.

Similarly to `Load Raw Marker Data`_, you can edit the underlying GRF file to
fix any issues, such as vectors pointing in the wrong direction or invalid
column headers.


Associate Ground Reaction Forces With Model (``ExternalLoads``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note::

  🧙‍♂️ **Roleplay**: you want to use the GRFs with OpenSim's Inverse Dynamics
  (ID) solver. However, OpenSim requires either an ``ExternalLoads`` XML file
  to link the GRFs to the model.

This is the final---and most error-prone---part of the process.

``TODO``


Suggested Further Steps
^^^^^^^^^^^^^^^^^^^^^^^

``TODO`` Inverse Dynamics, CMC, etc.


Summary
-------

In this tutorial, we covered (typical use-cases of) the preview experimental data
UI. This is useful when trying to connect experimental data to OpenSim models. We
hope to add more functionality to the UI over time.

----

More generally, this tutorial also outlines a general philosophy for handling
experimental data. This is because it's challenging. The general philosophies
we are trying to encourage are:

- **Work Incrementally**: handle each data file, or configuration file,
  one-at-a-time. Handle any errors as you go along. Otherwise, debugging
  will be much more complicated.

- **Don't Fly Blind**: always aim to have some kind of visual feedback when
  going through each step. Confirm that the something's there *and* that it
  looks reasonable.

- **Be Deliberate**: Don't just (e.g.) copy and paste an ``ExternalLoads`` file
  from the internet, or use a wizard, because it's required by a solver in the
  OpenSim GUI. Figure out *why* it's necessary and *what* it's doing. Read
  through the file - they don't bite, much 🧛‍♀️.

If you follow those steps, we believe you'll find it easier to integrate
experimental data with OpenSim models. 


(Optional) Extra Exercises
--------------------------

- **Play with previous models that have experimental data**. The `OpenSim Models Repository`_ contains
  a collection of OpenSim models and examples of how those models were used with
  experimental data (in ``Pipelines/``). It's an excellent source for seeing how
  previous researchers have combined OpenSim with experimental data to do something
  useful. One of the pipelines from that repository ``Gait10dof18musc`` was used
  to write the walkthrough section of this tutorial. `SimTK.org`_ is also a good
  source for published OpenSim models.

.. _OpenSim Models Repository: https://github.com/opensim-org/opensim-models
.. _Gait10dof18musc Model Direct Link: https://github.com/opensim-org/opensim-models/tree/c62c24b0da1f89178335cf10f646a39c90d15580/Pipelines/Gait10dof18musc/OutputReference
.. _Example MOT File: https://github.com/opensim-org/opensim-models/blob/master/Pipelines/Gait10dof18musc/ExperimentalData/subject01_walk_grf.mot
.. _SimTK.org: https://simtk.org/
.. _OpenSim IK Tutorial: https://opensimconfluence.atlassian.net/wiki/spaces/OpenSim/pages/53089741/Tutorial+3+-+Scaling+Inverse+Kinematics+and+Inverse+Dynamics
