#!/usr/bin/env python3

import subprocess
from pathlib import Path

def main():
    build_type = "RelWithDebInfo"
    build_path = Path(f"build/{build_type}")
    prefix_path = Path(f'build/third_party/{build_type}/install').resolve()
    configure_args = [
        "cmake",
        "-G", "Ninja",
        "-S", ".",
        "-B", str(build_path),
        f"-DCMAKE_PREFIX_PATH={prefix_path}",
        f"-DCMAKE_BUILD_TYPE={build_type}"
    ]
    print(configure_args)
    subprocess.run(configure_args, check=True)

    build_args = ["cmake", "--build", str(build_path)]
    subprocess.run(build_args, check=True)

    test_args = ["ctest", "--output-on-failure", "--test-dir", str(build_path)]
    subprocess.run(test_args, check=True)

if __name__ == "__main__":
    main()
