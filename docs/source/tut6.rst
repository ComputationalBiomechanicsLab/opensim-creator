.. _tut6:


🪄 Use the Model Warper
=======================

.. warning::

    **Model warping is very 🪄 experimental 🪄.**

    We invite you to try model warping out and get a feel for how it might be
    useful. Maybe it's already useful enough for your requirements for you to use
    it in something serious (some researchers already have 🎉). However, be
    aware that some implementation details of model warping, such as the structure
    of the ``.warpconfig.toml`` file, aren't stable yet. This means that they may
    change as model warping is developed.

    The documentation here is extremely work-in-progress, so expect many ``TODO`` s
    and ``FIXME`` s. This will be improved as the model warping feature is developed.
    We figured it's better to show you what's happening as we develop it, rather
    than only releasing it once it's perfect.
    

In this tutorial, we will be using the model warping UI to warp an entire
OpenSim model in a landmark-driven way. The benefit of this technique,
compared to standard scaling, is that it makes non-uniform, subject-specific
scaling possible.

.. figure:: _static/tut6_model-warper.png
    :width: 60%

    A screenshot of the model warping UI, showing how the model warper can
    be used to warp a source (reference/template) model into a new model. The
    warping relationship is possible because the meshes in this model have
    fully-paired landmarks with the "target" meshes.


Prerequisites
-------------

* **You can diagnose/work-with with OpenSim models**. This tutorial assumes that
  you're able to diagnose the models that goes into, and come out of, the model
  warping UI. If you don't feel comfortable with working on OpenSim models, then
  go through earlier tutorials (e.g. :doc:`tut1`).

* **A basic understanding of the Thin-Plate Spline (TPS) technique**. The model
  warper applies the TPS technique to multiple parts of the source model. Therefore,
  it's recommended that you have already gone through :doc:`tut5`. That tutorial
  outlines how to pair landmarks on a single mesh to produce one warp transform. The
  model warping UI combines many transforms together with an OpenSim model.


Topics Covered by this Tutorial
-------------------------------

* The theory behind how the model warper works
* How to prepare an existing OpenSim source/template model for warping
* Concrete walkthrough of warping a simple model
* Customizing model warping behavior
* Diagnosing and working around model warping issues
* Limitations, references, future work


Model Warping: Theory
---------------------

``TODO`` : high-level explanation of how the model warper rescales the model and how
it differs from a generic scaling technique.


Preparing an OpenSim Model for Warping
--------------------------------------

``TODO`` : explain what the model warper can/can't warp. Explain ``StationDefinedFrame``
and limitations around warping frames, muscle scaling, etc.


Basic Example: Two-body model
-----------------------------

``TODO`` : provide a very stripped-down model that meets the requirements for warp-ability


Customizing Model Warping behavior
----------------------------------

``TODO`` : Explain how the user can use the ``.warpconfig.toml`` file to customize how the
model warper warps models.


Diagnosing Warping Issues
-------------------------

``TODO`` : explanation of any known issues, recommendations for working around them
etc.


Advanced Example: Many-Bodied Model with Custom Requirements
------------------------------------------------------------

``TODO`` : an example model that requires the user to specialize/specify customization
in the warp config (e.g. tell the warp engine to skip some steps, warp X using
technique Y, etc.)
