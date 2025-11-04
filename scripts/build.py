#!/usr/bin/env python3
#
# Performs an end-to-end build, test, and package of OpenSim Creator

import argparse
import logging
import multiprocessing
import os
import pprint
import platform
import subprocess
import time

_dry_run = False

def _envvar_as_tristate(key : str, default=None):
    value = os.getenv(key, default)
    if value is None:
        return None
    else:
        return value.lower() in {"on", "true", "yes", "1"}

def _run(args, extra_env_vars=None):
    if extra_env_vars is None:
        extra_env_vars = {}

    logging.info(f"running: {' '.join(args)}")

    if not _dry_run:
        subprocess.run(args, check=True, env={**os.environ, **extra_env_vars})

def _log_dir_contents(path: str):
    logging.info(f"listing {path}")
    for el in os.listdir(path):
        logging.info(f"{path}/{el}")

def _default_generator():
    if platform.system() == "Windows":
        return "Visual Studio 17 2022"
    return "Unix Makefiles"

def _generator_requires_architecture_flag(generator):
    return generator is not None and ("Visual Studio" in generator)

def _is_multi_configuration_generator(generator):
    return generator is not None and ("Visual Studio" in generator or "Xcode" in generator or "Multi-Config" in generator)

def _run_cmake_configure(source_dir, binary_dir, generator, architecture, cache_variables, extra_env_vars=None):
    if extra_env_vars is None:
        extra_env_vars ={}

    args = ["cmake", "-S", source_dir, "-B", binary_dir]  # base arguments

    # append generator (-G) argument (if specified)
    if generator:
        args += ["-G", generator]
    # append architecture (-A) argument (if necessary)
    if _generator_requires_architecture_flag(generator):
        args += ["-A", architecture if architecture else "x64"]
    # append cache variables (-DK=V)
    for k, v in cache_variables.items():
        args += [f"-D{k}={v}"]

    _run(args, extra_env_vars)

def _run_cmake_build(binary_dir, generator, build_type, concurrency, target=None, extra_env_vars=None):
    if extra_env_vars is None:
        extra_env_vars = {}

    args = ["cmake", "--build", binary_dir, "--verbose", "-j", str(concurrency)]  # base arguments

    # append --config argument (if necessary)
    if _is_multi_configuration_generator(generator):
        args += ["--config", build_type]
    # append --target argument (if specified)
    if target:
        args += ["--target", target]

    _run(args, extra_env_vars)

def _run_ctest(test_dir, concurrency, excluded_tests=None, extra_env_vars=None):
    if excluded_tests is None:
        excluded_tests = []
    if extra_env_vars is None:
        extra_env_vars = {}

    args = ["ctest", "--test-dir", test_dir, "-j", str(concurrency)]  # base arguments
    if excluded_tests:
        args += ["-E", "|".join(excluded_tests)]
    _run(args, extra_env_vars)

# Represents the top-level, potentially caller-controlled, build configuration.
class BuildConfiguration:
    def __init__(self):
        self.base_build_type = os.getenv("OSC_BASE_BUILD_TYPE", "Release")
        self.concurrency = int(os.getenv("OSC_BUILD_CONCURRENCY", multiprocessing.cpu_count()))
        self.build_target = os.getenv("OSC_BUILD_TARGET", "package")
        self.build_docs = _envvar_as_tristate("OSC_BUILD_DOCS")
        self.generator = os.getenv("OSC_BUILD_GENERATOR", _default_generator())
        self.architecture = None
        self.system_version = os.getenv("OSC_SYSTEM_VERSION")
        self.build_dir = os.curdir
        self.codesign_enabled = None
        self.skip_osc = False
        self.skip_rendering_tests = False
        self.headless_mode = True
        self.allowed_final_target_build_attempts = 1
        self.seconds_between_final_target_build_attempts = 2

        # these can be `None`, meaning "fall back to `base_build_type` at runtime"
        self._osc_deps_build_type = os.getenv("OSC_DEPS_BUILD_TYPE")
        self._osc_build_type = os.getenv("OSC_BUILD_TYPE")

        # Mac-specific options
        self.osx_architectures = os.getenv("OSC_OSX_ARCHITECTURES")
        self.osx_deployment_target = os.getenv("OSC_TARGET_OSX_VERSION")
        self.osx_sysroot = None if self.osx_deployment_target is None else f"/Library/Developer/CommandLineTools/SDKs/MacOSX{self.osx_deployment_target}.sdk/"

    def __repr__(self):
        return pprint.pformat(vars(self))

    def get_dependencies_build_dir(self):
        return os.path.join(self.build_dir, f"third_party-build-{self.get_osc_deps_build_type()}")

    def get_dependencies_install_dir(self):
        return os.path.join(self.build_dir, f"third_party-install-{self.get_osc_deps_build_type()}")

    def get_osc_build_dir(self):
        return os.path.join(self.build_dir, f"build/{self.get_osc_build_type()}")

    def get_osc_deps_build_type(self):
        return self._osc_deps_build_type or self.base_build_type

    def get_osc_build_type(self):
        return self._osc_build_type or self.base_build_type

    def get_dependencies_cmake_cache_variables(self):
        rv = {
            "CMAKE_BUILD_TYPE": self.get_osc_deps_build_type(),
            "CMAKE_INSTALL_PREFIX": self.get_dependencies_install_dir(),
        }
        if self.system_version:
            rv["CMAKE_SYSTEM_VERSION"] = self.system_version
        if self.osx_architectures:
            rv['CMAKE_OSX_ARCHITECTURES'] = self.osx_architectures
        if self.osx_deployment_target:
            rv['CMAKE_OSX_DEPLOYMENT_TARGET'] = self.osx_deployment_target
        if self.osx_sysroot:
            rv['CMAKE_OSX_SYSROOT'] = self.osx_sysroot

        return rv

    def get_osc_cmake_cache_variables(self):
        # calculate cache variables
        rv = {
            "CMAKE_BUILD_TYPE": self.get_osc_build_type(),
            "CMAKE_PREFIX_PATH": os.path.abspath(self.get_dependencies_install_dir()),
        }
        if self.build_docs is not None:
            rv["OSC_BUILD_DOCS"] = "ON" if self.build_docs else "OFF"
        if self.codesign_enabled is not None:
            rv["OSC_CODESIGN_ENABLED"] = "ON" if self.codesign_enabled else "OFF"
        if self.system_version:
            rv["CMAKE_SYSTEM_VERSION"] = self.system_version
        if self.osx_architectures:
            rv['CMAKE_OSX_ARCHITECTURES'] = self.osx_architectures
        if self.osx_deployment_target:
            rv['CMAKE_OSX_DEPLOYMENT_TARGET'] = self.osx_deployment_target
        if self.osx_sysroot:
            rv['CMAKE_OSX_SYSROOT'] = self.osx_sysroot

        return rv

    def get_osc_ctest_extra_environment_variables(self):
        rv = {}
        if self.headless_mode:
            rv["OSC_INTERNAL_HIDE_WINDOW"] = "1"
        return rv

    def get_excluded_tests(self):
        if not self.skip_rendering_tests:
            return []  # no tests excluded
        else:
            return [
                "Graphics",
                "MeshDepthWritingMaterialFixture",
                "MeshNormalVectorsMaterialFixture",
                "ShaderTest",
                "MaterialTest",
                "RegisteredDemoTabsTest",
                "RegisteredOpenSimCreatorTabs",
                "AddComponentPopup",
                "LoadingTab",
            ]

# Represents a section (grouping, substep) of the build
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
        # configure
        _run_cmake_configure(
            source_dir="third_party",
            binary_dir=conf.get_dependencies_build_dir(),
            generator=conf.generator,
            architecture=conf.architecture,
            cache_variables=conf.get_dependencies_cmake_cache_variables()
        )
        # build
        _run_cmake_build(
            binary_dir=conf.get_dependencies_build_dir(),
            generator=conf.generator,
            build_type=conf.get_osc_deps_build_type(),
            concurrency=conf.concurrency
        )

    with Section("log osc dependencies build/install dirs"):
        _log_dir_contents(conf.get_dependencies_build_dir())
        _log_dir_contents(conf.get_dependencies_install_dir())

def build_osc(conf: BuildConfiguration):
    if conf.skip_osc:
        logging.info("skip_osc was set: skipping the OSC build")
        return

    with Section("build osc"):
        # configure
        _run_cmake_configure(
            source_dir=".",
            binary_dir=conf.get_osc_build_dir(),
            generator=conf.generator,
            architecture=conf.architecture,
            cache_variables=conf.get_osc_cmake_cache_variables()
        )
        # build
        _run_cmake_build(
            binary_dir=conf.get_osc_build_dir(),
            generator=conf.generator,
            build_type=conf.get_osc_build_type(),
            concurrency=conf.concurrency
        )
        # test
        _run_ctest(
            test_dir=conf.get_osc_build_dir(),
            concurrency=conf.concurrency,
            excluded_tests=conf.get_excluded_tests(),
            extra_env_vars=conf.get_osc_ctest_extra_environment_variables()
        )

        # build final target (which might not be in ALL, e.g. `package`)
        #
        # multiple attempts might be necessary in certain situations. E.g. MacOS via
        # a GitHub action runner can spuriously fail for no good reason
        for i in range(0, conf.allowed_final_target_build_attempts):
            try:
                _run_cmake_build(
                    binary_dir=conf.get_osc_build_dir(),
                    generator=conf.generator,
                    build_type=conf.get_osc_build_type(),
                    concurrency=conf.concurrency,
                    target=conf.build_target
                )
                break
            except Exception as e:
                if i+1 == conf.allowed_final_target_build_attempts:
                    raise
                else:
                    logging.warning(f"error running final build step (will reattempt): {str(e)}")

            time.sleep(conf.seconds_between_final_target_build_attempts)

        if platform.system() == "Darwin":
            # On Mac, ensure all runtime dependencies are within a known whitelist
            _run(["./scripts/test_macos-dependencies.py", os.path.join(conf.get_osc_build_dir(), "osc/osc.app/Contents/MacOS/osc")])
        if conf.osx_deployment_target:
            # If the caller specified a specific deployment target, ensure the binary has that target
            _run(["./scripts/test_macos-uses-sdk.py", conf.osx_deployment_target, os.path.join(conf.get_osc_build_dir(), "osc/osc.app/Contents/MacOS/osc")])

def main():
    logging.basicConfig(level=logging.DEBUG)

    # create build configuration with defaults (+ environment overrides)
    conf = BuildConfiguration()

    # parse CLI args
    parser = argparse.ArgumentParser()
    parser.add_argument("--jobs", "-j", type=int, default=conf.concurrency)
    parser.add_argument("--skip-osc", help="skip building OSC (handy if you plan on building OSC via Visual Studio)", default=conf.skip_osc, action="store_true")
    parser.add_argument("--build-dir", "-B", help="build binaries in the specified directory", type=str, default=conf.build_dir)
    parser.add_argument("--generator", "-G", help="set the build generator for cmake", type=str, default=conf.generator)
    parser.add_argument("--build-type", help="the type of build to produce (CMake string: Debug, Release, RelWithDebInfo, etc.)", type=str, default=conf.base_build_type)
    parser.add_argument("--system-version", help="specify the value of CMAKE_SYSTEM_VERSION (e.g. '10.0.26100.0', a specific Windows SDK)", type=str, default=conf.system_version)
    parser.add_argument("--codesign-enabled", help="enable signing resulting binaries/package", default=conf.codesign_enabled, action="store_true")
    parser.add_argument("--skip-rendering-tests", help="skip tests that use the rendering subsystem", default=conf.skip_rendering_tests, action="store_true")
    parser.add_argument("--headless", help="run tests is headless mode (i.e. don't show UI during UI tests)", default=conf.headless_mode, action="store_true")
    parser.add_argument("--allowed-final-target-build-attempts", help="the number of times the final build step is allowed to fail (can be handy when the packaging system is flakey)", type=int, default=conf.allowed_final_target_build_attempts)
    parser.add_argument("--seconds-between-final-target-build-attempts", help="the number of seconds that should be slept between final target build attempts", default=conf.seconds_between_final_target_build_attempts, type=int)
    if platform.system() == "Darwin":
        parser.add_argument("--osx-architectures", help="set which architectures to build for on (Mac-specific)")
        parser.add_argument("--osx-deployment-target", help="set the version of MacOS that the build should target", default=conf.osx_deployment_target)
        parser.add_argument("--osx-sysroot", help="set the MacOSX SDK sysroot that should be used to build everything", default=conf.osx_sysroot)

    # overwrite build configuration with any CLI args
    args = parser.parse_args()
    conf.concurrency = args.jobs
    conf.skip_osc = args.skip_osc
    conf.build_dir = args.build_dir
    conf.generator = args.generator
    conf.base_build_type = args.build_type
    conf.system_version = args.system_version
    conf.codesign_enabled = args.codesign_enabled
    conf.skip_rendering_tests = args.skip_rendering_tests
    conf.headless_mode = args.headless
    conf.allowed_final_target_build_attempts = args.allowed_final_target_build_attempts
    conf.seconds_between_final_target_build_attempts = args.seconds_between_final_target_build_attempts
    conf.osx_architectures = args.osx_architectures
    conf.osx_deployment_target = args.osx_deployment_target
    conf.osx_sysroot = args.osx_sysroot

    log_build_params(conf)
    build_osc_dependencies(conf)
    build_osc(conf)

if __name__ == "__main__":
    main()
