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

TODO: explain adding a tibia body associated with the tibia mesh, can initially be
attached to ground if it makes the initial landmark/SDF setup easier.


Add Tibia Landmarks
-------------------

TODO: as above, import the landmarks somehow (even if it literally means just adding them
manually and typing in the relevant coordinates).


Add a StationDefinedFrame to the Tibia Head
-------------------------------------------

TODO: as above, ensure there's a frame on the tibial head that makes sense for the joint


Join the Tibia head SDF to the Femur Condyl SDF
-----------------------------------------------

TODO: Mess around with the joint (e.g. via socket editor) to rejoin the existing placeholder joint
(e.g. between tibia and ground) to the femur


Add a Muscle Between the Femur and the Tibia
--------------------------------------------

TODO: describe adding relevant landmarks/stations or whatever is necessary in order to
create a muscle that has one point on the femur and one point on the tibia. Point out that
it's going to look kind of shit initially because it will take the shortest path between the
two (i.e. it'll clip through the meshes etc.)


Add a Knee Wrap Cylinder Wrap Surface
-------------------------------------

TODO: describe adding a knee wrap surface to the relevant body (accessible via the UI by
right-clicking the relevant body -> Add -> WrapSurface or similar).


Associate the Muscle with the Wrap Surface
------------------------------------------

TODO: describe why wrap surfaces are separate from their interaction set (it makes logical sense
but users routinely are confused by this extra step). Associated by right-clicking muscle
and following the relevant menu option

BONUS: Add Markers Ready for IK or similar
------------------------------------------

TODO: describe adding relevant markers, pretend this is how you'd link the model to
lab measurements.

ESMAC2025 Notes: What's Missing Here
------------------------------------

This tutorial isn't going to cover the mesh warper, Thin-Plate Spline, or warping because
that's covered in the subsequent part of the workshop by :doc:`the-model-warper`.
