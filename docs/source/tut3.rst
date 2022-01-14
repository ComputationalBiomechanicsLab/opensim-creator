.. _tut3:

Tutorial 3: Use the Mesh Importer
=================================

In this tutorial, we will be using the mesh importer feature of OpenSim Creator to create a pendulum:

.. figure:: _static/tut3_result.png
    :width: 60%

    The final model made in this tutorial, as-seen in the mesh importer screen. It is a double pendulum made from two bodies and two pinjoints, with decorative meshes used for the heads + struts. TODO DOWNLOAD MODEL

Although the model we will make in this tutorial is effectively an extension of :ref:`tut1`, the **method** used here is different. Here, we will be using the mesh importer feature of OpenSim Creator to create the model, rather than building the model directly in the OpenSim model (``.osim``) editor screen.

The mesh importer is "looser" than the main ``osim`` editor. However, it's disadvantage is that it doesn't directly edit an ``osim`` file. Rather, it edits a simplified "model" that can be exported to the (more complex) ``osim`` format. For this reason, the mesh importer is recommended a *first-step* utility that helps set up the top-level ``osim`` model ready for the ``osim`` editor to tackle the things like adding/editing forces, contact surfaces, etc. See :ref:`doc_meshimporterwizard` for an overview of the mesh importer's main features.


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


Step 2: Add Bodies & PinJoints
------------------------------

You can add bodies to the scene by either:

* Right-clicking somewhere in the 3D scene and clicking ``Add Other > Body``
* Using the dropdown at the top of the scene: ``Add Other > Body``

Joints can be created in the scene by:

* Right-clicking on a **body** in the scene and clicking ``Join to`` or ``Add Other > Joint``
* Clicking on the thing in the scene that the body should join to

All scene elements can be edited by right-clicking them. Feel free to experiment 👩‍🔬 with the available menus/actions: accidents can always be reversed (``Ctrl+Z`` or ``Edit > Undo``).


For this particular model, you will need to:

- Add a body called TODO

NAMING CONVENTIONS

* pendulum_base
* ground_offset, pendulum_base_offset (uh - weld joint?)
* pendulum_base_to_ground (WeldJoint)
* pendulum_head
* pendulum_head_to_pendulum_base
* pendulum_head_2
* pendulum_head_2_to_pendulum_head



Step 3: Add Decorative Geometry
-------------------------------


Step 4: Export and Simulate
---------------------------


(Optional) Extra Exercises
--------------------------


Next Steps
----------