#!/usr/bin/env python3


import argparse
import subprocess

parser = argparse.ArgumentParser(description='check that the given binary uses the correct MacOS SDK')
parser.add_argument('expected_sdk_version', help='the expected SDK version')
parser.add_argument('binary', help='the binary to check')
args = parser.parse_args()

sdk_version = subprocess.check_output(f'otool -l {args.binary} | grep sdk', shell=True).decode('utf-8').split(' ')[-1].strip()
assert sdk_version == args.expected_sdk_version, f'{sdk_version} is not the expected SDK version ({args.expected_sdk_version})'

