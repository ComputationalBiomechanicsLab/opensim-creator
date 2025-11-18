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

| Python Interpreter | Operating System | Processor Architecture |
| ------------------ | ---------------- | ---------------------- |
| >=3.12             | Windows          | amd64 (Intel/AMD)      |


# Core Goals

- Make building and integrating [Simbody](https://github.com/simbody/simbody)'s and
  [OpenSim](https://simtk.org/projects/opensim)'s C++ APIs as easy as possible, to
  encourage new developers to work on those codebases (`libopynsim`).
- Provide a straightforward infrastructure for researchers to develop new C++ and python
  algorithms that are tested and deployed regularly.
- Upstream stable ideas, algorithms, project designs, dependency handling methods, etc. from this
  project to [Simbody](https://github.com/simbody/simbody) and [OpenSim](https://simtk.org/projects/opensim).

# TODO

- GitHub Actions build
- Development/Release configurations /w strict compiler flags
- Codesigning on MacOS/Windows
- API documentation in the documentation build
- Developer guide that specifically focuses on CLion
- READMEs/architectural explanations
- Integration into opensim-creator build
