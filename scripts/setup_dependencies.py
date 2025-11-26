#!/usr/bin/env python3

import argparse
import sys
import subprocess
from pathlib import Path

def run(cmd, **kwargs):
    """Run a shell command, echo it, and fail on error."""
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True, **kwargs)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("build_type", nargs="*", default=["Release"])
    args = parser.parse_args()

    for config in args.build_type:
        print(f"=== Building dependencies for configuration: {config} ===")

        third_party_dir = Path("third_party")

        # Equivalent to: cd third_party/ && cmake ...
        run([
            "cmake",
            "--workflow",
            "--preset",
            f"OPynSim_third-party_{config}"
        ], cwd=third_party_dir)

if __name__ == "__main__":
    main()

