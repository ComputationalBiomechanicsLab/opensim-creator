# `libosim`: Customized OpenSim Build

`libosim` is a custom `cmake` build of OpenSim's and Simbody's
source code, such that a single `cmake` target, `osim` is produced
as a static library that OpenSim Creator (+bindings) can link
to without having to faff around with upstream OpenSim's and Simbody's
cmake build.

This also adds extension code on top of the `OpenSim` API, namely:

- `osim.{h,cpp}` custom code, under the `osim` namespace, for initializing
   the OpenSim API in a sane way (i.e. with the US locale, without the
   default logging implementation, with all the submodules of OpenSim, such
   as `osimActuators`, `osimTools`, etc.)
- `ThirdPartyPlugins/`: third-party plugin code, usually from GitHub
   or [SimTK.org](https://simtk.org) that has been vendored so that
   research users don't have to figure out how to compile+load the
   plugin (a common pain point with OpenSim's native APIs)

Optionally, this subproject provides a `OSIM_INSTALL_OPENSIM_HEADERS` cmake
option, which installs the underlying OpenSim+Simbody headers and the `osim`
cmake target. This is mostly API compatible with OpenSim's C++ API (OpenSim
Creator was able to compile against it, rather than OpenSim, when it was
developed), but downstream code will have to link to `osim` (the static libray)
with `target_link_library(yourprogram PUBLIC osim)` and initialize the API
with `osim::init` from `<libosim/osim.h>`.

