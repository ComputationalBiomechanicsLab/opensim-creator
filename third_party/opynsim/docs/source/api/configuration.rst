Configuration
=============

The ``opynsim`` API contains utilities that configure the global state of OPynSim. These
are sometimes necessary for debugging or implementing legacy compatibility.

Logging
-------

The most common thing to globally configure is  ``opynsim``\'s log level, which
can be achieved with :func:`opynsim.set_log_level`:

.. code:: python

    import opynsim as opyn
    import logging

    # Make OPynSim's logging more verbose (default is `logging.WARN`).
    opyn.set_log_level(logging.DEBUG)

    # OPynSim's logger is compatible with Python's logging API. For example:
    logging.basicConfig(
        level=logging.INFO,
        format='%(levelname)s: %(message)s',
        handlers=[
            logging.FileHandler("opynsim.log"), # Write logs to a log file...
            logging.StreamHandler()             # ... and also write them to the console.
        ]
    )

All log output from OPynSim goes via the ``logger.Logger`` returned by ``logger.getLogger("opynsim")``, or
one of its children.


The Search Path
---------------

Whenever OPynSim encounters a relative resource path in a file (e.g. ``pelvis.vtp`` in ``some_model.osim``), it
performs a search for that resource on the caller's filesystem. :func:`opynsim.get_search_paths`'s docstring
explains the process.

This mechanism can be useful for specifying central, shared, resource directories - particularly when sharing
mesh data across multiple models located in different directories (e.g. you might have a one-model-per-study
layout for models, but want a shared mesh geometry directory). :func:`opynsim.prepend_search_path` is how you could
specify that shared directory:

.. code:: python

    import opynsim as opyn
    from pathlib import Path

    # Passing a relative path causes it to resolve in a context-dependent way
    # (e.g. relative to each model file, or relative to the working directory)
    opyn.prepend_search_path("shared_geometry")

    # Passing an absolute path says "I don't care about context, always resolve it
    # relative to my working directory" (in this case).
    opyn.prepend_search_path(Path("shared_geometry").absolute())

See :func:`opynsim.get_search_paths` for a detailed explanation.

API Reference
-------------

.. autofunction:: opynsim.set_log_level
.. autofunction:: opynsim.get_search_paths
.. autofunction:: opynsim.prepend_search_path
.. autofunction:: opynsim.append_search_path
.. autofunction:: opynsim.remove_search_path
