# OpenSim Creator's Documentation

> The source code behind https://docs.opensimcreator.com
>
> A manual for [OpenSim Creator](https://github.com/ComputationalBiomechanicsLab/opensim-creator) that's
> targeted at end-users. Built using [Sphinx](https://www.sphinx-doc.org/en/master/)

This repository contains the source code for OpenSim Creator's user-facing
documentation, which is hosted at https://docs.opensimcreator.com .


## 🖥️ Dependencies/Environment Setup

This documentation is mostly self-standing, but may contain links to
`files.opensimcreator.com` (usually, when an asset is large or shared, such as
video tutorials). The concrete deployment steps that we use to handle
`files.opensimcreator.com` and actually deploy the documentation to
`docs.opensimcreator.com` is described in OpenSim Creator's [README.md](../README.md).

To build the documentation, you will need to setup your development
environment with:

- `python` and `pip`
- A python virtual environment (can be a `venv`, e.g. `python -m venv venv/ && source venv/bin/activate`)
- All relevant python packages installed into that environment (e.g. `pip install -r requirements.txt`)

If you plan on developing the documentation, you probably also want the
development requirements listed in `requirements-dev.txt` (e.g. run `pip install -r requirements-dev.txt`).

E.g. here's how you'd set up a suitable development environment on Ubuntu:

```bash
apt install python
python -m venv venv/
source venv/bin/activate
pip install -r requirements.txt
pip install -r requirements-dev.txt
```


## 🏗️ Building

To build the documentation source code into standalone web assets, use the
`sphinx-build` command:

```bash
# writes the documentation to `build/html`
sphinx-build -M html source/ build/
```


## ⌨️ Developing

The `requirements-dev.txt` installs the `sphinx-autobuild` package, which can be
used to host a local webserver that live-reloads the website whenever you edit
one of the source (i.e. `.rst`) files:

```bash
# run this in a console
sphinx-autobuild source/ build/

# ... and then browse to `localhost:8000`
```


## 🕊️ Releasing

The documentation is built+released alongside OpenSim Creator, so the release
procedure for the documentation is part of the release procedure for OpenSim
Creator itself. See [RELEASE_CHECKLIST.md](../RELEASE_CHECKLIST.md).


## 🚀 Deploying

The deployment procedure happens post-release and is described in
[RELEASE_CHECKLIST.md](../RELEASE_CHECKLIST.md). That procedure is subject to change,
but *probably* involves something like `rsync`ing the built assets to a webserver, or
GitHub Pages.
