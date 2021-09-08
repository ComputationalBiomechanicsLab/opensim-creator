# OpenSim Creator's Documentation

> [Sphinx](https://www.sphinx-doc.org/en/master/)-based docs that use the [furo](https://sphinx-themes.org/sample-sites/furo/) theme

This directory contains the source code for OpenSim Creator's official documenation. The
documentation is kept in-tree so that documentation revisions are part of each software
revision, which makes versioning etc. easier. It also makes packaging the documentation
with OpenSim Creator easier.

## 🏗️ Building + Developing

To build the documentation, you will need:

- A python3 release
- `pip`
  - Usually comes with python
  - On Windows, you may need to add `C:\Users\USERNAME\AppData\Local\Programs\Python\Python39\Scripts` to your `PATH`
  - On Linux, you can probably just `{apt-get,yum,pacman} install pip`
  - On Mac, you can probably just `brew install pip`

Workspace setup:

- If you use `venv`s, set one up: `python -m venv venv/ && source venv/bin/activate`
- Install dependencies: `pip install -r requirements.txt`

Building HTML:

- While in the `docs/` dir, run `sphinx-build -M html source build`
- The docs wil be written to `build/html`, they can be opened in a standard browser

Automatically Rebuilding + Reloading (handy for development):

- Install `sphinx-autobuild`: `pip install sphinx-autobuild`
- While in the `docs/` dir, run `sphinx-autobuild source build`
- Browse to `127.0.0.1:8000` in a browser
- Whenever you change a source file, the browser should reload
