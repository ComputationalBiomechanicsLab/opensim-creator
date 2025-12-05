# liboscar-demos: A library containing democode for `liboscar`

`liboscar-demos` is a library target that exposes various tech demos
as `osc::Tab`s that can be opened in the UI to demo something.

Most of this democode exercises `liboscar`'s graphics API, with a
big chunk of the democode derived from the content of [Learn OpenGL](https://learnopengl.com/),
because the `liboscar` graphics API was developed incrementally from
scratch in OpenGL (it only later adopted a backend-agnostic design).
