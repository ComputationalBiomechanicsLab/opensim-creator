# `opensimcreator`: Python Bindings for OpenSim Creator

> **EXPERIMENTAL**: the bindings are currently very work in progress. We will
> try to put anything unstable into the `opensim.experimental` module and
> try to ensure anything else is more-stable, but some breakages should be
> expected until stated otherwise.

This directory contains python bindings for `libOpenSimCreator` and `libosim`,
so that end-users can automate some parts of the model building/visualization
process.

This is intended to be a standalone, cleanroom, API that uses completely
independent datastructures from the OpenSim python API. The intention is
to focus on designing something that feels coherent and easy to use with
other high-level python libraries (e.g. `numpy`, `pandas`), rather than
something that's fast and/or entirely compatible with OpenSim's APIs. Instead,
we'll provide APIs for ingesting/producing OpenSim's **data** (e.g. `osim`,
`sto`, `trc`) such that it can be separately fed into whatever version of
the OpenSim API you're using.

