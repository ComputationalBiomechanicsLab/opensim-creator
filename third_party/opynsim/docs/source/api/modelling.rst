Modelling
=========

At a high-level, OPynSim's modelling API is designed around three classes with
distinct roles:

- :class:`opynsim.ModelSpecification`: A high-level specification of the
  model. This is what Python code manipulates before calling :func:`opynsim.compile`
  to yield...
- :class:`opynsim.Model`: A compiled read-only physics model, which can
  create/read/manipulate...
- :class:`opynsim.ModelState`: A single state of an :class:`opynsim.Model`. This
  can be manipulated with Python code to do something useful with the model.


API Reference
-------------

.. autofunction:: opynsim.compile_specification
.. autofunction:: opynsim.import_osim_file
.. autofunction:: opynsim.example_specification_pendulum
.. autofunction:: opynsim.example_specification_double_pendulum

.. automodule:: opynsim
   :members:
   :imported-members:
   :undoc-members:
   :show-inheritance:

.. autodata:: opynsim.STAGE_TOPOLOGY
.. autodata:: opynsim.STAGE_MODEL
.. autodata:: opynsim.STAGE_INSTANCE
.. autodata:: opynsim.STAGE_TIME
.. autodata:: opynsim.STAGE_POSITION
.. autodata:: opynsim.STAGE_VELOCITY
.. autodata:: opynsim.STAGE_DYNAMICS
.. autodata:: opynsim.STAGE_ACCELERATION
.. autodata:: opynsim.STAGE_REPORT