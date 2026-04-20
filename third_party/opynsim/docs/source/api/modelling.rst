Modelling
=========

Modelling is a central part of of the OPynSim API. At a high-level, the API is designed
around three classes with distinct roles:

- :class:`opynsim.ModelSpecification`: A high-level specification of the
  model. This is what Python code manipulates before calling :meth:`opynsim.ModelSpecification.compile`
  to yield...
- :class:`opynsim.Model`: A compiled read-only physics model, which can
  create/read/manipulate...
- :class:`opynsim.ModelState`: A single state of an :class:`opynsim.Model`, which can
  be modified by Python code to modify the state of the model.

When modelling with OPynSim, it's important to understand this relationship, because the rest
of the modelling API is built around it.


Typical Modelling Workflow
--------------------------

Typical modelling workflows follow some combination of these steps:

1. Import or create a :class:`opynsim.ModelSpecification`.
2. If required, modify the  :class:`opynsim.ModelSpecification`.
3. Compile the :class:`opynsim.ModelSpecification` to a :class:`opynsim.Model`.
4. Generate an initial :class:`opynsim.ModelState` with :meth:`opynsim.Model.initial_state`.
5. If required, modify the :class:`opynsim.ModelState`.
6. Use, or extract data from, the resulting :class:`opynsim.Model` + :class:`opynsim.ModelState` pair.

Below is an example workflow that exercises those steps. The :doc:`/tutorials-and-examples/examples` page is a good place to
find further inspiration. Use API reference (below) to learn about the API in more detail.

.. code:: python

    import opynsim as opyn

    # 1. Import a `ModelSpecification`.
    model_specification = opyn.import_osim_file("some.osim")

    force_wrt_elbow_per_tendon_length = {}
    for i in range(16):
        tendon_length = 0.005 * i/15

        # 2. If required, modify the `ModelSpecification`.
        model_specification.set_property("/forceset/muscle/tendon_length", tendon_length)

        # 3. Compile the specification into a `Model`.
        model = model_specification.compile()

        # 4. Generate an initial `ModelState` for the `Model`.
        state = model.initial_state()

        datapoints[tendon_length] = []
        for j in range(10):
            # 5. If required, modify the `ModelState`.
            model.set_coordinate_value(state, "/jointset/elbow/x", 0.1 * j/10)

            # 6. Extract data from the resulting `Model` + `ModelState` pair.
            datapoints[tendon_length].append(model.get_output_value("/forceset/muscle/activation"))

    # (... and then, use, plot, etc. the aggregated data...)


API Reference
-------------

.. autofunction:: opynsim.compile_specification
.. autofunction:: opynsim.import_osim_file
.. autofunction:: opynsim.example_specification_pendulum
.. autofunction:: opynsim.example_specification_double_pendulum

.. automodule:: opynsim
   :members:
   :imported-members:
   :undoc-members:
   :show-inheritance:

.. autodata:: opynsim.STAGE_TOPOLOGY
.. autodata:: opynsim.STAGE_MODEL
.. autodata:: opynsim.STAGE_INSTANCE
.. autodata:: opynsim.STAGE_TIME
.. autodata:: opynsim.STAGE_POSITION
.. autodata:: opynsim.STAGE_VELOCITY
.. autodata:: opynsim.STAGE_DYNAMICS
.. autodata:: opynsim.STAGE_ACCELERATION
.. autodata:: opynsim.STAGE_REPORT