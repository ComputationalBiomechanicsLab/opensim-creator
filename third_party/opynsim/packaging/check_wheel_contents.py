#!/usr/bin/env python3

import argparse
import re
import zipfile
from pathlib import Path

def check_wheel(wheel_path : Path, required_file_patterns):
    assert wheel_path.exists()

    found = set()
    missing = set()
    with zipfile.ZipFile(wheel_path, 'r') as whl:
        wheel_contents = whl.namelist()
        for f in required_file_patterns:
            if any([f.fullmatch(el) for el in wheel_contents]):
                found.add(f)
            else:
                missing.add(f)

    if missing:
        print(f"These files appear to be missing in {wheel_path}:")
        for f in missing:
            print(f"  - {f}")
        return False

    return True
def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("wheel_path")
    parser.add_argument("required_files", nargs="+")
    args = parser.parse_args()

    success = check_wheel(Path(args.wheel_path), {re.compile(f, re.IGNORECASE) for f in args.required_files})
    return 0 if success else 1

if __name__ == "__main__":
    exit(main())
