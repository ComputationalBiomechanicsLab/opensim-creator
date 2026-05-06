Modelling
=========

Modelling is a central part of of the OPynSim API. At a high-level, the API is designed
around three classes with distinct roles:

- :class:`opynsim.ModelSpecification`: A high-level specification of the
  model. This is what Python code manipulates before calling :meth:`opynsim.ModelSpecification.compile`
  to yield...
- :class:`opynsim.Model`: A compiled read-only physics model, which can
  create/read/manipulate...
- :class:`opynsim.ModelState`: A single state of an :class:`opynsim.Model`, which can
  be modified by Python code to modify the state of the model.

When modelling with OPynSim, it's important to understand this relationship, because the rest
of the modelling API is built around it.


Typical Modelling Workflow
--------------------------

Typical modelling workflows follow some combination of these steps:

1. Import or create a :class:`opynsim.ModelSpecification`.
2. If required, modify the  :class:`opynsim.ModelSpecification`.
3. Compile the :class:`opynsim.ModelSpecification` to a :class:`opynsim.Model`.
4. Generate an initial :class:`opynsim.ModelState` with :meth:`opynsim.Model.initial_state`.
5. If required, modify the :class:`opynsim.ModelState`.
6. Use, or extract data from, the resulting :class:`opynsim.Model` + :class:`opynsim.ModelState` pair.

See :doc:`../examples/plot-model-output-vs-property` for an example of this pattern.
