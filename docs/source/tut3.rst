.. _tut3:

Tutorial 3: Use the Mesh Importer
=================================

In this tutorial, we will be using the mesh importer feature of OpenSim Creator to create a pendulum:

.. figure:: _static/tut3_result.png
    :width: 60%

    The final model made in this tutorial, as-seen in the mesh importer screen. It is a double pendulum made from two bodies and two pinjoints, with decorative meshes used for the heads + struts. The mesh importer screen can be handy for creating new models because it is designed to use absolute coordinates and have multiple utilities for moving and orienting things in the model.

Although the model we will make in this tutorial is effectively an extension of :ref:`tut1`, the **method** used here is different. Here, we will be using the mesh importer feature of OpenSim Creator to create the model, rather than building the model directly in the OpenSim model (``.osim``) editor.

The mesh importer is designed to be *a lot* more relaxed about how/when things can be placed in the scene. Key differences:

* **You can add bodies whenever/wherever you want in the scene**. The mesh importer will automatically add a ``FreeJoint`` if it detects that the body isn't connected to ground. Connections between bodies and joints can be modified at any time. A body's location in 3D space is free-form and the mesh importer will automatically compute the ``OffsetFrame`` necessary to place the body in the exported OpenSim model.

* **You can freely move, orient, and scale anything in the scene - including joints**. The mesh importer uses an absolute coordinate system, rather than a relative one (OpenSim's default). This is a disadvantage when simulating relative quantities (e.g. joint angles), but can be simpler to work with when initially designing the topology of a model. You can move any element in the scene without having to worry about relative coordinate systems, attached elements shifting around, etc.

* **There are more tools for placement/orientation**. Because of the above points (free-form placement, absolute coordinate systems), the mesh importer has more tools available for freely placing things in the model. E.g. it contains tools for orienting things along mesh points, moving elements between other elements, etc.

Overall, the mesh importer is "looser" than the main ``osim`` editor. However, it's disadvantage is that it doesn't directly edit an ``osim`` file. Rather, it edits a simplified "model" that can be exported to the (more complex) ``osim`` format. For this reason, the mesh importer is recommended a *first-step* utility that helps set up the top-level model (e.g. body placement, joint placement) such that it's ready for ``osim`` editor to tackle the more complicated steps (adding/editing forces, contact surfaces, muscles, etc.).


Topics Covered by this Tutorial
-------------------------------

* Using the mesh importer to create a basic OpenSim model
* UI features and shortcuts in the mesh importer
* Exporting + simulating the model created by the mesh importer


Step 1: Open the Mesh Importer
------------------------------

The mesh importer is a separate screen from the main ``osim`` editor. It creates/manipulates a free-form 3D scene that can be exported (one-way) to an ``osim`` model. You can open the mesh importer either from the main menu (``File > Import Meshes``) or through the splash screen:

.. figure:: _static/tut3_open-meshimporter.png
    :width: 60%

    The mesh importer can be opened from the main splash screen (highlighted above with a red box) or through the main menu (``File > Import Meshes``).


One opened, you will be greeted with a new mesh importer scene, which will be used for the next few steps of this tutorial:

.. figure:: _static/tut3_opened-meshimporter.png
    :width: 60%

    The mesh importer screen, which initially loads with a blank scene that's ready for your masterpiece 🎨


Step 2: Add Bodies & PinJoints to Scene
---------------------------------------


Step 3: Add Decorative Geometry
-------------------------------


Step 4: Export and Simulate
---------------------------


(Optional) Extra Exercises
--------------------------


Next Steps
----------