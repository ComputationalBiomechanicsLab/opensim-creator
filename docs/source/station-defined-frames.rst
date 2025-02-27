Station Defined Frames
======================

This tutorial focuses on the ``StationDefinedFrame`` component, merged in
`opensim-core/pull/3694`_ , available in OpenSim >= v4.3.1. Specifically,
this tutorial focuses on adding ``StationDefinedFrames`` via OpenSim
Creator. If you're more interested in the underlying implementation, or
its associated OpenSim API, then `StationDefinedFrame.h`_ provides a
technical (C++) description.


Overview
--------

Frames compose a right-handed set of three orthogonal axes (i.e. a 3D rotation) and
an origin point (i.e. a 3D translation). OpenSim has a ``Frame`` abstraction that it
uses when defining almost all spatial relationships. Joints join ``Frames`` bodies are
a ``Frame`` ; muscle path points are defined within ``Frames`` (as are markers); and
meshes, contact geometry, and wrap surfaces are attached to ``Frames`` - the list
goes on.

A ``Frame`` 's transform can be constant (e.g. ``Ground``), based on constraints (e.g. ``Body``),
or user-defined (e.g. ``PhysicalOffsetFrame``). User-defined ``Frame`` s are the primary
way that modellers define spatial relationships in a model.

.. admonition:: An Example
    
    An OpenSim model file might encode "``PhysicalOffsetFrame`` ``A`` is ``(0.5, 0, 0)``
    meters, and a 90 degree rotation along the Y axis, in the ``Frame`` of body ``B``"). You
    can see this for yourself by searching for ``PhysicalOffsetFrame`` in the source code of
    an existing ``.osim`` file.

``StationDefinedFrame`` s are user-defined ``Frame`` s that calculate their rotation
and translation from relationships between stations (rigid points) in the model. This
mirrors how anatomical joint/body frames are typically defined (e.g. ISB CITATION), making
them easier to work with in that context. Another advantage is that the ``StationDefinedFrame``
definition is compatible with algorithms that scale/warp/move points (e.g. TPS, as
described in :doc:`the-mesh-warper`).

.. _opensim-core/pull/3694: https://github.com/opensim-org/opensim-core/pull/3694
.. _StationDefinedFrame.h: https://github.com/opensim-org/opensim-core/blob/main/OpenSim/Simulation/Model/StationDefinedFrame.h