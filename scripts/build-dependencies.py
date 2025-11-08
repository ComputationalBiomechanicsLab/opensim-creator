#!/usr/bin/env python3

import argparse
import os
from pathlib import Path
import subprocess

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-type", default="Release")
    parser.add_argument("--generator", "-G", default=None)
    parser.add_argument("--jobs", "-j", type=int, default=os.cpu_count())
    args = parser.parse_args()

    build_type = args.build_type
    build_path = Path(f"build/third_party/{build_type}")
    configure_args = [
        "cmake",
        "-S", "third_party/",
        "-B", str(build_path),
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_INSTALL_PREFIX={build_path / 'install'}",
    ]
    if args.generator:
        configure_args += ["-G", args.generator]

    subprocess.run(configure_args, check=True)

    build_args = ["cmake", "--build", str(build_path), "-j", str(args.jobs)]
    subprocess.run(build_args, check=True)

if __name__ == "__main__":
    main()
