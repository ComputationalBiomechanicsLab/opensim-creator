#!/usr/bin/env python3

import argparse
import subprocess

whitelisted_library_prefixes = [
    '/System/Library',
    '/usr/lib/libc++',
    '/usr/lib/libobjc',
    '/usr/lib/libSystem.B.dylib'
]

parser = argparse.ArgumentParser(description='check that the given binary (usually, osc) only uses whitelisted libraries')
parser.add_argument('binary', help='path to the binary')
args = parser.parse_args()

output = subprocess.check_output(f'otool -L {args.binary}', shell=True).decode('utf-8')
for line in output.splitlines():
    if line.startswith('\t'):
        assert any([line[1:].startswith(x) for x in whitelisted_library_prefixes]), f'{line} isnt a permitted dependency'
