> **Warning**: This library is work-in-progress and is currently
> only used *within* OpenSim Creator.
>
> Long-term, the intention is to publish `oscar` separately from OpenSim Creator.
> However, `oscar` isn't ready for that yet, because various APIs within `oscar`
> are not yet stabilized, or reveal unnecessary internals, or do something very
> dumb, etc.

# `oscar`

> A framework for creating research tooling.

`oscar` is the "engine" part of OpenSim Creator. It's designed to have
a minimal, OpenSim-independent, dependency set and provide what's typically
necessary when creating research tooling:

| Component | Description |
| - | - |
| `Formats/` | Parsers/Writers for file formats |
| `Graphics/` | Backend-independent, Unity-inspired graphics API |
| `Maths/` | Common mathematical classes/algorithms for UI/scene rendering |
| `Object/` | Standardized implementation of classes, properties, objects, and node hierarchies (WIP) |
| `Platform/` | Configuration, window creation, logging, error handling, stacktrace dumping, etc. |
| `Scene/` | Mid-level classes for rendering a 3D scene |
| `Shims/` | Shims for upcoming but not-yet-supported versions of C++ |
| `UI/` | Anything related to rendering a 2D UI via ImGui |
| `Utils/` | Commonly-used helper classes/functions that typically augment the standard C++ library. |
| `Variant/` | A flexible runtime type (WIP: will be handy for scripting ,generic UIs, etc.) |
