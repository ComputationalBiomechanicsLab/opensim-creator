# `oscar_learnopengl`

> Implements https://learnopengl.com/ tutorials via the `oscar` API

This project provides `osc::ITab` implementations of many of the tutorials from
https://learnopengl.com/ , the itent of this code is to:

- Enable verifying that the `oscar` graphics API is capable of rendering
  each LearnOpenGL tutorial without directly referrring to OpenGL itself

- Act as a rosetta stone between the OpenGL API and the `oscar` graphics API

- Provide working examples of UIs that use the `oscar` API

Use `osc::register_learnopengl_tabs(TabRegistry&);` to register all demo tabs that
this project exports.
