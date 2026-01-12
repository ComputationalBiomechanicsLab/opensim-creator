#!/usr/bin/env python3
#
# Checks that the specified binary uses the specified version of the
# MacOS SDK. This is handy for double-checking that the build system
# is using the correct SDK, which can impact which versions of MacOS
# can run the binary.

import argparse
import subprocess

parser = argparse.ArgumentParser(description='check that the given binary uses the correct MacOS SDK')
parser.add_argument('expected_sdk_version', help='the expected SDK version')
parser.add_argument('binary', help='the binary to check')
args = parser.parse_args()

sdk_version = None
minos_version = None
for line in subprocess.check_output(["otool", "-l", args.binary], text=True, encoding="utf-8").splitlines():
    if "sdk" in line:
        sdk_version = line.split(" ")[-1].strip()
    if "minos" in line:
        minos_version = line.split(" ")[-1].strip()

assert sdk_version, f"Could not parse sdk version from output"
assert sdk_version == args.expected_sdk_version, f'{sdk_version} is not the expected SDK version ({args.expected_sdk_version})'
assert minos_version, f"Could not parse minimum OS version from output"
assert minos_version == args.expected_sdk_version, f"{minos_version}: is not the expected minimum OS version ({args.expected_sdk_version})"
