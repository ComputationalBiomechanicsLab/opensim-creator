#!/usr/bin/env python3

import argparse
import subprocess
import sys
from pathlib import Path

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("wheel_files", nargs="+")
    parser.add_argument("--test", action="store_true")
    args = parser.parse_args()

    twine_args = [sys.executable, "-m", "twine", "upload", "--non-interactive", "--verbose"]
    if args.test:
        twine_args += ["--repository-url", "https://test.pypi.org/legacy/"]
    twine_args += [str(Path(f).resolve()) for f in args.wheel_files]

    subprocess.run(twine_args, check=True)
    return 0

if __name__ == "__main__":
    exit(main())
