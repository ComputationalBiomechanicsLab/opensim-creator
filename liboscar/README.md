# `liboscar`: Engine code for CAD projects

`liboscar` is an engine for making CAD tooling. It's designed to have a minimal
dependency set, such that it can be built from source on all target
platforms+architectures.

| Directory | Description |
| - | - |
| `Concepts/` | Custom C++20 concepts |
| `DOM/` | **WIP**: Document object model: classes, properties, objects, and nodes |
| `Formats/` | Parsers/Writers for file formats |
| `Graphics/` | Backend-independent, Unity-inspired graphics API |
| `Maths/` | Common mathematical classes/algorithms for UI/scene rendering |
| `Platform/` | Configuration, window creation, logging, error handling, stacktrace dumping, etc. |
| `Shims/` | Shims for upcoming but not-yet-supported versions of C++ |
| `testing/` | Code related to the unittest suite for `liboscar` |
| `UI/` | Anything related to rendering a 2D UI |
| `Utils/` | Commonly-used helper classes/functions that typically augment the standard C++ library. |
| `Variant/` | A flexible runtime type (WIP: will be handy for scripting ,generic UIs, etc.) |
