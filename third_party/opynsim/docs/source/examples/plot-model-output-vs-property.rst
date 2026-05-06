Plot Model Output vs. Property
==============================

.. code:: python

    import opynsim as opyn

    # 1. Read a `ModelSpecification` from an `.osim` file.
    model_specification = opyn.read_osim("some.osim")

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
