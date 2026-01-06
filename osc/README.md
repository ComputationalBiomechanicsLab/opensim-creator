# `osc/`: Main Application Executable for OpenSim Creator

This directory contains the necessary code to build `osc`, which is a top-level
executable that links to `libopensimcreator`. It's the main binary that end-users
are expected to run.

The main concern for this part of the codebase is to build a bootable executable
that correctly locates runtime resources (model files, icons) and can be installed
onto the target machine. If you are looking for implementation details, see
`libopensimcreator`.
