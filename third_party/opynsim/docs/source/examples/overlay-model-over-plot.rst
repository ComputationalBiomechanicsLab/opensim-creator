Overlay Model Over Plot
=======================

This example uses `Matplotlib <https://matplotlib.org/>`_ to composite the pixel data
returned by :meth:`opynsim.graphics.Texture2D.pixels_rgba32` over a line plot, so that
raw data plots an be annotated with helpful visualizations.

.. code:: python

    import opynsim as opyn
    import opynsim.examples
    import opynsim.graphics
    import numpy as np
    import matplotlib.pyplot as plt  # from `matplotlib` package
    from matplotlib.offsetbox import OffsetImage, AnnotationBbox

    # Create/import a `Model` + `ModelState`.
    model = opyn.examples.pendulum_model()
    model_state = model.initial_state()
    model.realize(model_state, opyn.STAGE_REPORT)  # usually required for rendering

    # Define data/image sample points
    x_min = -0.5*np.pi
    x_max =  0.5*np.pi
    data_xs = np.linspace(x_min, x_max, 100)

    # Use OPynSim to simulate and produce output values
    simulated_y_positions = []
    simulated_y_accelerations = []
    for x in data_xs:
        model.set_coordinate_value(model_state, "/jointset/pin/pin_coord_0", x)
        model.realize(model_state, opyn.STAGE_REPORT)
        simulated_y_positions.append(model.get_output_value(model_state, "/bodyset/head[position]")[1])
        simulated_y_accelerations.append(model.get_output_value(model_state, "/bodyset/head[linear_acceleration]")[1])

    # Use OPynSim to render the model + state at some (sparser) positions in the plot
    num_images = 8
    image_xs = np.linspace(x_min, x_max, num_images)
    images = []
    for x in image_xs:
        model.set_coordinate_value(model_state, "/jointset/pin/pin_coord_0", x)
        model.realize(model_state, opyn.STAGE_REPORT)
        images.append(opyn.graphics.render_model_in_state(model, model_state))

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
