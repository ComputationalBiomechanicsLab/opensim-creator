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

.. autofunction:: opynsim.import_osim_file
.. autofunction:: opynsim.compile_specification

.. automodule:: opynsim
   :members:
   :imported-members:
   :undoc-members:
   :show-inheritance:
