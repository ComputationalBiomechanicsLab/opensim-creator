# `OpenSimCreator`

> Implementation code for the OpenSim Creator UI

This project directly integrates the [OpenSim API](https://github.com/opensim-org/opensim-core)
against the `oscar` API.

The general structure of this project is work-in-progress, primarly because
I'm midway through refactoring `oscar` into a more sane layout.

The implementation does not provide a `main` function (i.e. it's not an executable,
but a library). See `opensim-creator/apps` for applications that use the library API
to boot something.
