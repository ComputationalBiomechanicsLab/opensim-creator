.. _tut6:


Use the Model Warper
====================

.. warning::

    **Model Warping is 🪄 experimental 🪄.**

    You can try it out, get a feel for it, but beware that some things (e.g.
    the ``.warpconfig.toml`` file) aren't established yet, which means they
    may change, as we rollout more robust designs.
    

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
