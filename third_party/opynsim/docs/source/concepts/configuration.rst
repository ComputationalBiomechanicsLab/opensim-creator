Configuration
=============

The :mod:`opynsim.config` module centralizes functions that configure the global
state of OPynSim. These are sometimes necessary for debugging, implementing
legacy compatibility, and customizing where shared assets should be loaded from.

Logging
-------

It's useful to globally configure ``opynsim``\'s log level, so that downstream
scripts can contain more/less feedback. It can be configured with :func:`opynsim.config.set_log_level`:

.. code:: python

    import opynsim as opyn
    import opynsim.config
    import logging

    # Make OPynSim's logging more verbose (default is `logging.WARN`).
    opyn.config.set_log_level(logging.DEBUG)

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


Resource Search Path
--------------------

Whenever OPynSim encounters a relative resource path in a file (e.g. ``pelvis.vtp`` in ``some_model.osim``), it
performs a search for that resource on the caller's filesystem. The documentation for :func:`opynsim.config.get_search_paths`
explains the process.

Editing the search path can be useful for specifying central, shared, resource directories - particularly when
sharing mesh data across multiple models located in different directories (e.g. you might have a one-model-per-study
layout for models, but want a shared mesh geometry directory). :func:`opynsim.config.prepend_search_path` is one
way to specify an additional shared directory:

.. code:: python

    import opynsim as opyn
    import opynsim.config
    from pathlib import Path

    # Passing a relative path causes it to resolve in a context-dependent way
    # (e.g. relative to each model file, or relative to the working directory)
    opyn.config.prepend_search_path("shared_geometry")

    # Passing an absolute path says "I don't care about context, always resolve it
    # to this exact location."
    opyn.config.prepend_search_path(Path("shared_geometry").absolute())

See :func:`opynsim.config.get_search_paths` for a detailed explanation of the
path resolution process. Alternatively, :func:`opynsim.config.append_search_path`
can be used to specify a low-priority fallback directory.
