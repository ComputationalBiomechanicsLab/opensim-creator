# `liboscar`: Engine code for CAD projects

`liboscar` is the "engine" part of OpenSim Creator. It's designed to have
a minimal dependency set, such that external projects can use it without
also pulling in OpenSim.

| Directory | Description |
| - | - |
| `DOM/` | **WIP**: Document object model: classes, properties, objects, and nodes |
| `Formats/` | Parsers/Writers for file formats |
| `Graphics/` | Backend-independent, Unity-inspired graphics API |
| `InternalVendoredCode/` | Third-party code that has been copied+pasted into `liboscar`, potentially with modifications |
| `Maths/` | Common mathematical classes/algorithms for UI/scene rendering |
| `Platform/` | Configuration, window creation, logging, error handling, stacktrace dumping, etc. |
| `Shims/` | Shims for upcoming but not-yet-supported versions of C++ |
| `testing/` | Code related to the unittest suite for `liboscar` |
| `UI/` | Anything related to rendering a 2D UI |
| `Utils/` | Commonly-used helper classes/functions that typically augment the standard C++ library. |
| `Variant/` | A flexible runtime type (WIP: will be handy for scripting ,generic UIs, etc.) |
