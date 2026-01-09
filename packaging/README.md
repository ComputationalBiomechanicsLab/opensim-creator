# `packaging/`: Packaging Scripts For Distributing OpenSim Creator

This directory the scripts + resources that are necessary to package
a complete distribution of OpenSim Creator, which includes the
executable (`osc`), any libraries, runtime resources, icons, and
OS-specific metadata. It also handles packaging-adjacent concerns,
such as code signing and notarization.

Packaging **not** include handling third-party code (see `third_party/`),
implementation details (see `libopensimcreator/`), or compiling and installing
the necessary resources to run the top-level executable (see `osc/`).
