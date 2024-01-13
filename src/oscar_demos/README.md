# `oscar-demos`

> Demos that use the `oscar` API to demonstrate something useful/interesting

- Provides `osc::ITab` implementations for library demos (e.g. `ImGuiDemoTab`)

- And provides tab implementations that use the `oscar` API in an interesting
  way (e.g. `HittestTab`)

Use `osc::RegisterDemoTabs(TabRegistry&);` to register all demo tabs that
this project exports.
