# `ThirdPartyPlugins`

> Externally-sourced OpenSim plugins that are bundled included with `osim`

This project builds a shared library containing OpenSim plugin components that
users have requested for inclusion into `osim`.

The reason why plugins are included in-tree rather than (e.g.) providing a
native plugin API/UI is because `osim`'s primary aim is to be an all-in-one
binary that doesn't build against a particular API/ABI of OpenSim (e.g. v4.5).

Current proposed plugin inclusion pipeline looks like this:

- User writes a native plugin against OpenSim and OpenSim GUI
- If it becomes popular/useful, it's pulled into here, so that projects using
  `osim` (e.g. OpenSim Creator) become responsible for ensuring it compiles
  and is available on all target platforms (typically, end-users/researchers
  only ensure their plugin works on Windows against one version of OpenSim)
- If people keep using the plugin, then it's upstreamed into `opensim-core`
  as a builtin component


# Registering Components

Downstream code must call `RegisterTypes_osimPlugin` to register all third-party
plugins with OpenSim's central `OpenSim::Object` registry.

