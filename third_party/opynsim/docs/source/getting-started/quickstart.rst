.. currentmodule:: opynsim

Quickstart
==========

Import ``opynsim``
------------------

After :doc:`installing OPynSim <installation>`, it may be imported into Python
code like this:

.. code:: python

    import opynsim as opyn

This convention allows access to the :mod:`opynsim` Python module with a short, recognizable,
prefix (``opyn.``), which we use extensively in this documentation.


Read an ``osim`` File
---------------------

:class:`ModelSpecification` is a central part of the ``opynsim``
API. It's a high-level model specification object that can be used
to build and customize the resulting :class:`Model`\'s behavior.

One way to create a :class:`ModelSpecification` is to read it from an
``.osim`` file using :func:`read_osim`. This means OPynSim can read complex specifications built
using visual tools like `OpenSim Creator <https://opensimcreator.com>`_.

.. code-block:: python
    :emphasize-lines: 10

    import opynsim as opyn
    import opynsim.config
    import pathlib

    # (optional): Add a geometry directory to the search path, so that
    #             OPynSim can find shared mesh files.
    opyn.config.append_search_path("/some/geometry/directory")

    # Read an `.osim` file into a `ModelSpecification`.
    model_specification = opyn.read_osim("arm26.osim")

    # `pathlib.Path`s are also supported.
    model_specification2 = opyn.read_osim(pathlib.Path("/some/path/to/arm26.osim"))

.. note::

    The remainder of the documentation uses generators from the :mod:`opynsim.examples`
    module (e.g. :func:`examples.pendulum_specification`) to
    create :class:`ModelSpecification`\s.

    This is because it's easier to copy + paste Python code that uses generated
    examples. However, you can always exchange an example :class:`ModelSpecification` for
    one loaded via :func:`read_osim`.


Compile a Specification into a Model
------------------------------------

Once a :class:`ModelSpecification` has been prepared, it can be used
to compile a :class:`Model` using the :meth:`ModelSpecification.compile`
method:

.. code-block:: python
    :emphasize-lines: 9

    import opynsim as opyn
    import opynsim.examples

    # alternatively: model_specification = opyn.read_osim("your.osim")
    model_specification = opyn.examples.double_pendulum_specification()

    # ... if necessary, edit the `ModelSpecification`, and then...

    model = model_specification.compile()  # builds the model from the specification


Create and Realize an Initial State of the Model
------------------------------------------------

:meth:`Model.initial_state` creates an initial :class:`ModelState`
for a  model. This is the state of the model that you would see if loading
it in a visualizer without loading states externally:

.. code-block:: python
    :emphasize-lines: 7

    import opynsim as opyn
    import opynsim.examples

    model_specification = opyn.examples.double_pendulum_specification()
    model = model_specification.compile()

    model_state = model.initial_state()              # Produce an initial state of the model.
    model.realize(model_state, opyn.STAGE_DYNAMICS)  # Realize the state to a specific simulation stage.

Once you have a :class:`ModelState`, you can the manipulate and inspect it
according to your modelling requirements. The above example uses :meth:`Model.realize`
to realize ``state`` to a later :class:`ModelStateStage`, which can be
necessary to read certain computed outputs from the state.

A shorthand version of the above would be:

.. code-block:: python
    :emphasize-lines: 5

    import opynsim as opyn
    import opynsim.examples

    model = opyn.examples.double_pendulum_model()  # or `*_specification().compile()`
    model_state = model.initial_state(realized_to=opyn.STAGE_DYNAMICS)


Visualize the Model in a State
------------------------------

OPynSim supports :doc:`../concepts/ui`, which can be useful for
visualizing and interacting with its datastructures.

The :mod:`opynsim.ui` module provides high-level functions, such as
:func:`ui.show_model_in_state`, which visualizes a :class:`Model` in a
single :class:`ModelState`. The state should be realized to
:attr:`ModelStateStage.REPORT` to ensure the state contains all necessary
information the UI may read:

.. code-block:: python
    :emphasize-lines: 11

    import opynsim as opyn
    import opynsim.examples
    import opynsim.ui

    model = opyn.examples.double_pendulum_model()
    model_state = model.initial_state(
        realized_to=opyn.STAGE_REPORT  # Required for rendering/UI
    )

    # Shows `model` in `state` in an interactive window.
    opyn.ui.show_model_in_state(model, model_state)


Render Visualization to an Image File
-------------------------------------

The OPynSim :doc:`../concepts/graphics` API provides utilities for rendering OPynSim's
datastructures to images (:class:`graphics.Texture2D`). This can be useful
for automating tasks like creating custom plots or creating images/videos of models.

The API includes high-level functions, such as :func:`graphics.render_model_in_state`,
which returns a :class:`graphics.Texture2D` - a class that stores rendered pixel data. The
example below renders an :class:`Model` + :class:`ModelState` to a texture and then
uses `Pillow <https://python-pillow.github.io/>`_ to write the pixel data to a PNG file:

.. code-block:: python
    :emphasize-lines: 10-14

    import opynsim as opyn
    import opynsim.examples
    import opynsim.graphics
    from PIL import Image  # from `Pillow` package

    # Create/import a `Model` + `ModelState`.
    model = opyn.examples.double_pendulum_model()
    model_state = model.initial_state(realized_to=opyn.STAGE_REPORT)

    # Render the `Model` + `ModelState` to a `Texture2D` (image).
    texture_2d = opyn.graphics.render_model_in_state(model, model_state)

    # Copy the texture's pixels into a `PIL.Image` object.
    image = Image.fromarray(texture_2d.pixels_rgba32(), mode="RGBA")

    # Save the `PIL.Image` as a PNG file.
    image.save("render_output.png")
