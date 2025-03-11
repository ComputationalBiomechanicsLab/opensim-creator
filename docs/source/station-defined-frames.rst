Station Defined Frames
======================

This tutorial focuses on the ``StationDefinedFrame`` component, which was
merged into OpenSim in `opensim-core/pull/3694`_  and should be available in
OpenSim >= v4.3.1. Specifically, this tutorial focuses on adding
``StationDefinedFrames`` via OpenSim Creator. If you're more interested
in the underlying implementation, or its associated OpenSim API,
then `StationDefinedFrame.h`_ provides a technical (C++) description.


Overview
--------

Frames are defined to compose a right-handed set of three orthogonal axes (an
orientation) and an origin point (a translation). OpenSim has a ``Frame`` abstraction
that it uses when defining almost all spatial relationships. Joints
join ``Frames`` bodies are a ``Frame`` ; muscle path points are defined within
``Frames`` (as are markers); and meshes, contact geometry, and wrap surfaces are
attached to (and defined in) ``Frames``.

A ``Frame`` 's transform can be constant (e.g. ``Ground``), based on constraints (e.g. ``Body``),
or user-defined (e.g. ``PhysicalOffsetFrame``). User-defined ``Frame`` s are the primary
way that modellers define spatial relationships in an OpenSim model.

.. admonition:: ``PhysicalOffsetFrame`` Example
    
    An OpenSim model might contain a ``PinJoint`` definition that attaches to
    a ``PhysicalOffsetFrame`` in order to customize the joint center. The offset frame's
    osim XML may look something like this:

    .. code-block:: XML

        <PhysicalOffsetFrame name="body_offset">
            <socket_parent>/bodyset/some_body</socket_parent>
            <translation>0 0.8 0</translation>
            <orientation>1.5707 0 0</orientation>
        </PhysicalOffsetFrame>

    Where ``socket_parent`` indicates which frame ``body_offset`` is defined in,
    ``translation`` defines its translation within the parent (here, ``some_body``),
    and ``orientation`` defines its orientation with respect to that
    parent. OpenSim uses this information like this at runtime to figure out
    where (e.g.) bodies and muscle points are.

``StationDefinedFrame`` s are user-defined ``Frame`` s that derive their transform
from relationships between stations (body-fixed points) in the model. This more
closely mirrors how anatomical joint/body frames are formally defined
(e.g. `Grood et. al.`_, `Wu et. al.`_). It's also compatible with algorithms
that operate on points (e.g. TPS warping, see :doc:`the-mesh-warper`). The
mathematical relationship between stations in the model and the ``StationDefinedFrame``
are shown in :numref:`sdf-maths-figure`.

.. _sdf-maths-figure:
.. figure:: _static/station-defined-frames/sdf-maths.svg
    :width: 60%

    The relationship between stations and a ``StationDefinedFrame``. :math:`\vec{a}`,
    :math:`\vec{b}`, :math:`\vec{c}`, and :math:`\vec{d}` are four stations in the
    model that must be attached---either directly, or indirectly (e.g. via a
    ``PhysicalOffsetFrame``)---to the same body. The ``StationDefinedFrame``
    implementation uses the stations to derive :math:`f(\vec{v})`, its transform
    function. The origin station, :math:`\vec{o}`, may be coincident with one of
    the other stations.

Practically speaking, this means is that ``StationDefinedFrame`` s let modellers
define frames by choosing 3 or 4 stations (landmarks) on each body. Once that
relationship is established, the resulting frame is automatically recalculated
whenever the the stations moved (e.g. due to scaling, warping, shear, etc.).


A Simple Example
----------------

OpenSim Creator includes example models that use ``StationDefinedFrame``:

- ``StationDefinedFrame.osim`` : A simple example that contains four stations defined
  in one body with a ``StationDefinedFrame`` that's defined by the stations.

- ``StationDefinedFrame_Advanced.osim``: A more advanced example that contains multiple
  ``StationDefinedFrame`` s that are chained and use stations attached via
  ``PhysicalOffsetFrame`` s.

This example walks through how something like ``StationDefinedFrame.osim`` can be built from
scratch, so that you can get an idea of how the mathematics (:numref:`sdf-maths-figure`) is exposed via
OpenSim's component system. The next section, `A Practical Example`_, then shows how ``StationDefinedFrame`` s
can be added to an existing model.


Make a One-Body Model
~~~~~~~~~~~~~~~~~~~~~

1. Create a blank OpenSim model (e.g. from the splash screen or main menu).
2. Add a body to the model (as described in :ref:`add-body-with-weldjoint`), attach a brick
   geometry to the body, so it's easier to visualize.
3. You should end up with something like :numref:`blank-model-single-body-with-brick-figure`.

.. _blank-model-single-body-with-brick-figure:
.. figure:: _static/station-defined-frames/model-with-one-body.jpg
    :width: 60%

    ``TODO`` model containing one body with a brick attached.


Add Stations to the Body
~~~~~~~~~~~~~~~~~~~~~~~~

``TODO``

.. figure:: _static/station-defined-frames/add-station.jpg
    :width: 60%

    ``TODO``

.. figure:: _static/station-defined-frames/stations-added.jpg
    :width: 60%

    ``TODO``

Add a ``StationDefinedFrame``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. figure:: _static/station-defined-frames/add-sdf.jpg
    :width: 60%

    ``TODO``

.. figure:: _static/station-defined-frames/sdf-added.jpg
    :width: 60%

    ``TODO``


A Practical Example
-------------------

This example walks through how ``StationDefinedFrame`` s can be practically used in OpenSim
Creator. It will focus on handling these common questions that arise when adding
``StationDefinedFrame`` s to larger/existing OpenSim Models:

- Where should ``StationDefinedFrame`` s be stored in the model hierarchy?
- How should ``Joint`` s be created between ``StationDefinedFrame`` s (and other ``Frame`` s)?
- How can existing ``Joint`` s be updated to use ``StationDefinedFrame`` s?

``TODO``: write up the answers to these questions!


.. _opensim-core/pull/3694: https://github.com/opensim-org/opensim-core/pull/3694
.. _StationDefinedFrame.h: https://github.com/opensim-org/opensim-core/blob/main/OpenSim/Simulation/Model/StationDefinedFrame.h
.. _Grood et. al.:  https://doi.org/10.1115/1.3138397
.. _Wu et. al.: https://doi.org/10.1016/0021-9290(95)00017-C
