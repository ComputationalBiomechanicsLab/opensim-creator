Quickstart
==========

Import ``opynsim``
------------------

After :doc:`installing OPynSim <installation>`, it may be imported into Python
code like this:

.. code:: python

    import opynsim as opyn

This convention allows access to OPynSim features with a short, recognizable, prefix (``opyn.``),
which we will use in examples.

Import an ``osim`` File
-----------------------

:class:`opynsim.ModelSpecification` is a central part of the ``opynsim``
API. It's a high-level model specification object that Python code can
manipulate in order to build and customize the resulting :class:`opynsim.Model`\'s
behavior.

One way to create a :class:`opynsim.ModelSpecification` is to import it from an
``.osim`` file, which enables importing complex specifications built using visual
tools like `OpenSim Creator <https://opensimcreator.com>`_.  :func:`opynsim.import_osim_file`
is one way to do this:

.. code:: python

    import opynsim as opyn
    import opynsim.config
    import pathlib

    # (optional): Add a geometry directory to the search path, so that
    # OPynSim can find shared mesh files.
    opyn.config.append_search_path("/some/geometry/directory")

    # Import an `.osim` file as an `opynsim.ModelSpecification`
    model_specification = opyn.import_osim_file("arm26.osim")

    # `pathlib.Path`s are also supported
    model_specification2 = opyn.import_osim_file(pathlib.Path("/some/path/to/arm26.osim"))

.. note::

    The remainder of the documentation uses generators (e.g. :func:`opynsim.example_specification_pendulum`) to
    create :class:`opynsim.ModelSpecification`\s.

    This is only because it's easier to copy + paste Python code that uses generated
    examples. However, you can always exchange an example :class:`opynsim.ModelSpecification` for
    one loaded via :func:`opynsim.import_osim_file`.


Compile a Specification into a Model
------------------------------------

Once a :class:`opynsim.ModelSpecification` has been prepared, it can be used
to build (compile) a :class:`opynsim.Model`, which represents a read-only physics model.

:meth:`opynsim.ModelSpecification.compile` is one way to do this:

.. code:: python

    import opynsim as opyn

    # alternatively: model_specification = opyn.import_osim_file("your.osim")
    model_specification = opyn.example_specification_double_pendulum()

    # ... if necessary, edit the `ModelSpecification`, and then...

    model = model_specification.compile()  # builds the model from the specification


Create and Realize an Initial State of the Model
------------------------------------------------

:class:`opynsim.Model`\s are capable of producing an initial
:class:`opynsim.ModelState`. This is the state of the model that you
would see if loading the model in a visualizer without loading
states externally (e.g. from a motion file).

:meth:`opynsim.Model.initial_state` creates a new initial :class:`opynsim.ModelState`:

.. code:: python

    import opynsim as opyn

    model_specification = opyn.example_specification_double_pendulum()
    model = model_specification.compile()

    state = model.initial_state()  # produces an initial state of the model

Once you have a :class:`opynsim.ModelState`, you can the manipulate and inspect it
according to your modelling requirements.


Visualize the Model in a State
------------------------------

The OPynSim :doc:`../api/ui` API provides various utilities for visualizing and
interacting with OPynSim's datastructures.

The API includes high-level functions, such as :func:`opynsim.ui.show_model_in_state`,
which can be used to visualize a model in a single state. The state should be realized
to :attr:`opynsim.STAGE_REPORT` to ensure that all the state variables the
renderer reads are available:

.. code:: python

    import opynsim as opyn
    import opynsim.ui

    model_specification = opyn.example_specification_double_pendulum()
    model = model_specification.compile()
    state = model.initial_state()

    model.realize(state, opyn.STAGE_REPORT)    # required for rendering
    opyn.ui.show_model_in_state(model, state)  # shows `model` in `state`


Render Visualization to an Image File
-------------------------------------

The OPynSim :doc:`../api/graphics` API provides lower-level utilities for rendering
OPynSim's datastructures to an image (:class:`opynsim.graphics.Texture2D`). This can be useful
for automating tasks like creating custom plots or creating images and videos of models.

The API includes high-level functions, such as :func:`opynsim.graphics.render_model_in_state`,
which returns a :class:`opynsim.graphics.Texture2D`, which you can then use in your Python
code. The example below renders an :class:`opynsim.Model` + :class:`opynsim.ModelState` to
a texture and then uses `Pillow <https://python-pillow.github.io/>`_ to write its raw pixel
data into a PNG file:

.. code:: python

    import opynsim as opyn
    import opynsim.graphics
    from PIL import Image  # from `Pillow` package

    # Create/import a `Model` + `ModelState`.
    model_specification = opyn.example_specification_double_pendulum()
    model = model_specification.compile()
    model_state = model.initial_state()
    model.realize(model_state, opyn.STAGE_REPORT)  # required for rendering

    # Render the `Model` + `ModelState` to a `Texture2D` (image).
    texture_2d = opyn.graphics.render_model_in_state(model, model_state)

    # Copy the texture's pixels into a `PIL.Image` object.
    image = Image.fromarray(texture_2d.pixels_rgba32(), mode="RGBA")

    # Save the `PIL.Image` as a PNG file.
    image.save("render_output.png")
