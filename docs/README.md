# OpenSim Creator's User-Facing Documentation

> The source code behind https://docs.opensimcreator.com, which is a user-facing
> manual for the [OpenSim Creator Project](https://github.com/ComputationalBiomechanicsLab/opensim-creator).
> Built using [Sphinx](https://www.sphinx-doc.org/en/master/).


## 🖥️ Dependencies/Environment Setup

This documentation is mostly self-standing, but may contain links to
`files.opensimcreator.com` (usually, when an asset is large or shared, such as
video tutorials).

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
pip install -r requirements.txt -r requirements-dev.txt
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
