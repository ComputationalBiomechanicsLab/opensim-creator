Quickstart
==========

Import ``opynsim``
------------------

After :doc:`installing OPynSim <installation>`, it may be imported into Python
code like this:

.. code:: python

    import opynsim


Configuring OPynSim (Optional)
------------------------------

The ``opynsim`` :doc:`configuration <../api/configuration>` API contains utilities that globally
affect OPynSim's behavior. These are mostly for debugging and/or legacy support.

If you are working with OpenSim model files, and want a similar development
experience (in terms of logging and loading things) to the ``opensim`` API,
then you can do something like this after importing ``opynsim``:

.. code:: python

    import opynsim
    import pathlib
    import logging

    # Make OPynSim's logging noisier
    opynsim.set_logging_level(logging.DEBUG)

    # Add `/path/to/geometry` as an OpenSim geometry directory.
    opynsim.add_opensim_geometry_directory(pathlib.Path("/path/to/geometry/"))

See :func:`opynsim.set_logging_level` and :func:`opynsim.add_opensim_geometry_directory` for
more information.

Import an ``osim`` File
-----------------------

:class:`opynsim.ModelSpecification` is a central part of the ``opynsim``
API. It's a high-level model specification object that Python code can
manipulate in order to build and customize the resulting :class:`opynsim.Model`\'s
behavior.

The OPynSim :doc:`modelling <../api/modelling>` API provides :func:`opynsim.import_osim_file` function,
which imports an ``.osim`` file into a :class:`opynsim.ModelSpecification`:

.. code:: python

    import opynsim
    import pathlib

    # Import an `.osim` file as an `opynsim.ModelSpecification`
    model_specification = opynsim.import_osim_file("arm26.osim")

    # `pathlib.Path`s are also supported
    model_specification2 = opynsim.import_osim_file(pathlib.Path("/some/path/to/arm26.osim"))

.. note:: These documentation pages mostly use example specification generators.

    The remainder of this quickstart guide, and many of the documentation pages, use example
    specifications generated from ``opynsim.example_specification_*`` methods, rather than
    :func:`opynsim.import_osim_file` because they don't require external data and can therefore
    be copied+pasted more easily.

    You can always exchange an example for your own :class:`opynsim.ModelSpecification`, or
    one loaded from an ``.osim`` file.

Compile a Specification into a Model
------------------------------------

Once a :class:`opynsim.ModelSpecification` has been prepared, it can be used
to build a :class:`opynsim.Model`, which represents a read-only physics model.

:func:`opynsim.compile_specification` is how you do this:

.. code:: python

    import opynsim

    model_specification = opynsim.example_specification_double_pendulum()

    # ... if necessary, edit the `ModelSpecification`, and then...

    model = opynsim.compile_specification(model_specification)


Create and Realize an Initial State of the Model
------------------------------------------------

:class:`opynsim.Model`\s are capable of producing an initial
:class:`opynsim.ModelState`. This is the state of the model that you
would see if loading the model in a visualizer without loading
states externally from (e.g.) a motion file.

:meth:`opynsim.Model.initial_state` creates a new initial :class:`opynsim.ModelState`:

.. code:: python

    import opynsim

    model_specification = opynsim.example_specification_double_pendulum()
    model = opynsim.compile_specification(model_specification)

    state = model.initial_state()

Once you have a :class:`opynsim.ModelState`, you can the manipulate and inspect it
according to your modelling requirements.


Visualize the Model in a State
------------------------------

The OPynSim :doc:`../api/ui` API provides various utilities for visualizing and
interacting with OPynSim's datastructures.

The API includes high-level functions, such as :func:`opynsim.ui.show_model_in_state`,
which can be used to visualize a model in a single state. The state should be realized
to :attr:`opynsim.ModelStateStage.REPORT` to ensure that all the state variables the
renderer reads are fully realized:

.. code:: python

    import opynsim
    import opynsim.ui

    model_specification = opynsim.example_specification_double_pendulum()
    model = opynsim.compile_specification(model_specification)
    state = model.initial_state()
    model.realize(state, opynsim.ModelStateStage.REPORT)  # required for rendering

    opynsim.ui.show_model_in_state(model, state)


Render Visualization to an Image File
-------------------------------------

The OPynSim :doc:`../api/graphics` API provides lower-level utilities for rendering
OPynSim's datastructures to an image (:class:`opynsim.graphics.Texture2D`). This can be useful
for automating tasks like creating custom plots or saving images and videos.

The API includes high-level functions, such as :func:`opynsim.graphics.render_model_in_state`,
which returns a :class:`opynsim.graphics.Texture2D`, which you can then use in your Python
code. The example below renders an :class:`opynsim.Model` + :class:`opynsim.ModelState` to
a texture and then uses `Pillow <https://python-pillow.github.io/>`_ to write its raw pixel
data into a PNG file:

.. code:: python

    import opynsim
    import opynsim.graphics
    from PIL import Image  # pip install Pillow

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
