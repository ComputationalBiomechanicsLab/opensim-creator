#!/usr/bin/env python3

import argparse
import lief
from pathlib import Path

def list_exported_defined_symbols(binary_path : Path):
    assert binary_path.exists(), f"{binary_path}: does not exist"
    binary = lief.parse(binary_path)
    assert binary, f"{binary_path}: lief could not parse this binary"

    symbols = set()
    for symbol in binary.exported_symbols:
        symbols.add(symbol.name)
    return symbols

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary")
    parser.add_argument("permitted_symbols", nargs="*")
    parser.add_argument('--verbose', action='store_true')
    args = parser.parse_args()

    binary_path = Path(args.binary)
    permitted_symbols = set(args.permitted_symbols)

    symbols = list_exported_defined_symbols(binary_path)
    good_symbols = permitted_symbols.intersection(symbols)
    bad_symbols = symbols.difference(permitted_symbols)

    if bad_symbols:
        print(f"FAILURE: {binary_path} contains the following not-whitelisted symbols:")
        for bad_symbol in bad_symbols:
            print(f"    {bad_symbol}")

    if bad_symbols or args.verbose:
        print("Extra information:")
        print("    Permitted symbols:")
        if permitted_symbols:
            for permitted_symbol in permitted_symbols:
                print(f"        {permitted_symbol}")
        else:
            print("        (none)")

        print(f"    Symbols in {binary_path} that were permitted:")
        if good_symbols:
            for good_symbol in good_symbols:
                print(f"        {good_symbol}")
        else:
            print("        (none)")

    return 0 if len(bad_symbols) == 0 else 1

if __name__ == "__main__":
    import sys
    sys.exit(main())
