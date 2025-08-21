Make a Knee
===========

.. warning::

    This tutorial is new ⭐, and ``StationDefinedFrame`` s require OpenSim >= v4.5.1. The content
    of this tutorial should be valid long-term, but we are waiting for OpenSim GUI v4.6 to be
    released before we remove any "experimental" labelling. We also anticipate adding some handy
    tooling around re-socketing existing joints and defining ``StationDefinedFrame`` s.

In this tutorial, we will be making a basic model of a knee using OpenSim Creator:

:red:`TODO`: screenshot of the knee

This tutorial will primarily use the model editor workflow to build a new model that
contains some of the steps/components necessary to build a human model. In essence, the
content here will be familar to that in :doc:`make-a-bouncing-block`, but with a focus
on using landmark data, :doc:`station-defined-frames`, and wrap surfaces to build a
model of a biological system.


Prerequisites
-------------

This tutorial assumes you:

- Have a basic working knowledge of OpenSim, which is covered in :doc:`make-a-pendulum`
  and :doc:`make-a-bouncing-block`.
- (*optional*) The modelling process will also include adding a ``StationDefinedFrame`` to
  the model. The details of how they work is explained in :doc:`station-defined-frames`.
- (*optional*) The building process uses externally-provided landmarks in a CSV. If you
  would like to know how to manually place landmarks on a mesh, we recommend reading
  through :doc:`the-mesh-importer`.


Topics Covered by this Tutorial
-------------------------------

* Creating an OpenSim model by adding bodies and joints
* Adding a ``StationDefinedFrame`` to the model in order to define anatomically
  representative joint frames.
* Adding a muscle to the model
* Adding a wrap surface to the model and associating muscles to that surface.


Download Resources
------------------

In order to follow this tutorial, you will need to download the associated
resources :download:`download here <_static/the-model-warper/walkthrough-model.zip>`.


Top-Level Process explanation
-----------------------------

TODO: briefly give readers an overview of what we want to achieve and how an opensim
model is typically built (bodies, joints, muscles, etc.).


Create a New Model
------------------

TODO: explain creating a blank model from the home screen etc.


Add a Femur Body
----------------

TODO: explain adding a femur body to the new model. Associate the body with
the femur mesh.


Add Femur Landmarks
-------------------

TODO: explain adding markers for the femur to the model as stations/markers.


Add a StationDefinedFrame for the Femur Head
--------------------------------------------

TODO: explain adding a ``StationDefinedFrame`` to the femur head


Add a StationDefinedFrame to the Femur Condyls
----------------------------------------------

TODO: explain adding a ``StationDefinedFrame`` to the femur condyls (repetition)


Add a Tibia Body
----------------
