#!/usr/bin/env python3
#
# Checks that the specified binary file will work on at least
# the specified macOS version.

import argparse
import subprocess

parser = argparse.ArgumentParser(description='check that the given binary works on the desired macOS version')
parser.add_argument('min_os_version', help='the expected minimum os version')
parser.add_argument('binary', help='the binary to check')
args = parser.parse_args()

minos_version = None
for line in subprocess.check_output(["otool", "-l", args.binary], text=True, encoding="utf-8").splitlines():
    if "minos" in line:
        minos_version = line.split(" ")[-1].strip()

assert minos_version, f"Could not parse minimum OS version from output"
assert minos_version == args.min_os_version, f"{minos_version}: is not the expected minimum OS version ({args.min_os_version})"
