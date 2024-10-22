# `scripts/`: Various Scripts For Building/Developing/Introspecting OpenSim Creator

This directory contains arbitrary scripts that are used to perform various
project-related tasks in OpenSim Creator. There's no *solid* conventions here,
it's just whatever happens to be useful for project development. Some *loose*
conventions are:

- `cellar/` contains scripts that probably haven't been used for a while but
  are sticking around because I'm a kleptomaniac
- `build_*` are scripts related to building OpenSim Creator from source on
  various platforms and with various flags
- `env_*` are usually used in conjunction with `source` to setup a terminal's
  environment variables etc. before running `osc` et. al.
- `*.supp` are suppression files to work around the various known-about leaks
  in OpenSim, or OS libraries
