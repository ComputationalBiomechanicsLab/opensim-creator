# `docs/`: User-Facing Documentation for OPynSim

The documentation pages for OPynSim use a combination of the binding generation from
[nanobind](https://nanobind.readthedocs.io), [Sphinx](https://www.sphinx-doc.org), and
[Doxygen](https://www.doxygen.nl) to produce a single directory that can be uploaded to
a webserver. OPynSim's build system, CMake, is used to orchestrate the various parts
of the build, via the `opynsim_docs` target.
