#!/usr/bin/env python3

from pathlib import Path
import subprocess

def main():
    build_type = "RelWithDebInfo"
    build_path = Path(f"build/third_party/{build_type}")
    configure_args = [
        "cmake",
        "-G", "Ninja",
        "-S", "third_party/",
        "-B", str(build_path),
        f"-DCMAKE_BUILD_TYPE={build_type}",
        f"-DCMAKE_INSTALL_PREFIX={build_path / 'install'}",
    ]
    subprocess.run(configure_args, check=True)

    build_args = ["cmake", "--build", str(build_path)]
    subprocess.run(build_args, check=True)

if __name__ == "__main__":
    main()
