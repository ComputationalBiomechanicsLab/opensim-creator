Graphics
--------

The :mod:`opynsim.graphics` module provides utilities for rasterizing
OPynSim's datastructures, images, lines, and shapes into a grid of pixels
(a bitmap/raster image). The utilities are available at different abstraction
levels, depending on how much control the caller needs over the rendering
process:

- **High-Level Rendering Functions**: Functions such as :func:`opynsim.graphics.render_model_in_state`
  are high-level functions that internally handle graphics context initialization, scene
  graph generation, rendering, packing the raster image into a Python-readable
  datastructure, and cleanup.
- **Low-Level Rendering Utilities**: TODO.


Examples
--------

Render to a PNG
~~~~~~~~~~~~~~~

This example uses `Pillow <https://python-pillow.github.io/>`_ to encode
the pixel data returned by :meth:`opynsim.graphics.Texture2D.pixels_rgba32` into a PNG file.

.. code:: python

    import opynsim
    import opynsim.graphics
    from PIL import Image  # from `Pillow` package

    # Create/import a `Model` + `ModelState`.
    model_specification = opynsim.example_specification_double_pendulum()
    model = opynsim.compile_specification(model_specification)
    model_state = model.initial_state()
    model.realize(model_state, opynsim.ModelStateStage.REPORT)  # usually required for rendering

    # Render the `Model` + `ModelState` to an `opynsim.graphics.Texture2D`.
    texture_2d = opynsim.graphics.render_model_in_state(model, model_state)

    # Read the pixels into a `PIL.Image` object.
    image = Image.fromarray(texture_2d.pixels_rgba32(), mode="RGBA")

    # Write the `PIL.Image` to disk as a PNG file.
    image.save("render_output.png")


Render to a Plot
~~~~~~~~~~~~~~~~

This example uses `Matplotlib <https://matplotlib.org/>`_ to composite the pixel data
returned by :meth:`opynsim.graphics.Texture2D.pixels_rgba32` into a line plot.

.. code:: python

    import opynsim
    import opynsim.graphics
    import numpy as np
    import matplotlib.pyplot as plt  # from `matplotlib` package
    from matplotlib.offsetbox import OffsetImage, AnnotationBbox

    # Create/import a `Model` + `ModelState`.
    model_specification = opynsim.example_specification_pendulum()
    model = opynsim.compile_specification(model_specification)
    model_state = model.initial_state()
    model.realize(model_state, opynsim.ModelStateStage.REPORT)  # usually required for rendering

    # Define data/image sample points
    x_min = -0.5*np.pi
    x_max =  0.5*np.pi
    data_xs = np.linspace(x_min, x_max, 100)

    # Use OPynSim to simulate and produce output values
    simulated_y_positions = []
    simulated_y_accelerations = []
    for x in data_xs:
        model.set_coordinate_value(model_state, "/jointset/pin/pin_coord_0", x)
        model.realize(model_state, opynsim.ModelStateStage.REPORT)
        simulated_y_positions.append(model.get_output_value(model_state, "/bodyset/head[position]")[1])
        simulated_y_accelerations.append(model.get_output_value(model_state, "/bodyset/head[linear_acceleration]")[1])

    # Use OPynSim to render the model + state at some positions in the plot
    num_images = 8
    image_xs = np.linspace(x_min, x_max, num_images)
    images = []
    for x in image_xs:
        model.set_coordinate_value(model_state, "/jointset/pin/pin_coord_0", x)
        model.realize(model_state, opynsim.ModelStateStage.REPORT)
        images.append(opynsim.graphics.render_model_in_state(model, model_state))

    # Create plot
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(data_xs, np.sin(data_xs + 0.5*np.pi), label='sin(x+0.5pi)')
    ax.plot(data_xs, simulated_y_positions,  "o", label='y (simulated)')
    ax.plot(data_xs, np.cos(data_xs + 0.5*np.pi), label='cos(x+0.5pi)')
    ax.plot(data_xs, simulated_y_accelerations, label='y_accel (simulated)')
    ax.set_title("Model Render Overlay at Peak")
    plt.legend()

    # Draw images as annotations over the plot
    for i in range(num_images):
        # Create a Matplotlib `OffsetImage` using the rendered pixel data
        offset_image = OffsetImage(images[i].pixels_rgba32(), zoom=0.2, alpha=1.0)

        # Wrap the `OffsetImage` into an `AnnotationBox` to anchor the image somewhere
        # in the plot data (here, on the first peak of sin(x)).
        ab = AnnotationBbox(
            offset_image,              # The rendered scene, wrapped in `OffsetImage`
            (image_xs[i], 1),
            #xybox=(-25, 50),           # Offset the image from the peak
            xycoords='data',           # Use plot coordinates for the anchor
            boxcoords="offset points",
            frameon=False,
            pad=0.0
        )
        ax.add_artist(ab)  # Add image

    # Show the figure in a window.
    #
    # This might require installing `python3-tk` on Linux.
    plt.show()

    # Alternatively, Save plot to PNG file
    # plt.savefig("plot.png", dpi=300)


API Reference
-------------

.. autofunction:: opynsim.graphics.render_model_in_state

.. automodule:: opynsim.graphics
   :members:
   :imported-members:
   :undoc-members:
   :show-inheritance:
