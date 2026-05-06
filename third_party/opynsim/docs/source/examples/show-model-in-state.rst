Show Model in State
===================

This example uses the :mod:`opynsim.ui` module to show the caller
an interactive window that displays the given :class:`opynsim.Model`
in the given :class:`opynsim.ModelState`.

.. code:: python

    import opynsim as opyn
    import opynsim.examples
    import opynsim.ui

    # Create/import a `Model` + `ModelState`.
    model = opyn.examples.double_pendulum_model()
    model_state = model.initial_state()
    model.realize(model_state, opyn.STAGE_REPORT)  # usually required for rendering

    # Show them in an interactive window.
    opyn.ui.show_model_in_state(model, model_state)
