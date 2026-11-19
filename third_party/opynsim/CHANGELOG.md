# ChangeLog

All notable changes to this project will be documented here. The format is based
on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).


## [Upcoming Release] - YYYY/MM/DD

## [0.9.0] - 2026/11/19

v0.9.0 is effectively a re-tagged v0.1.0. The reason for this is so that OPynSim's version
number matches [OpenSim Creator's](https://www.opensimcreator.com), because the two projects
will be versioned in lock-step from now on.


## [0.1.0] - 2026/11/17

0.1 is a pre-alpha development release that rearranges the locations of various classes
in the Python API for ease-of-import (now, `import opynsim` suffices) and simplifies
things like the camera API.

Additionally, OPynSim's build now includes [OpenSim Creator](https://github.com/ComputationalBiomechanicsLab/opensim-creator) (OSC),
which makes development of both products in lockstep much easier. To accommodate
this, and standardize the codebase, thousands of changes to the C++ codebase were also
made (rearranging, reformatting). Those shouldn't affect user-facing behavior. We will
continue to ship OSC as a stable, separate application - this is just a build change.

- Python: BREAKING: Deleted the `opynsim.graphics` submodule: all of its definitions are now
  contained in the top-level `opynsim` module.
- Python: BREAKING: Deleted the `opynsim.solvers` submodule: all of its definitions are
  now contained in the top-level `opynsim` module.
- Python: Added `ForwardDynamicSimulator.model_state` property, so code can get the
  internal `ModelState` that's managed by a `ForwardDynamicSimulator`.
- Python: Changed `opynsim.ForwardDynamicsSolver.__init__` such that `model_state`
  is now an optional argument that defaults to `model.initial_state()` when not supplied.
- Python: `opynsim.Camera` now has a `vertical_field_of_view` read/write property.
- Python: Added a constructor to `opynsim.Camera` that optionally accepts `position`,
  `direction`, `up`, and `vertical_field_of_view`, which enables one-liner camera initializers.
- Python: Added `opynsim.Camera.look_at`, which creates a camera looking at a target.
- Python: Added `opynsim.Camera.__repr__`, `opynsim.ModelState.__repr__`, and
  `opynsim.Model.__repr__`.
- Python: `opynsim.ui.show_model_in_state` now accepts a `camera`, which the implementation
  will handily mutate in-place.
- Python: functions that accept `Vector3`-like inputs (e.g. `Camera` constructor) now accept
  them as a `numpy.ndarray`, a 3-element Python tuple, or a 3-element Python list (previously:
  only `numpy.ndarray` was accepted).
- Python: The type annotation for `Vector3`-like inputs was simplified to "Vector3f", because
  the actual type annotation was extremely long and annoying to actually read ;).
- OSC: Removed support for `Smith2018ArticularContactForce` and `Smith2018ContactMesh` in order
  to ensure `.osim` data interoperability with baseline OpenSim. Use an older version of
  OPynSim/OpenSim Creator if you need to use these components, or get them merged
  into opensim-core!
- OSC: The model editor now has user-editable `target` and `position` inputs in the
  camera controls, which matches the Python API's `Camera.look_at` API.
- OSC: The mesh warping workflow now has a right-click context menu for the mesh, which
  includes a new "Move Selection Here" option, enabling moving existing landmarks to
  the click location.
- OSC: Fixed a bug where the global geometry directory took precedence over the local geometry
  directory (#51).
- C++: `libopensimcreator` and `osc` were merged into OPynSim's build, so that both OPynSim
  and OpenSim Creator can be built from one source tree without much effort.
- C++: Added `osc::Camera::Camera(position, direction, up)` constructor.
- C++: Added `osc::Camera::look_at(position, target, up)` factory function.
- C++: Added `opyn::operator==(const ModelState&, const ModelState&)` so that states can be
  compared (e.g. for tests).
- C++: Added `std::formatter` support for `ModelStateStage` and `ModelState`.
- C++: Added `std::formatter` support for `Model`.
- C++: `osc::ui::MouseButton` was merged into `osc::MouseButton`, to reduce enum/type duplication.
- C++: Refactored `osc::num_options<T>()` `constexpr` function to a `constexpr`
  value `osc::num_options_v<T>`.
- C++: Changed the minimum step size used by the `ForwardDynamicSolver` to match simbody's
  default minimum step size (previously, was `1.0e-8`). This matches OpenSim's default.


## 0.0.8 - 2026/09/08

0.0.8 is a pre-alpha development release that adds model warping support
to the Python API (see API documentation for `opynsim.solvers.ModelWarper`.

- OpenSim's `Tools/` are no longer part of the build (unused)
- The `graphics.Camera` class now auto-normalizes the `direction` and
  `up` properties when they are set from Python code.
- Thin-Plate Spline (TPS) scaling steps in the model warper now cache the
  warping solver between steps, which improves performance for larger
  model warping pipelines with many TPS scaling steps.
- Model warping scaling steps now automatically convert any backward slashes
  in filepaths (e.g. TPS source/destination landmarks) into forward slashes,
  so that legacy files from Windows computers can be loaded on Unix-based
  computers.


## 0.0.7 - 2026/07/15

0.0.7 is a pre-alpha development release, don't expect stability, or an
accurate CHANGELOG!

- Fixed `Model.states_from_data_frame` not supporting legacy column headers.
- The `ComponentOutputExtractor` name should now more-closely match the output names
  returned by OPynSim's output API


## 0.0.6 - 2026/06/08

0.0.6 is a pre-alpha development release, don't expect stability, or an
accurate CHANGELOG!

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

