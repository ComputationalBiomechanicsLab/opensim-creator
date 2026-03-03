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

This example uses `Pillow <https://python-pillow.github.io/>`_ (``pip install Pillow``) to encode
the pixel data returned by :meth:`opynsim.graphics.Texture2D.pixels_rgba32` into a PNG file.

.. code:: python

    import opynsim
    import opynsim.graphics
    from PIL import Image

    # Create/import a `Model` + `ModelState`.
    model_specification = opynsim.import_osim_file("arm26.osim")
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
    import matplotlib.pyplot as plt
    from matplotlib.offsetbox import OffsetImage, AnnotationBbox

    # Create/import a `Model` + `ModelState`.
    model_specification = opynsim.import_osim_file("arm26.osim")
    model = opynsim.compile_specification(model_specification)
    model_state = model.initial_state()
    model.realize(model_state, opynsim.ModelStateStage.REPORT)  # usually required for rendering

    # Render the `Model` + `ModelState` to an `opynsim.graphics.Texture2D`.
    texture_2d = opynsim.graphics.render_model_in_state(model, model_state)

    # Create a Matplotlib `OffsetImage` using the rendered pixel data
    offset_image = OffsetImage(texture_2d.pixels_rgba32(), zoom=0.2)

    # Wrap the `OffsetImage` into an `AnnotationBox` to anchor the image somewhere
    # in the plot data (here, on the first peak of sin(x)).
    ab = AnnotationBbox(
        offset_image,               # The rendered scene, wrapped in `OffsetImage`
        (np.pi/2, 1),               # First peak of sin(x)
        xybox=(50, 50),             # Offset the image 50 points away from the peak
        xycoords='data',            # Use plot coordinates for the anchor
        boxcoords="offset points",
        pad=0.5,                    # Padding around the image
        arrowprops=dict(arrowstyle="->", connectionstyle="angle,angleA=0,angleB=90,rad=3")
    )

    # Create plot
    fig, ax = plt.subplots(figsize=(10, 6))
    x = np.linspace(0, 10, 100)
    ax.plot(x, np.sin(x), label='Sine Wave', color='blue')
    ax.set_title("Model Render Overlay at Peak")
    ax.set_ylim(-1.5, 2.5) # Make some room for the image
    ax.add_artist(ab)  # Add image
    plt.legend()

    # Save plot to PNG file
    plt.savefig("plot.png", dpi=300)


API Reference
-------------

.. autofunction:: opynsim.graphics.render_model_in_state

.. automodule:: opynsim.graphics
   :members:
   :imported-members:
   :undoc-members:
   :show-inheritance:
