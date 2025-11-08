#!/usr/bin/env python3

import argparse
import os
from pathlib import Path
import subprocess

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-type", help="build type (e.g. RelWithDebInfo, Debug)", default="Release")
    parser.add_argument("--generator", "-G", help="Generator that cmake should configure for (e.g. Ninja)", default=None)
    parser.add_argument("--jobs", "-j", type=int, default=os.cpu_count())
    args = parser.parse_args()
    build_type = args.build_type
    build_path = Path(f"build/{build_type}")
    prefix_path = Path(f'build/third_party/{build_type}/install').resolve()

    configure_args = [
        "cmake",
        "-S", ".",
        "-B", str(build_path),
        f"-DCMAKE_PREFIX_PATH={prefix_path}",
        f"-DCMAKE_BUILD_TYPE={build_type}"
    ]
    if args.generator:
        configure_args += ["-G", args.generator]

    # Configure
    subprocess.run(configure_args, check=True)

    # Build
    build_args = ["cmake", "--build", str(build_path), "-j", str(args.jobs)]
    subprocess.run(build_args, check=True)

    # Test
    test_args = ["ctest", "--output-on-failure", "--test-dir", str(build_path), "-j", str(args.jobs)]
    subprocess.run(test_args, check=True)

    # Package
    package_args = ["cmake", "--build", str(build_path), "-j", str(args.jobs), "--target", "package"]
    subprocess.run(package_args)

if __name__ == "__main__":
    main()
