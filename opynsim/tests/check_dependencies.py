#!/usr/bin/env python3

# Checks the provided binary against a whitelist to ensure that extra
# runtime libraries haven't slipped through the build.

import argparse
import lief
import re

parser = argparse.ArgumentParser()
parser.add_argument("binary_to_test", help="Filesystem path to a binary that should be checked")
parser.add_argument('whitelist', help="Filesystem path to a text file containing a whitelisted runtime dependency on each line")
parser.add_argument('--verbose', action='store_true')
args = parser.parse_args()

# Load+Parse the binary
binary = lief.parse(args.binary_to_test)
if binary is None:
    raise RuntimeError(f"Could not parse {args.binary_to_test}")

# Load+Parse the whitelist as a list of regex patterns
patterns = []
with open(args.whitelist, 'rt') as f:
    for line in f:
        line = line.strip()
        if line and not line.startswith("#"):
            patterns.append(re.compile(line, re.IGNORECASE))

good_deps = set()
bad_deps = set()
for dep in binary.imports:
    dep_name = dep.name.lower()
    if any(pattern.fullmatch(dep_name) for pattern in patterns):
        good_deps.add(dep_name)
    else:
        bad_deps.add(dep_name)

if bad_deps:
    print("FAILURE: the binary contains the following not-whitelisted dependencies:")
    for bad_dep in bad_deps:
        print(f"    {bad_dep}")

if bad_deps or args.verbose:
    print("Extra information:")
    print("    Patterns in the whitelist:")
    for pattern in patterns:
        print(f"        {pattern}")
    print("    Dependencies that passed the whitelist:")
    for good_dep in good_deps:
        print(f"        {good_dep}")

exit(1 if bad_deps else 0)
