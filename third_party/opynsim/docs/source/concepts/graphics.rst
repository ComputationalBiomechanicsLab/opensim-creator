Graphics
========

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
