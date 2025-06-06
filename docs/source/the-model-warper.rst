.. _the-model-warper:

The Model Warper
================

.. warning::

    **The model warper is very very 🪄 experimental 🪄.**

    The model warper has been in development since 2023 and has undergone several
    redesigns from its initial version (an extended form of :ref:`the-mesh-warper`)
    to what it is now (a way of combining various scaling steps in a linear pipeline).
    This is because because non-linear "model warping" combines a variety of scaling
    algorithms in a model-specific way - our goal is to provide clear UI tooling
    that makes combining those algorithms easier.

    We invite you to try the model warper out and get a feel for how it might be
    useful. Maybe it's already useful enough for you to use it in something
    serious (some researchers already have 🎉).

In this tutorial, we will be using the model warper to create a warping pipeline that
can be used to warp an entire OpenSim model using experimental measurements, such as
CT scans and weight measurements. The benefit of the model warper is that it lets you
combine various, potentially non-uniform, scaling steps into a single warping pipeline
that's standard, introspectible, and reusable.

.. _model-warper-ui:
.. figure:: _static/the-model-warper/model-warper.jpg
    :width: 60%

    The model warping UI. This tutorial goes through top-level model warping concepts
    and how OpenSim Creator's UI tooling helps design and execute a model warping
    procedure.


Prerequisites
-------------

* **You can diagnose and work with OpenSim models**. This tutorial assumes that
  you're able to diagnose the models that go into, and come out of, the model
  warping UI. If you don't feel comfortable with working on OpenSim models, then
  we recommend going through some model-building tutorials (e.g. :doc:`make-a-pendulum`,
  :doc:`make-a-bouncing-block`).

* **A basic understanding of the Thin-Plate Spline (TPS) technique**. The walkthrough
  in this tutorial uses the TPS technique to warp parts of the model. Therefore, it's
  recommended that you have already gone through :doc:`the-mesh-warper`, which outlines
  pairing landmarks between two corresponding meshes as inputs for the TPS technique.

* **Familiarity with StationDefinedFrames**. The walkthrough in this tutorial uses 
  ``StationDefinedFrame``\s so that non-linear TPS scaling steps correctly recompute
  the source model's joint frames. The :doc:`station-defined-frames` documentation
  outlines what ``StationDefinedFrame``\s are and how to add them to models.


Topics Covered by this Tutorial
-------------------------------

* A technical overview of how the model warper works
* A concrete walkthrough of warping a simple model
* An explanation of how model warping behavior can be customized


Technical Overview
------------------

A **model warping procedure** applies a sequence of **scaling steps** to the
**source model** one-at-a-time to yield a **result model**. Each scaling
step may require some sort of **scaling parameter**, or external data, to execute
successfully. Model warping procedures are customizable. The number, order, and
behavior of each scaling step may differ from procedure to procedure. This is to
accomodate a variety of source models, experiments, and scaling requirements.

OpenSim Creator provides a workflow for designing and executing a model warping
procedure, pictured in :numref:`model-warper-overview-screenshot`. The workflow
UI is designed to provide visual feedback about each scaling step, so that you
can incrementally build a warping procedure one scaling step at a time. The model
warping procedure can then be saved to a standard XML file so that it can be reused
and modified.

.. _model-warper-overview-screenshot:
.. figure:: _static/the-model-warper/model-warper.jpg
    :width: 60%

    The model warping workflow UI contains a toolbar with buttons for creating/loading
    the source model, warping procedure, and various other useful functions (top); a
    control panel for editing the scaling parameters of a warping procedure and
    an editable list of toggleable scaling steps which are applied in-order (left);
    and 3D views that show the source model and result (warped) model after applying
    the scaling steps side-by-side (right).


Walkthrough
-----------

This walkthrough goes through the process of building a model warping procedure from
scratch. The aim is to show how the model warping workflow can be used to build tricky
non-linear model warping procedures.

In particular, we will develop a procedure that warps a simple healthy leg model to account
for femoral torsion. Torsion is tricky to handle because standard scaling techniques, which
typically perform linear scaling, cannot handle non-linear, localized, morpological
changes. Therefore, a designer who's familiar with the model, the underlying biomechanics,
and the available experimental scaling parameters needs to develop a custom scaling/warping
procedure, which is where the model warping workflow may help.


Open the Model Warper Workflow UI
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The model warper is a specialized workflow in OpenSim Creator and can be accessed from the
splash screen:

.. figure:: _static/the-model-warper/model-warper-open-button-on-splash-screen.jpeg
    :width: 60%

    The model warper can be opened from the splash screen of OpenSim Creator (circled red).

This should open a blank model that has no scaling steps:

.. figure:: _static/the-model-warper/blank-model-warper-ui.jpeg
    :width: 60%

    A screenshot of the model warping UI when it's first opened.


Load the Source Model
^^^^^^^^^^^^^^^^^^^^^

.. note::

  We have already prepared a source model for this workflow, you can download it here (**TODO**).

  The model contains two bodies (upper leg, lower leg) joined together with a pin joint that
  uses :doc:`station-defined-frames` to represent (very roughly) a knee and one muscle that
  crosses that joint over a single wrap cylinder to represent (again, roughly) how the muscle
  wraps over bone.


Use the ``Source Model`` entry in the model warper's toolbar to load the source model ``.osim``
file. This should load the model and show it in the ``Source Model`` UI panel:

.. _model-warper-after-loading-model:
.. figure:: _static/the-model-warper/model-warper-after-loading-source-model.jpeg
    :width: 60%

    The model warper after loading the source model (**DOWNLOAD LINK TODO**).


Add a Mesh Warping Step
^^^^^^^^^^^^^^^^^^^^^^^

The model warper is designed around applying scaling steps to the source model one-by-one
to produce the result model. :numref:`model-warper-after-loading-model` shows the most
trivial case of this process, which is to apply no scaling steps and produce a result
model that's identical to the source model.

The essence of building a model warping procedure is to incrementally add the scaling
steps you need. The first step is to apply the subject's (target) femoral torsion to
the femur bone mesh in the model. There are external tools available online to do
this (e.g. `this one <https://simtk.org/projects/bone_deformity>`_) but, for this
walkthrough, we will use the Thin-Plate Spline technique, as described in
:doc:`the-mesh-warper`, because the model warper has in-built support for it.

.. note::

  In preparation for the mesh warping step, we have already established a TPS warp
  *from* the source femur mesh in the model *to* a subject-specific femur mesh by
  pairing landmarks between them in :doc:`the-mesh-warper`. Here's a screenshot of
  how that looked:

  .. figure:: _static/the-model-warper/mesh-warper-showing-basic-TPS-warp-of-femur.jpeg
    :width: 60%

    Screenshot of how landmarks were paired between the source mesh and the destination one
    in the mesh warper. Rotation and translation (i.e. reorientation) of the mesh was removed
    from the TPS warp using the appropriate checkboxes to correct for subject/scanner
    orientation. Destination data was pre-scaled by 0.001 to account for a difference in
    units between the mesh files (meters vs. millimeters).

  For the pruposes of model warping, all you need to know is that the TPS warping technique
  requires a ``.landmarks.csv`` for the "source" and a ``.landmarks.csv`` for the destination.
  Where the landmarks in those files come from is up to you (:doc:`the-mesh-warper` is one way).
  The model warping implementation uses pairs of landmarks from those files to warp, scale, reorient,
  and translate the applicable mesh, station, or muscle point.

The model warper's TPS-based mesh scaling step requires two sequences of landmarks. The model
download zip (**TODO**) includes a ``Geometry/`` directory that contains ``femur_r.landmarks.csv``
and ``subject_femur_r.landmarks.csv``, which represent ``femur_r.vtp``\'s landmarks and landmarks
``subject_femur_r.stl``\'s landmarks respectively.

To add a scaling step in the model warper UI, click the appropriate button and add a "Apply Thin-Plate
Spline (TPS) to Meshes" step (pictured in :numref:`model-warper-apply-tps-to-meshes-button`).

.. _model-warper-apply-tps-to-meshes-button:
.. figure:: _static/the-model-warper/apply-thin-plate-spline-to-meshes-scaling-step-button.jpeg
    :width: 60%

    The "Add Scaling Step" button in the model warper UI opens a menu where you can select
    the type of scaling step to add to the model warping procedure. In this first step, we
    add a "Apply Thin-Plate Spline (TPS) to Meshes" step.

Once you add the scaling step, you will find that the ``Result Model`` panel is blanked out with
error messages (:numref:`model-warper-after-adding-mesh-warping-step`). This is because the step
has been added, but the model warping procedure now needs additional information (i.e. which mesh
to warp and the two corresponding ``.landmarks.csv`` files) in order to apply the scaling step to
the source model.

.. _model-warper-after-adding-mesh-warping-step:
.. figure:: _static/the-model-warper/error-after-adding-mesh-warping-scaling-step.jpeg
    :width: 60%

    After adding the "Apply Thin-Plate Spline (TPS) to Meshes" scaling step, the UI stops showing
    the resultant (output) model because the warping procedure is missing the information it needs
    to apply the step.

To fix this issue, you need to fill in the values from :numref:`model-warper-mesh-scaling-properties`
in the appropriate input boxes. Once you do that, you should end up with something resembling
:numref:`model-warper-after-applying-tps-mesh-warp`.

.. _model-warper-mesh-scaling-properties:
.. list-table:: Property values for the TPS femur mesh scaling step.
   :widths: 25 25 50
   :header-rows: 1

   * - Property Name
     - Value
     - Comment
   * - ``source_landmarks_file``
     - ``Geometry/femur_r.landmarks.csv``
     - Source landmark locations
   * - ``destination_landmarks_file``
     - ``Geometry/subject_femur_r.landmarks.csv``
     - Destination landmark locations
   * - ``landmarks_frame``
     - ``/bodyset/femur_r``
     - The coordinate frame that the two landmark files are defined in.
   * - ``destination_landmarks_prescale``
     - 0.001
     - The destination landmarks (and mesh) were defined in millimeters.
   * - ``meshes``
     - ``/bodyset/femur_r/femur_r_geom_1``
     - Path within the OpenSim model to the femur mesh component that should be warped.

.. _model-warper-after-applying-tps-mesh-warp:
.. figure:: _static/the-model-warper/after-applying-tps-mesh-warp.jpeg
    :width: 60%

    The model warping UI after applying the mesh warping step. The warped mesh is shorter
    and slightly twisted when compared to the source mesh. Warping the joint frames, muscle
    points, and wrap geometry is handled later in this walkthrough. An easy way to see what
    a scaling step is doing is to toggle the ``enabled`` button on the step.

Add a Frame Warping Step
^^^^^^^^^^^^^^^^^^^^^^^^

**TODO**: Explain why ``StationDefinedFrame``\s are relevant for this step.

**TODO**: explain adding the scaling step, and any unusual-looking behavior (e.g. of
muscle points or bodies snapping around because they are temporarily in a new coordinate
system?)


Add a Muscle Point Scaling Step
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**TODO**: explain how the muscle points ride on the skeleton, so they need to be warped too
**TODO**: explain adding the step and then maybe a screenshot of before/after


Add a Wrap Cylinder Scaling Step
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**TODO**: explain the difficulties of scaling wrap cycliners (they're analaytic geometry, they're
based on external information like muscle density, they have an orientation and wrapping quadrant
that should be handled carefully, etc.)


Add a Body Mass Scaling Step
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**TODO**: add a mass scaling step. This is just another scaling step but is a good opportunity to
discuss the relevance of having scaling parameters.


Export Result Model
^^^^^^^^^^^^^^^^^^^

**TODO**: export the result model to a model editor and prompt the reader to save it if they like it.


Summary
-------

**TODO**: quick runthrough of what was communicated, why/where model warping can be useful
and an invite to try it on other models!
