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

Compile a Specification into a Model
------------------------------------

Once a :class:`opynsim.ModelSpecification` has been prepared, it can be used
to build a :class:`opynsim.Model`, which represents a read-only physics model.

:func:`opynsim.compile_specification` is how you do this:

.. code:: python

    import opynsim

    model_specification = opynsim.import_osim_file("arm26.osim")

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

    model_specification = opynsim.import_osim_file("arm26.osim")
    model = opynsim.compile_specification(model_specification)

    state = model.initial_state()

Once you have a :class:`opynsim.ModelState`, you can the manipulate and inspect it
according to your modelling requirements.


Render the Model in a State
----------------------------

.. warning::

    TODO: not yet implemented

The OPynSim :doc:`graphics <../api/graphics>` API provides various utilities for
rendering things to a window or image. The API was ported from `OpenSim Creator <https://opensimcreator.com>`_
so that Python scripts may have similar features and render identically to it.

In order to render anything, OPynSim needs an active graphics context (e.g.
an OpenGL context) to render with. This is achieved by creating an
:class:`opynsim.Application` before using any :mod:`opynsim.graphics` API. You
can then render things to images:

.. code:: python

    import opynsim

    model_specification = opynsim.import_osim_file("arm26.osim")
    model = opynsim.compile_specification(model_specification)
    state = model.initial_state()

    with opynsim.Application():
        camera = opynsim.graphics.Camera()
        image = opynsim.graphics.render_model_in_state(model, state, camera=camera)

See :doc:`../api/graphics` for an explanation of the graphics API.

TODO: Show Model+State in an Interactive UI
-------------------------------------------

TODO: Create a custom UI
------------------------