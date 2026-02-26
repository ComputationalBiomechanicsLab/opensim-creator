Application
-----------

The :mod:`opynsim.graphics` and :mod:`opynsim.ui` modules depend on the
creation of application state, so that they can access global
operating-system level capabilities, such as graphics context and
window creation.

As an OPynSim user, you must ensure that a :class:`opynsim.Application`
is initialized before trying to render anything (requires a graphics
context), create interactive UIs (requires an application event
queue), or show application windows (requires application window
manager).

TODO: think about this a bit: how do we manage things like people
composing multiple Python libraries that independently want to render
things? Might be better to just have a sane-looking lazy initializer
rather than potential application collisions, or have all functions
take ``opynsim.Application`` as an argument to encourage dependency
injection.
