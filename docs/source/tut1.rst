.. _tut1:

Tutorial 1: Make a Pendulum
===========================

In this first tutorial, we will be making a conventional pendulum using OpenSim Creator:

.. figure:: _static/tut1_appearanceformatted.png
    :width: 60%

    The pendulum created by this tutorial. Although OpenSim is commonly associated with biomechanical simulations, it can also be used to simulate "conventional" rigid-body scenes.

OpenSim is based on `Simbody <https://github.com/simbody/simbody>`__, a physics library for science- and engineering-quality simulations of articulated mechanisms. This means that OpenSim Creator can be used to simulate things like pendulums and robots, not just biomechanical simulations.

This tutorial focuses on creating a pendulum--one of the simplest physical systems--because it introduces core concepts that are common to all kinds of models - including complex human models.

Step 1: Create a New Model
--------------------------

In OpenSim Creator, create a new model. It should create a blank scene that looks like this:

.. figure:: _static/tut1_blankscene.png
    :width: 60%

    A blank OpenSim model. You can create a new model by clicking "New Model" in the splash screen, or pressing ``Ctrl+N``. The blank scene contains the ground frame. All frames are color-coded with red, green, and blue indicating X, Y, and Z respectively.

You should see a blank 3D scene with a chequered floor and a set of axes in the middle. In OpenSim terminology, these axes are **frames**. Frames show the orientation of something in the scene. In this case, they are showing the orientation of the model's **ground**. The ground of a model is always located at ``(0, 0, 0)`` in the scene. This means that the red, green, and blue axes of the ground frame correspond to +X, +Y, and +Z in the scene (the "world").

.. note::

    OpenSim models are described using a **relative coordinate system**. This means that the position and orientation of each component (e.g. a body) in an OpenSim model is described relative to some other component in the scene.

    What this *practically* means is that, in more complex models, setting a component's offset to +1 in X does not mean that the component will be positioned at ``(1, 0, 0)`` in the scene. The component may be positioned at ``parent.position + parent.orientation*(1, 0, 0)``, or some other location, depending on what (and how) the component is attached to the other components in the model (the model's **topology**). This is in contrast to artistic modelling software (e.g. `Blender <//blender.org>`__), where scene elements are typically transformed relative to the scene.

    The only component that doesn't have relative coordinates is the **ground**. The ground is the "root" of the model and is always defined to be at ``(0, 0, 0)``. All other components in the model must attach to ground directly or indirectly (i.e. though other components).


Step 2: Add a Body with a WeldJoint
-----------------------------------

In the UI, click the ``add body`` button. A dialog should pop up that prompts you to fill in the body's details. Create a body with the following details:

.. figure:: _static/tut1_addbodymodal.png

    ``pendulum_base``'s' body properties. **Note**: Make sure to also attach a ``Brick`` generated geometry that you can see the body in the visualizer.

You should now see a cube in the scene. The cube is the ``Brick`` that was attached to the newly-created body:

.. figure:: _static/tut1_firstbodyadded.png
    :width: 60%

    The scene after adding ``pendulum_base`` into the scene with a ``Brick`` as its attached geometry. Although OpenSim models bodies as point elements, many OpenSim models also attach 3D geometry to the bodies to make the model easier to visualize.

The ``pendulum_base`` body was added into the scene as being attached to the ground with a ``WeldJoint`` via two offset frames (this is what ticking the ``add offset frames`` did). The topology of the model looks something like this:

.. figure:: _static/tut1_firstbody_topology.svg
    :width: 25%

    The logical topology of the model after adding ``pendulum_base`` into the scene. This topology dictates the relative coordinates and physical dynamics of those elements in the model. Here, ``pendulum_base`` is attached to ``ground`` via a ``WeldJoint``. A ``WeldJoint`` has no degrees of freedom, so ``pendulum_base`` is effectively "anchored" in the scene.


.. note::

    OpenSim models are stored in an interconnected **hierarchy**. A model has child components--things like **bodies** and **joints**--and those child components, themselves, have children--e.g. things like **offset frames** and **decorative geometry**. Clicking something in OpenSim Creator typically selects the exact component you clicked on, so clicking the ``Brick`` in the 3D scene will select the ``Brick`` geometry that was added as a direct child of the ``pendulum_base`` body you added. You can then use the hierarchy viewer to see where the selected component is in the hierarchy.

    An OpenSim model is also interconnected via **sockets**. These allow parts of the hierarchy to connect to eachother in a non-hierarchical (specifically, graph-like) manner. For example, bodies and joints are direct children of a model--they are siblings--but joints use sockets (``parent_frame`` and ``child_frame``) to connect two frames in the model. Those two frames *may* be bodies.

    When these tutorials write about the **topology** of the model, we're usually talking about how the various bodies, joints, and frames *affect* eachover. That is dictated by the socket connectivity graph. The model hierarchy is more focused on the **storage structure** of the model and affects things like where the data is ultimately saved.

To reposition ``pendulum_base`` in the scene, we can change the ``translate`` property of either ``ground_offset`` or ``pendulum_base_offset``, which are offset frames that were added into the scene when the ``pendulum_base`` was added with ``add offset frames`` selected.

So, to move ``pendulum_base`` in the scene:

* Find ``jointset`` in the hierarchy viewer
* Find ``base_to_ground`` in the ``jointset``
* Find ``ground_offset`` and click it
* Change the ``translation`` property to ``(0.00, 1.00, 0.00)``

This will move the ``ground_offset`` frame +1 in ground's Y, which is the same as the scene's Y, so it will move ``ground_offset`` vertically upwards. Because ``pendulum_base`` is attached to ``ground_offset`` via a ``WeldJoint``, ``pendulum_base`` will also move vertically upwards:

.. figure:: _static/tut1_firstbodymoved.png
    :width: 60%

    The scene after changing ``ground_offset``'s ``translation`` property. Changing it also changed where ``pendulum_base`` is in the scene because of the topographical relationship between ``pendulum_base`` and ``ground_offset``.

.. note::

    Although this is only a small part of the model-building process, this first step covers *a lot* of core OpenSim topics such as adding bodies, selecting joints, attaching frames to eachover, and understanding the relative coordinate system.

    Try to get familiar with these basics. You will encounter them frequently. Experiment by changing the translation of the other offset frame (``base_offset``), use negative translations, attach different geometry, or change the geometry's appearange (for a ``Brick``, half widths can be changed to make it smaller/bigger).


Step 3: Add the pendulum head
-----------------------------

In the previous step, we created ``pendulum_base``, which is a body that is "welded" into the scene at some vertical (Y) offset. The next step is to create a ``pendulum_head`` that is attached to ``pendulum_base`` with a ``PinJoint``. The ``PinJoint`` is what will give ``pendulum_head`` one degree of freedom, enabling it to swing relative to ``pendulum_base``.

In the UI, click the ``add body`` button. Create a body with the following details:

.. figure:: _static/tut1_addpendulumhead.png

    ``pendulum_head``'s' body properties. **Note**: Make sure to also attach a ``Sphere`` generated geometry so the body so that you can see it in the visualizer.

.. warning::
    This should add ``pendulum_head`` into the scene. **However** you may not be able to see it, because it's at the exact same location as ``pendulum_base`` (it's attached to it) and is represented by a ``Sphere`` that is smaller than ``pendulum_base``'s ``Brick`` - it's *inside* ``pendulum_base``.

Next, we need to move ``pendulum_head`` such that it is below ``pendulum_base`` in the scene. It's best to keep the model's topology in mind when doing this. After adding ``pendulum_head``, the new model graph looks something like this:


.. figure:: _static/tut1_secondbody_topology.svg
    :width: 25%

    Topology of the model after adding ``pendulum_head``. Although we have only added two bodies, ``PhysicalOffsetFrame`` s have also been added between the bodies and their joints. This enables later moving (offsetting) a body relative to a joint it's (indirectly) attached to. Without the offset frames, the bodies would have to be attached at the joint's location. This isn't suitable for a pendulum, where the pendulum's head is typically *offset* from the ``PinJoint`` it will swing on.

The model's topology may look complicated, but keep the main goal in mind: we want ``pendulum_head`` to be offset from the ``PinJoint`` that it will be swinging on. Therefore, we need to change the ``translation`` property of the ``pendulum_head_offset`` that the ``PinJoint`` (``pendulum_head_to_pendulum_base``) is attached to.

To change the offset between the pendulum head and the ``PinJoint`` it swings on:

* Find ``jointset`` in the Hierarchy Viewer
* Find ``pendulum_head_to_pendulum_base`` in the ``jointset``
* Find ``pendulum_head_offset`` under that and click it
* Change the ``translation`` property to ``(0.0, 0.5, 0.0)``

After setting ``pendulum_head_offset``'s ``translation`` to ``(0.0, 0.5, 0.0)``, you should be able to see the pendulum head floating below the ``pendulum_base``:

.. figure:: _static/tut1_secondbodymoved.png
    :width: 60%

    How the scene should look after adding ``pendulum_head`` (a ``Body``) and setting ``pendulum_head_offset``'s ``translation`` to ``(0.0, 0.5, 0.0)``. The sphere is the decoration for ``pendulum_head`` and the cube is the decoration for ``pendulum_base``.

.. note::

    We just set the ``translation`` property of ``pendulum_head_offset`` to +0.5 in Y, but it moved down, not up, in the scene. Why?

    It's because of how the **relative coordinate system** interplays with the topography of the model.

    Looking at the topography graph (above), you'll see that the ``PinJoint`` is attached to both the ``pendulum_head_offset``  and ``pendulum_base_offset`` frames. The ``PinJoint`` enforces that the two frames its attached to are constrained to the same location (the only degree of freedom a ``PinJoint`` has is its single rotational axis). By setting ``pendulum_head_offset``'s translation to ``(0.0, 0.5, 0.0)``, we are stipulating that ``pendulum_head_offset`` *must* be 0.5Y above ``pendulum_head`` (in ``pendulum_head``'s coordinate system). The only way to do this, while satisfying ``PinJoint``'s constraints, is to put the ``pendulum_head`` 0.5Y below ``pendulum_head_offset`` in the scene.

    A rule of thumb for understanding how OpenSim resolves locations in the scene is to mentally traverse the topography graph. Start at the ground, which *must* be at ``(0.0, 0.0, 0.0)``, and work towards what you are working on (in this case, ``pendulum_head``). Each element you encounter (e.g. a body, a ``PinJoint``, or an offset frame) may additively enforce some kind of constraint or change in orientation.

Next, we are going to rotate the pendulum head along its swing direction slightly. At the moment, ``pendulum_head`` is directly below ``pendulum_base``. The only force acting on the scene is gravity (a force field with a vector of ``(0.0, -9.8, 0.0)``), so the pendulum head won't move when we simulate it. You can see this problem for yourself by running a simulation. The scene should be motionless.

We can "pre-swing" ``pendulum_head`` a little by starting it off at an angle. The ``PinJoint`` we used to attach the pendulum head to the pendulum base (``pendulum_head_to_pendulum_base``) has a single degree of freedom, ``rz``, which is exposed as a **coordinate** that can be edited. When the ``PinJoint`` was added, ``rz`` was given a default value of ``0.0`` (no rotation). You can edit the ``default_value`` property of ``rz``  to rotate ``pendulum_head`` along the ``PinJoint``'s degree of freedom slightly.

To change the ``rx`` coordinate of ``pendulum_head_to_pendulum_base``:

* Find ``pendulum_head_to_pendulum_base`` under ``jointset`` in the hierarchy viewer.
* Click ``rz`` to edit the ``rz`` model coordinate
* Use the Properties Editor to change ``rz``'s ``default_value`` property to ``1.0`` (radians)

After changing ``rz``, the pendulum head should be rotated slightly:

.. figure:: _static/tut1_pendulumheadjointrxchanged.png
    :width: 60%

    The pendulum after modifying the ``PinJoint``'s ``rz`` coordinate. By modifying the ``rz`` coordinate value, we are changing the angle between ``pendulum_base_offset`` and ``pendulum_head_offset`` (the parent + child of the ``PinJoint``). Because ``pendulum_head`` is attached to ``pendulum_head_offset``, this has the overall effect of moving the ``pendulum_head``.

If you simulate the model now, you should see that ``pendulum_head`` swings like a pendulum 😊

.. note::

    Hooray 🎉, we have created a functioning pendulum by adding two bodies and two joints into a model.

    Think about that for a second: at no point in this tutorial did we refer to anything pendulum-specific (e.g. the pendulum equation). Instead, we've created a physical system that has the same topology and constraints as a pendulum and simulated that. The simulation then produced the same *behavior* as an ideal pendulum.

    This approach can be *extremely* useful, because it means that we can design physical systems on a computer and then simulate those systems to yield physically-representative data. That data can then be compared to scientific predictions, or experimental measurements, to provide a deeper insight.

    Although a pendulum may not be all that impressive, the principles shown here scale to complex systems. Maybe the pendulum equation is simple, but what about a double pendulum, or a triple pendulum? What if we replace some of the attachments with springs? What about a leg containing many bodies, muscles, and joints attached in a complex topography? What if the pendulum hits a wall midway through its swing?


Step 4: (optional) Make the Pendulum Look Nicer
-----------------------------------------------

Although we *logically* have a pendulum that meets our requirements (a mass joined at some distance to a pivot point), our model certainly doesn't *look* like a pendulum. Lets fix that.

First, we can make the base into a thinner ceiling-like brick by changing the ``Brick``'s ``half_lengths`` property:

* Click the ``pendulum_base``'s cube in the visualizer, or browse to ``pendulum_base_geom_1`` in the hierarchy
* Change the ``half_lengths`` property to something like ``(0.2, 0.01, 0.2)``. This property only represents the *appearance* of the model, not the *behavior*.

Next, we can make the pendulum head a little smaller by changing the ``Sphere``'s ``radius`` property:

* Click the ``pendulum_head``'s sphere in the visualizer, or browse to ``pendulum_head_geom_1`` in the hierarchy
* Change the ``radius`` property to something like ``0.05``

Finally--and this is the harder part--we need to add a ``Cylinder`` in-between the ``pendulum_head`` and the ``PinJoint`` to act as a strut. The easiest way to do this is to add an offset frame that is between those two points (i.e. 0.25Y above ``pendulum_head``) and attach a ``Cylinder`` decoration to that frame. To do this:

* Select the ``pendulum_head`` in the hierarchy
* Click ``add offset frame`` in the properties editor, which should create and select ``pendulum_head_offsetframe``
* Set ``pendulum_head_offsetframe``'s ``translation`` to ``(0.0, 0.25, 0.0)``
* Click ``add geometry`` in the properties editor to add a ``Cylinder`` to ``pendulum_head_offsetframe``.
* Click the cylinder in the visualizer, or find ``pendulum_head_offsetframe_geom_1`` in the hierarchy
* Set the ``Cylinder``'s ``radius`` property to ``0.01`` and its ``half_height`` property to ``0.25``

Once you've done that, you should end up with a more convincing-looking pendulum:

.. figure:: _static/tut1_appearanceformatted.png
    :width: 60%

    Final pendulum model after updating the appearance. You can download the final model :download:`here <_static/tut1_final-model.osim>`



(Optional) Extra Exercises
--------------------------

* **Make a double pendulum**. Using similar steps to the ones used to set up ``pendulum_head``, create a second pendulum head that attaches to ``pendulum_head`` rather than ``pendulum_base``. This will create a double pendulum.

* **Open the pendulum in the official OpenSim GUI**. Save your pendulum to an ``.osim`` file and open it in the official OpenSim GUI. This will give you the chance to view it in other software, and might give you some alternative options (e.g. different plotting tools, more functionality in some areas)


Next Steps
----------

Although the model created here is simple, this tutorial had to  introduce quite a few OpenSim concepts that you will encounter again and again. Concepts like **bodies**, **joints**, **constraints**, and the **relative coordinate system**.

The next tutorial will reinforce these concepts by creating a more complex (but not quite biomechanical, yet 😉) model using these concepts, while introducing new things like collision detection and data extraction.
