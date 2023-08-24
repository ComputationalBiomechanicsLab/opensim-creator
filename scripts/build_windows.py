import logging
import os
import pprint
import subprocess

def _is_truthy_envvar(s : str):
    sl = s.lower()
    return sl in {"on", "true", "yes", "1"}

def _run(s: str):
    logging.info(f"running: {s}")
    subprocess.run(s, check=True)

def _log_dir_contents(path: str):
    logging.info(f"listing {path}")
    for el in os.listdir(path):
        logging.info(f"{path}/{el}")

class BuildConfiguration:

    def __init__(self):
        _default_build_type = "RelWithDebInfo"
        _default_build_concurrency = 1  # OpenSim crashes CI if this is too large
        _default_build_target = "package"
        _default_build_docs = "OFF"

        self.osc_base_build_type = os.getenv("OSC_BASE_BUILD_TYPE", _default_build_type)
        self.osc_deps_build_type = os.getenv("OSC_DEPS_BUILD_TYPE", self.osc_base_build_type)
        self.osc_build_type = os.getenv("OSC_BUILD_TYPE", self.osc_base_build_type)
        self.concurrency = int(os.getenv("OSC_BUILD_CONCURRENCY", _default_build_concurrency))
        self.build_target = os.getenv("OSC_BUILD_TARGET", _default_build_target)
        self.build_docs = _is_truthy_envvar(os.getenv("OSC_BUILD_DOCS", _default_build_docs))
        self.generator_flags = f'-G"Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE={self.osc_build_type}'
        self.dependencies_build_dir = "osc-dependencies-build"
        self.dependencies_install_dir = "osc-dependencies-install"
        self.osc_build_dir = "osc-build"

    def __repr__(self):
        return pprint.pformat(vars(self))

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
        _run(f'cmake -S third_party/ -B "{conf.dependencies_build_dir}" {conf.generator_flags} -DCMAKE_INSTALL_PREFIX="{conf.dependencies_install_dir}"')
        _run(f'cmake --build {conf.dependencies_build_dir} --config {conf.osc_deps_build_type} -j{conf.concurrency}')

    with Section("log osc dependencies build/install dirs"):
        _log_dir_contents(conf.dependencies_build_dir)
        _log_dir_contents(conf.dependencies_install_dir)

def build_osc(conf: BuildConfiguration):
    with Section("build osc"):
        test_osc_path = os.path.join(conf.osc_build_dir, 'tests', 'OpenSimCreator', conf.osc_build_type, 'testopensimcreator')
        test_oscar_path = os.path.join(conf.osc_build_dir, 'tests', 'oscar', conf.osc_build_type, 'testoscar')
        other_build_args = f'--config {conf.osc_build_type} -j{conf.concurrency}'

        # configure
        _run(f'cmake -S . -B {conf.osc_build_dir} {conf.generator_flags} -DCMAKE_PREFIX_PATH={os.path.abspath(conf.dependencies_install_dir)} -DOSC_BUILD_DOCS={"ON" if conf.build_docs else "OFF"}')

        # build+run oscar test suite
        _run(f'cmake --build {conf.osc_build_dir} --target testoscar {other_build_args}')
        _run(f'{test_oscar_path} --gtest_filter="-Renderer*')

        # build+run OpenSimCreator test suite
        _run(f'cmake --build {conf.osc_build_dir} --target testopensimcreator {other_build_args}')
        _run(f'{test_osc_path} --gtest_filter="-Renderer*"')

        # build final output target (usually, the installer)
        _run(f'cmake --build {conf.osc_build_dir} --target {conf.build_target} {other_build_args}')

def main():
    conf = BuildConfiguration()

    log_build_params(conf)
    ensure_submodules_are_up_to_date(conf)
    install_system_dependencies(conf)
    build_osc_dependencies(conf)
    build_osc(conf)

if __name__ == "__main__":
    main()
