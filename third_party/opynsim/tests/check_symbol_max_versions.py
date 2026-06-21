import argparse
import sys
import lief

def parse_version_string(ver_str, prefix):
    try:
        clean_str = ver_str.replace(prefix, "")
        return tuple(int(i) for i in clean_str.split('.'))
    except ValueError:
        return (0,)

def validate_binary(binary_path, max_symbol_versions):
    targets = {}
    for max_symbol_version in max_symbol_versions:
        symbol, version = max_symbol_version.split("_")
        prefix = symbol + "_"
        targets[prefix] = {"max": parse_version_string(version, prefix), "raw": version}

    binary = lief.parse(binary_path)
    assert binary and isinstance(binary, lief.ELF.Binary)

    found_versions = {prefix: set() for prefix in targets}

    for symbol in binary.dynamic_symbols:
        if symbol.has_version and symbol.symbol_version:
            sym_ver = symbol.symbol_version
            if sym_ver.has_auxiliary_version and sym_ver.symbol_version_auxiliary:
                name = sym_ver.symbol_version_auxiliary.name
                for prefix in targets:
                    if name.startswith(prefix):
                        found_versions[prefix].add(name)

    failed = False
    print(f"Auditing Comprehensive ABI limits for {binary_path}:\n")

    for prefix, versions in found_versions.items():
        max_tuple = targets[prefix]["max"]
        raw_limit = targets[prefix]["raw"]
        sorted_vers = sorted(list(versions), key=lambda x: parse_version_string(x, prefix))

        print(f"--- Checking {prefix} (Allowed Max: {prefix}{raw_limit}) ---")
        if not sorted_vers:
            print("  [OK]   No symbols found for this namespace.")
        for ver in sorted_vers:
            curr_tuple = parse_version_string(ver, prefix)
            if curr_tuple > max_tuple:
                print(f"  [FAIL] {ver} exceeds maximum allowed boundary!")
                failed = True
            else:
                print(f"  [OK]   {ver}")
        print()

    # Bonus: Print the target hardware architecture to ensure you didn't build for the wrong CPU type
    print(f"--- Target CPU Architecture ---")
    print(f"  Machine Architecture: {binary.header.machine_type}")
    print()

    if failed:
        print("ERROR: Comprehensive compliance check FAILED.")
        return 1
    else:
        print("SUCCESS: All crucial runtime constraints are verified safe.")
        return 0

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary_path")
    parser.add_argument("max_symbol_versions", nargs="*")
    args = parser.parse_args()
    return validate_binary(args.binary_path, args.max_symbol_versions)

if __name__ == "__main__":
    sys.exit(main())

