User Interfaces
===============

The :mod:`opynsim.ui` module provides utilities for creating and displaying
interactive user interfaces, including:

- **High-Level Visualization Functions**: Functions prefixed with ``show_`` are
  typically high-level functions that internally handle initialization, maintaining
  the GUI main loop, and cleanup for the caller. They are similar to other
  Python APIs, like ``matplotlib.pyplot.show`` and ``pyvista.Plotter.show``.
- **Mid-Level Visualization Builders**: TODO.

In effect, :mod:`opynsim.ui` combines the rendering capabilities of the
:mod:`opynsim.graphics` module with an application framework that manages
interaction. The implementation was ported from the
`OpenSim Creator <https://www.opensimcreator.com>`_, which is compatible
with OPynSim's data files, but doesn't support scripting.

Example
-------

.. code:: python

    import opynsim
    import opynsim.ui

    # Create/import a `Model` + `ModelState`.
    model_specification = opynsim.example_specification_double_pendulum()
    model = opynsim.compile_specification(model_specification)
    model_state = model.initial_state()
    model.realize(model_state, opynsim.ModelStateStage.REPORT)  # usually required for rendering

    # Show them in an interactive window.
    opynsim.ui.show_model_in_state(model, model_state)


API Reference
-------------

.. autofunction:: opynsim.ui.show_hello_ui
.. autofunction:: opynsim.ui.show_model_in_state
