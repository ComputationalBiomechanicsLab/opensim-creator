# `apps/`: OpenSim Creator's Executable Applications

This directory contains the code behind the top-level application executables
that users run. Generally speaking, this code is a thin layer around the
`OpenSimCreator` and `oscar` library targets.

| Directory | Description | Depends on |
| - | - | - |
| `osc/` | Handles the main OpenSim Creator UI binary that users typically boot into | `oscar`, `OpenSimCreator` |
| `hellotriangle/` | A bare-minimum usage of `oscar`'s `App` and graphics stack, used by developers for testing minimum platform compliance | `oscar` |
