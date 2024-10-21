#!/usr/bin/env python3

# `build_windows`: performs an end-to-end build of OpenSim Creator on
# the Windows platform
#
#     usage (must be ran in repository root): `python3 build_windows.py`

import argparse
import logging
import multiprocessing
import os
import pprint
import subprocess

def _is_truthy_envvar(s : str):
    sl = s.lower()
    return sl in {"on", "true", "yes", "1"}

def _run(s: str, extra_env_vars={}):
    logging.info(f"running: {s}")
    subprocess.run(s, check=True, env={**extra_env_vars, **os.environ})

def _log_dir_contents(path: str):
    logging.info(f"listing {path}")
    for el in os.listdir(path):
        logging.info(f"{path}/{el}")

class BuildConfiguration:

    def __init__(
            self,
            *,
            base_build_type="Release",
            build_dir=os.curdir,
            build_concurrency=multiprocessing.cpu_count(),
            build_target="package",
            build_docs="OFF",
            build_skip_submodules=False,
            build_skip_osc=False):

        self.base_build_type = os.getenv("OSC_BASE_BUILD_TYPE", base_build_type)
        self.osc_deps_build_type = os.getenv("OSC_DEPS_BUILD_TYPE")
        self.osc_build_type = os.getenv("OSC_BUILD_TYPE")
        self.concurrency = int(os.getenv("OSC_BUILD_CONCURRENCY", build_concurrency))
        self.build_target = os.getenv("OSC_BUILD_TARGET", build_target)
        self.build_docs = _is_truthy_envvar(os.getenv("OSC_BUILD_DOCS", build_docs))
        self.generator_flags = f'-G"Visual Studio 17 2022" -A x64'
        self.build_dir = build_dir
        self.skip_submodules = build_skip_submodules
        self.skip_osc = build_skip_osc

    def __repr__(self):
        return pprint.pformat(vars(self))

    def get_dependencies_build_dir(self):
        return os.path.join(self.build_dir, "osc-dependencies-build")

    def get_dependencies_install_dir(self):
        return os.path.join(self.build_dir, "osc-dependencies-install")

    def get_osc_build_dir(self):
        return os.path.join(self.build_dir, "osc-build")

    def get_osc_deps_build_type(self):
        return self.osc_deps_build_type or self.base_build_type

    def get_osc_build_type(self):
        return self.osc_build_type or self.base_build_type

class Section:
    def __init__(self, name):
        self.name = name
    def __enter__(self):
        logging.info(f"----- start: {self.name}")
    def __exit__(self, type, value, traceback):
        logging.info(f"----- end: {self.name}")

def log_build_params(conf: BuildConfiguration):
    with Section("logging build params"):
        logging.info(conf)

def ensure_submodules_are_up_to_date(conf: BuildConfiguration):
    if (conf.skip_submodules):
        logging.info('--skip-submodules was provided: skipping git submodule update')
        return

    with Section("update git submodules"):
        _run("git submodule update --init --recursive")

def install_system_dependencies(conf: BuildConfiguration):
    with Section("install system dependencies"):
        if conf.build_docs:
            _run("pip install -r docs/requirements.txt")
        else:
            logging.info("skipping pip install: this build isn't building docs")

def build_osc_dependencies(conf: BuildConfiguration):
    with Section("build osc dependencies"):
        _run(
            f'cmake -S third_party/ -B "{conf.get_dependencies_build_dir()}" {conf.generator_flags} -DCMAKE_BUILD_TYPE={conf.get_osc_deps_build_type()} -DCMAKE_INSTALL_PREFIX="{conf.get_dependencies_install_dir()}" -DOSCDEPS_OSIM_BUILD_OPENBLAS=ON',
            # note: this is necessary because OpenSim transitively uses `spdlog`, which has deprecated this
            extra_env_vars={'CXXFLAGS': '/D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING'},
        )
        _run(f'cmake --build {conf.get_dependencies_build_dir()} --config {conf.get_osc_deps_build_type()} -j{conf.concurrency}')

    with Section("log osc dependencies build/install dirs"):
        _log_dir_contents(conf.get_dependencies_build_dir())
        _log_dir_contents(conf.get_dependencies_install_dir())

def build_osc(conf: BuildConfiguration):
    if conf.skip_osc:
        logging.info("--skip-osc was provided: skipping the OSC build")
        return

    with Section("build osc"):
        other_build_args = f'--config {conf.get_osc_build_type()} -j{conf.concurrency}'

        # configure
        _run(f'cmake -S . -B {conf.get_osc_build_dir()} {conf.generator_flags} -DCMAKE_PREFIX_PATH={os.path.abspath(conf.get_dependencies_install_dir())} -DOSC_BUILD_DOCS={"ON" if conf.build_docs else "OFF"} -DCMAKE_EXECUTABLE_ENABLE_EXPORTS=ON')

        # build+run oscar test suite
        test_oscar_path = os.path.join(conf.get_osc_build_dir(), 'tests', 'testoscar', conf.get_osc_build_type(), 'testoscar')
        _run(f'cmake --build {conf.get_osc_build_dir()} --target testoscar {other_build_args}')
        _run(f'{test_oscar_path} --gtest_filter="-Renderer*')

        # build+run OpenSimCreator test suite
        #
        # (--gtest_filter the tests that won't work in CI because of rendering issues)
        test_osc_path = os.path.join(conf.get_osc_build_dir(), 'tests', 'TestOpenSimCreator', conf.get_osc_build_type(), 'TestOpenSimCreator')
        _run(f'cmake --build {conf.get_osc_build_dir()} --target TestOpenSimCreator {other_build_args}')
        _run(f'{test_osc_path} --gtest_filter="-AddComponentPopup*:RegisteredOpenSimCreatorTabs*:LoadingTab*"')

        # build final output target (usually, the installer)
        _run(f'cmake --build {conf.get_osc_build_dir()} --target {conf.build_target} {other_build_args}')

def main():
    logging.basicConfig(level=logging.DEBUG)

    # create build configuration with defaults (+ environment overrides)
    conf = BuildConfiguration()

    # parse CLI args
    parser = argparse.ArgumentParser()
    parser.add_argument('--jobs', '-j', type=int, default=conf.concurrency)
    parser.add_argument('--skip-submodules', help='skip running git submodule update --init --recursive', default=conf.skip_submodules, action='store_true')
    parser.add_argument('--skip-osc', help='skip building OSC (handy if you plan on building OSC via Visual Studio)', default=conf.skip_osc, action='store_true')
    parser.add_argument('--build-dir', '-B', help='build binaries in the specified directory', type=str, default=conf.build_dir)
    parser.add_argument('--build-type', help='the type of build to produce (CMake string: Debug, Release, RelWithDebInfo, etc.)', type=str, default=conf.base_build_type)

    # overwrite build configuration with any CLI args
    args = parser.parse_args()
    conf.concurrency = args.jobs
    conf.skip_submodules = args.skip_submodules
    conf.skip_osc = args.skip_osc
    conf.build_dir = args.build_dir
    conf.base_build_type = args.build_type

    log_build_params(conf)
    ensure_submodules_are_up_to_date(conf)
    install_system_dependencies(conf)
    build_osc_dependencies(conf)
    build_osc(conf)

if __name__ == "__main__":
    main()
