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
.. figure:: _static/use-the-model-warper/model-warper.jpg
    :width: 60%

    TODO


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

* **Familiarity with StationDefinedFrames**. TODO non-linear scaling essentially requires
  this type of frame definition.


Topics Covered by this Tutorial
-------------------------------

* A technical overview of how the model warper works
* A concrete walkthrough of warping a simple model
* An explanation of how model warping behavior can be customized


Technical Overview
------------------

TODO: explanation of ``ScalingStep`` s, visualization, etc.


Walkthrough
-----------

TODO: step-by-step walkthrough of the latest model warper UI
on a basic model (2 body) where we want to add some torsion
to a bone or something similar like that.


Frequently Asked Questions
--------------------------

TODO: explain some tips about how to use the model warper in
various challenging situations, write up some of the internal
questions we've had about its functionality.
