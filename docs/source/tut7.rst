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

.. warning::

    **The Preview Experimental Data UI is 🪄 experimental 🪄.**.

    We anticipate that this section will change over the next few releases as
    we work out the kinks, redesign parts of the workflow, etc. That is to say,
    what's described here does work, but isn't as slick as we'd like 😎.

In this section, we will be using experimental data from the `OpenSim Models Repository`_.
Specifically, the ``Gait10dof18musc``'s ``OutputReference`` data (`Gait10dof18musc Model Direct Link`_). You
will need to download the data in order to follow along with this walkthrough.

This walkthrough makes more sense if you engage in a little role-play 🧝🧙‍♂️. Imagine
that you've already recorded some experimental data and have made/acquired ``subject01.osim``.
You are now at the stage in your project where problems like loading/validating
raw data and linking it to the OpenSim model are bottlenecks. Concretely, you
want to double-check that your marker data matches what OpenSim expects, and
that the Inverse Kinematics (IK) result from OpenSim looks like it matches those
markers. You then want to validate that the ground reaction forces you measured
in the lab are correct and that they're synchronized with the marker data. Finally,
you want to apply those ground reaction forces to the OpenSim model and validate
that they are applied in the right location.

Load Raw Marker Data
^^^^^^^^^^^^^^^^^^^^

.. _walkthrough_markers-loaded:
.. figure:: _static/tut7_walkthrough-after-marker-data-loaded.png
    :width: 60%

    The preview experimental data UI after loading ``subject01_walk.trc``. The
    UI shows the marker locations as blue spheres. The time range for scrubbing
    can be adjusted using the min/max boxes either side of the scrubbing slider.

The first step is to load the raw marker data (``.trc``) file into the UI. That's
done by clicking the ``load raw data file`` button in the toolbar, followed by
selecting the ``subject01_walk.trc`` file (full path: ``Pipelines\Gait10dof18musc\OutputReference\ExperimentalData\subject01_walk.trc``). It
should look something like :numref:`walkthrough_markers-loaded`.

Load IK Result
^^^^^^^^^^^^^^

``TODO``

Load Raw Ground Reaction Forces
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``TODO``

Associate Ground Reaction Forces With Model (``ExternalLoads``)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

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
