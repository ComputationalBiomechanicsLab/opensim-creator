#!/usr/bin/env python3

# recursively adds trailing newline to any files missing one in the given dir

import argparse
import os

def ensure_all_files_have_trailing_newline(dirpath):
    for root, subdirs, files in os.walk("src"):
        for file in files:
            path = os.path.join(root, file)
            with open(path, "r") as f:
                content = f.read()
            if not content.endswith("\n"):
                with open(path, "w") as f:
                    f.write(content + "\n")

def main():
    p = argparse.ArgumentParser()
    p.add_argument("dirpath")
    parsed = p.parse_args()
    ensure_all_files_have_trailing_newline(parsed.dirpath)

if __name__ == '__main__':
    main()
