Tutorial 2: Make a Bouncing Block
=================================

In this tutorial, we will be making a bouncing block using OpenSim Creator:

.. figure:: _static/tut1_appearanceformatted.png
    :width: 60%

    TODO final solution image

In :ref:`tut1`, we created one of the most basic physical systems that can be modelled. That tutorial mostly focused on core OpenSim concepts. This tutorial is going to focus on reinforcing the same concepts to build more complex models - while bringing in new concepts like forces and collision detection. It will also introduce how to extract useful data from OpenSim models - a key byprodct of modelling.

This tutorial assumes you have already completed :ref:`tut1`. Therefore, it will skip some of the finer details. If you feel lost at any point, there should be partial solutions available along the way.


Step 1: Create the Foot
-----------------------

The most straightforward way to develop a new OpenSim model is to start at whichever body will be attached to ground (e.g. ``foot``) and work your way outwards to things that will be attached to that (e.g. ``leg``). Otherwise, you will spend a lot of time reconfiguring the topography of the scene, and probably spend even more time reorienting things in their new topography.

So, the first thing we need to add is the ``foot`` body. As explained in the previous tutorial, all bodies (which are frames) need to be attached to other frames in the scene and, ultimately, attached to ground. In our model, the ``foot`` will be a freely-moving element in the scene, so we need to attach it to the ground with a ``FreeJoint``. 

Using similar steps to the previous tutorial:

* Add a body called ``foot`` into the scene. It should have a mass of ``1 kg`` (the default) and be joined to ``ground`` with a ``FreeJoint`` called ``foot_to_ground``. Attach a ``Sphere`` geometry to it.
* Click the sphere and change its ``Appearance`` property such that it has a red color

You can then raise the foot above the ground slightly in one of two ways:

* **(as in the previous tutorial)** Set the ``foot_offset``'s ``translate`` property to ``(0.0, -0.5, 0.0)``. It's negative because ``foot_offset`` must be in the same location as ``ground``--it's attached to ground via a ``FreeJoint`` that has a default translation of ``(0, 0, 0)``--and we want ``foot`` to be above the offset frame. Logically, the ``foot_offset`` must therefore be *below* ``foot``, because ``translate`` is expressed relative to ``foot``.

* **(a better way, specific to** ``FreeJoint`` **)** Set the ``ty`` property of the ``foot_to_ground`` to ``0.5``. This translates the joint 0.5Y upwards.

These two approaches have equivalent side-effects, but the benefit of editing the ``FreeJoint``'s coordinate is that coordinates are displayed in the OpenSim GUI as sliders that users can later play with without having to edit specific parts of the model.

.. figure:: _static/tut2_added-foot.png
    :width: 60%

    Scene after adding ``foot`` (:download:`download model <_static/tut2_added-foot.osim>`)



Step 2: Add Contact Surfaces
----------------------------

If you simulate the model at this point, ``foot`` will just fall through the floor. The reason this happens is because the floor isn't a physical part of the scene and ``foot`` doesn't have a "size" or contact surface.

To add a floor (a ``ContactHalfSpace``) into the scene:

* Click the ``add contact geometry`` button, then ``ContactHalfSpace`
* Give the ``ContactHalfSpace`` the following properties:

.. figure:: _static/tut2_floor-properties.png

    Properties for the ``floor_contact`` component (a ``ContactHalfSpace``). Careful that ``orientation`` is set to ``-1.5707`` in Z. ``+1.5707`` behaves differently, because the contact surface is only "active" on one side.


To add a contact surface (a ``ContactSphere``) to ``foot``:

* Click the ``add contact geometry`` button, then click ``ContactSphere``
* Give the ``ContactSphere`` the following properties:

.. figure:: _static/tut2_footcontact-properties.png

    Properties for the ``foot_contact`` component (a ``ContactSphere``). The ``radius`` is set to match the ``Sphere`` decoration used on the ``foot``. It's also attached to ``foot`` so that it affects ``foot`` whenever contact occurs. You can edit the ``radius`` property if you can't see the sphere (it will probably be hidden slightly inside ``foot`` because their radii match).

The scene now has all the necessary contact geometry, but if you try to simulate the scene you will find that ``foot`` still just falls through the floor 😕. What's going on?

In OpenSim, ``ContactGeometry`` s express the *geometry* of the contact, but not the *force* that is experienced whenever contact between those geometries occurs. We need to separately add a ``HuntCrossleyForce`` into the model that expresses what happens when collision occurs. To do that:

* Click the ``add force/muscle`` button
* Click ``HuntCrossleyForce``
* Click ``add``
* In the properties editor, click ``add contact geometry`` and add ``floor_contact`` and ``foot_contact`` to the force

Simulating this model should show ``foot`` hit ``floor``, bounce a little, then stop. The ``HuntCrossleyForce``'s parameters dictate how stiff the contact is, dissipation, etc. - experiment with those properties to see what happens.

.. figure:: _static/tut2_collision-forces-added.png
    :width: 60%

    The model after adding ``floor_contact``, ``foot_contact`` and a ``HuntCrossleyForce`` that uses them. The ``foot`` sphere should fall onto the surface, bounce a little, then stop (:download:`download model <_static/tut2_added-contact-stuff.osim>`)


Step 3: Attach a Knee to the Foot
---------------------------------

The top "head" of our bouncing block is attached to the "foot" via a "knee" joint. We are going to build the model from the ``foot`` towards the "head" in a step-by-step manner. This lets us  view, reorient, and simulate the model at each step. The next step is to add a ``knee`` (body) attached by a ``knee_joint`` to the ``foot`` into the model.



