#!/usr/bin/env python3

# `build_windows`: performs an end-to-end build of OpenSim Creator on
# the Windows platform
#
#     usage (must be ran in repository root): `python3 scripts/build_windows.py`

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

def _run_cmake_configure(source_dir, binary_dir, generator, architecture, cache_variables, extra_env_vars={}):
    all_cache_variables = ' '.join([f'-D{k}={v}' for k, v in cache_variables.items()])
    maybe_arch_flag = f'-A {architecture}' if 'Visual Studio' in generator else ''
    _run(f'cmake -S {source_dir} -B {binary_dir} -G {generator} {maybe_arch_flag} {all_cache_variables}', extra_env_vars)

def _run_cmake_build(binary_dir, config, concurrency, target=None):
    maybe_target_flag = f'--target {target}' if target else ''
    _run(f'cmake --build {binary_dir} --config {config} -j{concurrency} {maybe_target_flag}')

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
            system_version=None,
            build_skip_osc=False):

        self.base_build_type = os.getenv("OSC_BASE_BUILD_TYPE", base_build_type)
        self.osc_deps_build_type = os.getenv("OSC_DEPS_BUILD_TYPE")
        self.osc_build_type = os.getenv("OSC_BUILD_TYPE")
        self.concurrency = int(os.getenv("OSC_BUILD_CONCURRENCY", build_concurrency))
        self.build_target = os.getenv("OSC_BUILD_TARGET", build_target)
        self.build_docs = _is_truthy_envvar(os.getenv("OSC_BUILD_DOCS", build_docs))
        self.generator = 'Visual Studio 17 2022'
        self.architecture = 'x64'
        self.system_version = os.getenv('OSC_SYSTEM_VERSION', system_version)
        self.build_dir = build_dir
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

def build_osc_dependencies(conf: BuildConfiguration):
    with Section("build osc dependencies"):
        # calculate cache variables
        cache_variables = {
            'CMAKE_BUILD_TYPE': conf.get_osc_deps_build_type(),
            'CMAKE_INSTALL_PREFIX': conf.get_dependencies_install_dir(),
            'OSCDEPS_BUILD_OPENBLAS': 'ON'
        }
        if conf.system_version:
            cache_variables['CMAKE_SYSTEM_VERSION'] = conf.system_version

        # configure
        _run_cmake_configure(
            source_dir='third_party',
            binary_dir=conf.get_dependencies_build_dir(),
            generator=conf.generator,
            architecture=conf.architecture,
            cache_variables=cache_variables,
            # note: this is necessary because OpenSim transitively uses `spdlog`, which has deprecated this
            extra_env_vars={'CXXFLAGS': '/D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING'}
        )
        # build
        _run_cmake_build(
            binary_dir=conf.get_dependencies_build_dir(),
            config=conf.get_osc_deps_build_type(),
            concurrency=conf.concurrency
        )

    with Section("log osc dependencies build/install dirs"):
        _log_dir_contents(conf.get_dependencies_build_dir())
        _log_dir_contents(conf.get_dependencies_install_dir())

def build_osc(conf: BuildConfiguration):
    if conf.skip_osc:
        logging.info("--skip-osc was provided: skipping the OSC build")
        return

    with Section("build osc"):
        # calculate cache variables
        cache_variables = {
            'CMAKE_BUILD_TYPE': conf.get_osc_build_type(),
            'CMAKE_PREFIX_PATH': os.path.abspath(conf.get_dependencies_install_dir()),
            'OSC_BUILD_DOCS': 'ON' if conf.build_docs else 'OFF'
        }
        if conf.system_version:
            cache_variables['CMAKE_SYSTEM_VERSION'] = conf.system_version

        # configure
        _run_cmake_configure(
            source_dir='.',
            binary_dir=conf.get_osc_build_dir(),
            generator=conf.generator,
            architecture=conf.architecture,
            cache_variables=cache_variables
        )

        # build
        _run_cmake_build(
            binary_dir=conf.get_osc_build_dir(),
            config=conf.get_osc_build_type(),
            concurrency=conf.concurrency
        )

        # test
        excluded_tests = [  # necessary in CI: no windowing system available
            'Renderer',
            'ShaderTest',
            'MaterialTest',
            'RegisteredDemoTabsTest',
            'RegisteredOpenSimCreatorTabs',
            'AddComponentPopup',
            'LoadingTab',
        ]
        excluded_tests_str = '|'.join(excluded_tests)
        _run(f'ctest --test-dir {conf.get_osc_build_dir()} -j {conf.concurrency} -E {excluded_tests_str}')

        # ensure final target is built - even if it isn't in ALL (e.g. `package`)
        _run_cmake_build(
            binary_dir=conf.get_osc_build_dir(),
            config=conf.get_osc_build_type(),
            concurrency=conf.concurrency,
            target=conf.build_target
        )

def main():
    logging.basicConfig(level=logging.DEBUG)

    # create build configuration with defaults (+ environment overrides)
    conf = BuildConfiguration()

    # parse CLI args
    parser = argparse.ArgumentParser()
    parser.add_argument('--jobs', '-j', type=int, default=conf.concurrency)
    parser.add_argument('--skip-osc', help='skip building OSC (handy if you plan on building OSC via Visual Studio)', default=conf.skip_osc, action='store_true')
    parser.add_argument('--build-dir', '-B', help='build binaries in the specified directory', type=str, default=conf.build_dir)
    parser.add_argument('--generator', '-G', help='set the build generator for cmake', type=str, default=conf.generator)
    parser.add_argument('--build-type', help='the type of build to produce (CMake string: Debug, Release, RelWithDebInfo, etc.)', type=str, default=conf.base_build_type)
    parser.add_argument('--system-version', help='specify the value of CMAKE_SYSTEM_VERSION (e.g. "10.0.26100.0", a specific Windows SDK)', type=str, default=conf.system_version)

    # overwrite build configuration with any CLI args
    args = parser.parse_args()
    conf.concurrency = args.jobs
    conf.skip_osc = args.skip_osc
    conf.build_dir = args.build_dir
    conf.generator = args.generator
    conf.base_build_type = args.build_type
    conf.system_version = args.system_version

    log_build_params(conf)
    build_osc_dependencies(conf)
    build_osc(conf)

if __name__ == "__main__":
    main()
