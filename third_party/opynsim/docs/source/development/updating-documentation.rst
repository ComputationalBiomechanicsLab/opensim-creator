Updating Documentation
======================

This page is work in progress, but tl;dr:

- Setup a python virtual environment the usual way (i.e. run ``python scripts/setup_venv.py`` from
  the top-level ``opynsim`` project directory).
- If you just want to build the documentation, activate the virtual environment then use
  ``sphinx-build`` to build it (google)
- Alternatively, if you want to develop the documentation, the recommended development environment
  for doing so is to use `Visual Studio Code`_ with the `Esbonio Plugin`_. The ``opynsim`` project
  already has a suitable ``pyproject.toml`` file in ``docs/`` that configures the plugin, provided
  the local virtual environment has been set up.

.. _Visual Studio Code: https://code.visualstudio.com/
.. _Esbonio Plugin: https://docs.esbon.io
