# `liboscar`: A library for building research UIs

`liboscar` is a library for creating research UIs. It's designed to have a
minimal dependency set, so that it can be built from source on all target
platforms+architectures, and a very low-level game-engine-like
design, so that prototype UIs can be created quickly.

| Directory   | Description |
|-------------| - |
| `concepts/` | Custom C++20 concepts |
| `dom/`      | **WIP**: Document object model: classes, properties, objects, and nodes |
| `formats/`  | Parsers/Writers for file formats |
| `graphics/` | Backend-independent, Unity-inspired graphics API |
| `maths/`    | Common mathematical classes/algorithms for UI/scene rendering |
| `platform/` | Configuration, window creation, logging, error handling, etc. |
| `shims/`    | Shims for upcoming but not-yet-supported versions of C++ |
| `tests/`    | Code related to the unittest suite for `liboscar` |
| `ui/`       | Anything related to rendering a 2D UI |
| `utils/`    | Commonly-used helper classes/functions that typically augment the standard C++ library. |
| `variant/`  | A flexible runtime type (WIP: will be handy for scripting, generic UIs, etc.) |

