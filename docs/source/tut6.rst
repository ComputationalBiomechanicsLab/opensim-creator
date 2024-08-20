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
    

TODO: overview and screenshot, top-level motivations, etc.

Prerequisites
-------------

TODO: model editing, mesh warping, a basic understanding of Thin-Plate Spline


Topics Covered by this Tutorial
-------------------------------

* TODO: how to ensure a model is warp-able
* TODO: basic example of a two-body model
* TODO: more advanced example walkthrough
* TODO: limitations, references, future work


Warpable Model Requirements
---------------------------

TODO: explain what the model warper can/can't warp. Explain ``StationDefinedFrame``
and limitations around warping frames, muscle scaling, etc.

Basic Example: Two-body model
-----------------------------

TODO: provide a very stripped-down model that meets the requirements for warp-ability


Advanced Example: Many-Bodied Model with Custom Requirements
------------------------------------------------------------

TODO: an example model that requires the user to specialize/specify customization
in the warp config (e.g. tell the warp engine to skip some steps, warp X using
technique Y, etc.)
