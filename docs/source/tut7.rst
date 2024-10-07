.. _tut6:


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
is typically required when you have external experimental data (e.g. markers,
ground reaction forces) that you want to use with an OpenSim solver.

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
  effectively, but those skills are useful for debugging (e.g.) issue with
  ``ExternalLoads``.

* **For your own work**, you will need to have your experimental data files in
  a format that's compatible with OpenSim (e.g. ``trc``). Typically, this
  requires either using an OpenSim-aware data exporter, or writing a script
  that converts your experimental data from your source format into an
  OpenSim-compatible format. The data formats used by OpenSim (e.g. ``sto``)
  are typically simple plaintext formats, which means that you can generally
  (e.g.) use a one-off python script to generate them. We recommend looking
  at some existing OpenSim-compatible data files (e.g. this `Example MOT File`_
  from the `OpenSim Models Repository`_) for inspiration.


Topics Covered by this Tutorial
-------------------------------

- A high-level overview of the preview experimental data workflow UI.
- A concrete walkthrough of using the preview experimental data UI in
  a typical experimental workflow.


Opening the Preview Experimental Data UI
----------------------------------------

The preview experimental data UI is an independent "workflow" UI that can be
accessed from OpenSim Creator's splash screen:

``TODO``: screenshot of the splash screen showing where the preview experimental
data UI button is.


Preview Experimental Data UI Overview
-------------------------------------

.. figure:: _static/tut7_preview-experimental-data-ui.jpeg
    :width: 60%

    The preview experimental data UI. In its current (🪄 experimental) iteration,
    it has buttons for loading a model, the model's associated trajectory, raw
    data files (unassociated to the model), and OpenSim XML files (e.g. ``ExternalLoads``).

- ``TODO`` the preview experimental data UI is similar to the model editing UI, but
  with some key differences
- ``TODO`` highlight the ability to load osim files
- ``TODO`` highlight the ability to load motion
- ``TODO`` highlight the ability to load raw data files
- ``TODO`` highlight the ability to load + associate OpenSim XML files, which is handy
  for debugging ``ExternalLoads``, ``PrescribedForces``, etc.
- ``TODO`` roughly explain relationship between raw data files and ``ExternalLoads`` etc.
  but beware that it'll be redundantly explained during the walkthrough in the next
  section.


Walkthrough: Markers to Motion
------------------------------

``TODO`` rough high-level workflow diagram, even a pen drawing of a flowchart would
be fine.

- ``TODO`` direct people to a model's repository data source so they can follow
  along.

- ``TODO`` load marker data, verify it looks ok

- ``TODO`` use marker data in IK? Or "here's one I did earlier?"

- ``TODO`` load IK into the preview experimental data UI and verify that it
  overlaps well with the marker data etc.

- ``TODO`` load GRFs, verify they look ok

- ``TODO`` write an ``ExternalLoads`` that uses the GRFs, show how it can be
  debugged with help from the preview experimental data UI


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
.. _Example MOT File: https://github.com/opensim-org/opensim-models/blob/master/Pipelines/Gait10dof18musc/ExperimentalData/subject01_walk_grf.mot
.. _SimTK.org: https://simtk.org/