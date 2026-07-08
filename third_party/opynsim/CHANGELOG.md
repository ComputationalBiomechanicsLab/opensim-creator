# ChangeLog

All notable changes to this project will be documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).


## [Upcoming Release] - YYYY/MM/DD

## 0.0.6 - 2026/06/08

0.0.6 is a pre-alpha development release, don't expect stability, or the
CHANGELOG, to make much sense yet!

- Patched an issue in OpenSim where it wasn't correctly clearing out memory
  when a model contained graph cycles (slave bodies, etc.).
- Patched an issue in OpenSim where it could segfault with use-after-free
  issues when re-finalizing a model containing graph cycles over-and-over.
- Updated OpenSim from `30430e4feecfef3f6385140c72b8e34a54d76d3e` to `286b1f60f147ae707edaa0693931d750305ae50b`, 
  which includes new `MeyerFregly2016Force` and `MeyerFregly2016Muscle` components.
- Updated simbody to from `f9ab12cbad9d0da106473259d34c50577f934f49` to `7f35622b3c5daac919fc39a2865498c03c553e53`,
  which contains some cable path fixes.
- Nanobind was updated to v2.12.0
- OpenBLAS was updated to v0.3.31
- Switched Windows build to statically compile the C++ runtime, because the
  binary is standalone and doesn't leak exceptions via the Python API.
- Added support for FPS-style UX via:
  - `App::enable_main_window_relative_mouse_mode`
  - `App::disable_main_window_relative_mouse_mode`
  - `App::main_window_mouse_confinement`
  - `App::set_main_window_mouse_confinement`
- Fixed an issue where `OpenSim::IMU` decorations were being emitted with
  negative scale factors (ComputationalBiomechanicsLab/opensim-creator/#1179).

