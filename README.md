# OPynSim

> Musculoskeletal Modelling Simplified.
>
> ⚠️ **EXPERIMENTAL** ⚠️ : This is an experimental/prototype project that requires more resources.

The aim of OPynSim is to provide a python-native API for musculoskeletal modelling that
doesn't compromise on nearly 20 years of research, feature development, and UI development from
[Simbody](https://github.com/simbody/simbody), [OpenSim](https://simtk.org/projects/opensim),
and [OpenSim Creator](https://www.opensimcreator.com/).

# Installation

See supported platforms

```bash
pip install opynsim
```

# Supported Platforms

OPynSim is supported on the following platform combinations:

| Python Version | Operating System                                                             | Processor Architecture  |
|--------------- |------------------------------------------------------------------------------|-------------------------|
| >=3.12         | Windows                                                                      | amd64 (Intel/AMD)       |
| >=3.12         | Linux (manylinux 234: Debian 12+, Ubuntu 21.10+, Fedora 35+, CentOS/RHEL 9+) | amd64 (Intel/AMD)       |
| >=3.12         | MacOS >= 14.5 (Sonoma)                                                       | amd64 (Intel)           |
| >=3.12         | MacOS >= 14.5 (Sonoma)                                                       | arm64 (Apple Silicon)   |


# Core Goals

- Make building and integrating [Simbody](https://github.com/simbody/simbody)'s and
  [OpenSim](https://simtk.org/projects/opensim)'s C++ APIs as easy as possible, to
  encourage new developers to work on those codebases (`libopynsim`).
- Provide a straightforward infrastructure for researchers to develop new C++ and python
  algorithms that are tested and deployed regularly.
- Upstream stable ideas, algorithms, project designs, dependency handling methods, etc. from this
  project to [Simbody](https://github.com/simbody/simbody) and [OpenSim](https://simtk.org/projects/opensim).

# TODO

- Compiling everything within Rosetta: `arch -x86_64 /usr/bin/env bash`
- Codesigning on MacOS/Windows
- Sanity-check MSVC runtime in opynsim
- Sanity-check MacOS SDK/ABI in opynsim
- Sanity-check SDK version in opynsim
- Python declarations `.pyd` for IDEs etc.
- API documentation in the documentation build
- Developer guide that specifically focuses on CLion
- READMEs/architectural explanations
- Integration into opensim-creator build
