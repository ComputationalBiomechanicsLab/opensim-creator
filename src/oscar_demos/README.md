# `src/oscar_demos`: Demos that use the `oscar` API

- Provides `osc::Tab` implementations for library demos (e.g. `ImGuiDemoTab`)

- And provides tab implementations that use the `oscar` API in an interesting
  way (e.g. `HittestTab`)

Use `osc::register_demo_tabs(TabRegistry&);` to register all demo tabs that
this project exports.
