# `src/oscar`: The underlying engine that powers OpenSim Creator

`oscar` is the "engine" part of OpenSim Creator. It's designed to have
a minimal, OpenSim-independent, dependency set while providing what's
typically necessary when creating research tooling:

| Directory | Description |
| - | - |
| `DOM/` | **WIP**: Document object model: classes, properties, objects, and nodes |
| `Formats/` | Parsers/Writers for file formats |
| `Graphics/` | Backend-independent, Unity-inspired graphics API |
| `Maths/` | Common mathematical classes/algorithms for UI/scene rendering |
| `Platform/` | Configuration, window creation, logging, error handling, stacktrace dumping, etc. |
| `Shims/` | Shims for upcoming but not-yet-supported versions of C++ |
| `UI/` | Anything related to rendering a 2D UI via ImGui |
| `Utils/` | Commonly-used helper classes/functions that typically augment the standard C++ library. |
| `Variant/` | A flexible runtime type (WIP: will be handy for scripting ,generic UIs, etc.) |
