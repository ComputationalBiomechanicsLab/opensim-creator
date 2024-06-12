# `OpenSimThirdPartyPlugins`

> Externally-sourced OpenSim plugins that are included with OpenSim Creator

This project builds a shared library containing OpenSim plugin components that
users have requested for OpenSim Creator.

The reason why plugins are included in-tree rather than (e.g.) providing a
native plugin API/UI in OpenSim Creator itself (much like in OpenSim) is
because OpenSim Creator is not built against a particular release version of
OpenSim (e.g. v4.5). Additionally, OpenSim Creator has to behave exactly
the same on multiple platforms without having to handle per-platform native
environments.

Current proposed plugin inclusion pipeline looks like this:

- User writes a native plugin against OpenSim and OpenSim GUI
- If it becomes popular/useful, it's pulled into here. Then OpenSim Creator's
  maintainers become responsible for ensuring it compiles on all target
  platforms (typically, end-users/researchers only ensure the plugin works
  on Windows against one version of OpenSim)
- If people keep using it, then the plugin should be upstreamed to
  `opensim-core` as a builtin component

# Registering Components

Downstream codebases (e.g. `OpenSimCreator`) must call `RegisterTypes_osimPlugin`
to register all components with the central `OpenSim::Object` registry.
