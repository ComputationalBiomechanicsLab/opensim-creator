![logo with title](logo_with_title.svg)

A lightweight development UI for [OpenSim](https://github.com/opensim-org/opensim-core)

🚧 **ALPHA-STAGE SOFTWARE: take everything you see here with a pinch of salt and extremely low expectations** 🚧

![screenshot](screenshot.png)

`osmv` is a lightweight GUI for the OpenSim C++ API. It is designed to be easy to build and only depends on OpenSim + lightweight GUI libraries (SDL2, GLEW, and ImGui). `osmv` was developed to make it easy to create prototype GUIs on top of the OpenSim API. 

It is not designed as a production-ready user-facing GUI for OpenSim. For that, you should use the official [OpenSim GUI](https://github.com/opensim-org/opensim-gui).

# Downloads

**Alpha stage**: these download links are just updated whenever we get a clean build from CI. They are not formal releases and they are not tested. Please give osmv a try--and definitely report bugs--but you might find the quality level fairly low at the moment.

| OS | Comments |
| - | - |
| [Debian/Ubuntu](https://github.com/adamkewley/osmv/suites/2004144172/artifacts/40079334) | Should work on Debian Buster+ or Ubuntu16+ |
| [OSX (Catalina)](https://github.com/adamkewley/osmv/suites/2004144172/artifacts/40079335) | Should work on Catalina+, unsure about earlier versions |
| [Windows](https://github.com/adamkewley/osmv/suites/2004144172/artifacts/40079336) | Should work on Windows 10 |


# Building

Because things are still in alpha (and might change, a lot), we don't currently publish a full build guide. However, the build is designed to be easy to run on a blank machine, so there are end-to-end build scripts /w comments for building on each platform. Please read these:

| OS | Comments |
| - | - |
| [Ubuntu 20+/Debian Buster](scripts/debian-buster_e2e-build.sh) | Builds OpenSim+osmv from scratch |
| [Ubuntu Xenial](scripts/ubuntu-xenial_e2e-build.sh) | Downloads gcc-8 and builds OpenSim+osmv from scratch using that |
| [OSX (Catalina)](scripts/mac_catalina_10-15_build.sh) | Builds OpenSim+osmv from scratch using basic build toolchain (full XCode shouldn't be necessary) |
| [Windows 10](scripts/windows_e2e-build.bat) | Builds OpenSim+osmv using VS2019 |

These are the exact build scripts we use for continuous integratation (config [here](.github/workflows/continuous-integration-workflow.yml)). If you can't get a build working then it might be our fault. Check the [actions](https://github.com/adamkewley/osmv/actions) page to see if the build is passing CI on GitHub.
