.. _tut4:


Tutorial 4: Make an Arm
=======================

In this tutorial, we will be using OpenSim Creator to create a basic human hand from some mesh files:

.. figure:: _static/tut4_final-result.png
    :width: 60%

    TODO: correct screenshot and model + meshes. The model created from these :download:`📥 meshes <_static/arm_meshes.zip>`. TODO download model link etc.

This tutorial will use the **mesh importer** to import mesh files that represent an arm, followed by using the mesh importer to assign the meshes to custom-placed **bodies** and **joints**. Once they are placed, the tutorial then introduces using **stations** to mark "points of interest" that can be used in the ``osim`` editor to define **muscle paths**.

This is a harder tutorial that builds on top of several techniques that were demonstrated in previous tutorials:

* :ref:`tut1`: Adding components into an osim, editing component values
* :ref:`tut2`: Adding forces into a model, making refinements/simplications based on preliminary simulation results
* :ref:`tut3`: Importing meshes and placing bodies/joints with the mesh importer

Those techniques are used *ad nauseam* here. Therefore, it is strongly recommended that you consult those tutorials if you aren't sure what's going on here.


Topics Covered by this Tutorial
-------------------------------

- **Importing meshes** with the mesh importer.
- Using the mesh importer to **attach meshes to bodies** and **join bodies with joints**
- Using the mesh importer to mark points of interest in the model by **adding stations**
- Using the ``osim`` editor to add **muscle paths** to the model


Step 1: Import Meshes
---------------------

TODO overview with link to meshes

* TODO
* Instruct to open mesh importer
* Give link to meshes (again)
* Explain import (add meshes or drag drop)
* Screenshot


Step 2: Add Bodies to the Meshes
--------------------------------

TODO overview of bodies with maybe hand-draw diagrams

* Instruct to right-click finger bones and add bodies at centerpoint, click location, whatever
* Assign a whole finger up to the elbow
* Explain any handy movement commands, reorientation, etc.
* Screenshot of assignment up to the elbow
* Instruct to assign rest of the hand. Include a naming convention diagram (e.g. annotated screenshot?)
* Screenshot of fully-assigned hand model in mesh importer


Step 3: Add Joints Between the Bodies
-------------------------------------

TODO overview of joints between the bodies, maybe explain some OpenSim-specific stuff like how Z is specifically useful in OpenSim for PinJoints or whatever.

* Instruct to right-click bodies in the 3D viewer or hierarchy viewer. Use ``Join to`` to create a joint to the next body up.
* Assign all joints from the same finger (as for bodies) up to the elbow
* Screenshot of assignment up to the elbow
* Instruct to assign rest of the hand. Include naming convention diagram (e.g. annotated screenshot?)
* Screenshot of fully-assigned model


Step 4: Mark Points of Interest on the Meshes
---------------------------------------------

TODO overview of the problems of assigning muscle insertion points etc. The challenges of figuring out where they are within the related body frame etc.

* Instruct to right-click meshes in the 3D viewer or hierarchy viewer. Use ``Add > Station`` add a station at the click locaiton. Make it clear that they can also be freely moved around in 3D space afterwards.
* Instruct to assign stations along the same finger-to-elbow path as before
* Screenshot of assignment up to the elbow
* This tutorial skips assigning the rest (have it as an extra exercise)
* Include naming conventions for the muscles
* Screenshot of the assigned model


Step 5: Convert to an OpenSim Model
-----------------------------------

TODO guide for converting to an opensim model

* Instruct to click the button
* Advise running some basic simulations on the model
* Advise playing around with the joint coordinates a little bit
* Advise saving as an osim and opening in the official OpenSim GUI
* Screenshot of some basic simulation running with the muscle-less model


Step 6: Add Muscle Paths
------------------------

TODO guide for adding muscle paths into the model

* Instruct to click ``Add Muscle > Thelen`` or something
* Instruct to add points to the muscle path (TODO: needs to be added into UI). Instruct to use the stations as places the points can be added
* Instruct to add all muscles for the stations that were marked up (entire finger and up to the elbow)
* Instruct to play around with muscle parameters, try simulating with different params, etc.
* Screenshot of the model with a muscle assigned


Summary
-------

* In this guide, we covered etc. etc. etc.


(Optional) Extra Exercises
--------------------------

* Assign the rest of the muscles
* Try different muscles
* Try adding contact surfaces? (maybe this should be a separate tutorial - advanced model compositing or whatever)


Next Steps
----------
