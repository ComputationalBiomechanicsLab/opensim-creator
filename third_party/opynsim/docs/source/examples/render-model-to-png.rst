Render Model to PNG
===================

This example uses `Pillow <https://python-pillow.github.io/>`_ to encode
the pixel data returned by :meth:`opynsim.graphics.Texture2D.pixels_rgba32`
into a PNG file that is then saved to ``render_output.png``.

.. code:: python

    import opynsim as opyn
    import opynsim.examples
    import opynsim.graphics
    from PIL import Image  # from `Pillow` package

    # Create/import a `Model` + `ModelState`.
    model = opyn.examples.double_pendulum_model()
    model_state = model.initial_state()
    model.realize(model_state, opyn.STAGE_REPORT)  # usually required for rendering

    # Render the `Model` + `ModelState` to an `opynsim.graphics.Texture2D`.
    texture_2d = opyn.graphics.render_model_in_state(model, model_state)

    # Read the pixels into a `PIL.Image` object.
    image = Image.fromarray(texture_2d.pixels_rgba32(), mode="RGBA")

    # Write the `PIL.Image` to disk as a PNG file.
    image.save("render_output.png")
