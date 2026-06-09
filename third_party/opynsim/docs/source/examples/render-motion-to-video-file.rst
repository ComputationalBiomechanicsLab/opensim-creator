Render Motion to Video File
===========================

This example uses `Imageio <https://github.com/imageio/imageio>`_ (installed
both ``imageio`` and ``imageio-ffmpeg``) to combine frames rendered by
the :mod:`opynsim.graphics` module into a viewable ``.mp4`` file.

.. code:: python

    import opynsim as opyn
    import opynsim.graphics
    import imageio  # from `imageio` and `imageio-ffmpeg` packages

    # Load + compile an `opynsim.Model` from an `osim` file.
    model = opyn.read_osim("some.osim").compile()

    # Load an associated motion from an `sto file` into an `opynsim.DataFrame`.
    sto = opyn.read_sto("some.sto")

    # Create `opynsim.ModelStates` of the `opynsim.Model` from the data frame.
    states = model.states_from_data_frame(sto)

    # Render each state as a frame in `output.mp4`.
    with imageio.get_writer("output.mp4", fps=60) as writer:
        for state in states:
            model.realize(state, opyn.STAGE_REPORT) # Required for rendering
            texture_2d = opyn.graphics.render_model_in_state(model, state)
            pixels_ndarray = render.pixels_rgb24()
            writer.append_data(pixels_ndarray)
