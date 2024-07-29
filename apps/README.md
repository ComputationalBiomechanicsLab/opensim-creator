# `apps/`: OpenSim Creator's Executable Applications

This directory contains the code behind the top-level application executables
that users run. Generally speaking, this code is a thin layer around the
`OpenSimCreator` and `oscar` library targets.

| Directory | Description | Depends on |
| - | - | - |
| `osc/` | Handles the main user-facing OpenSim Creator UI binary (`osc.exe`) | `oscar`, `OpenSimCreator` |
| `hellotriangle/` | Implements a minimal usage of `oscar`'s `App` and graphics stack, used to test platform compatiblity | `oscar` |
