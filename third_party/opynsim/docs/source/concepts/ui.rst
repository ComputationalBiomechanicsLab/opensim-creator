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
