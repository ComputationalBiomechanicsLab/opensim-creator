#!/usr/bin/env python3

import argparse
import os
import re

def rename_header_files(dirpath):
    for root, subdirs, files in os.walk(dirpath):
        for file in files:
            if file.endswith('.hpp'):
                newfile = file.replace('.hpp', '.h')
                oldpath = os.path.join(root, file)
                newpath = os.path.join(root, newfile)
                os.rename(oldpath, newpath)

def fix_includes(dirpath):
    pattern = re.compile(r'.hpp', re.MULTILINE)
    for root, subdirs, files in os.walk(dirpath):
        for file in files:
            if file.endswith('.h') or file.endswith('.cpp') or file.endswith('.txt'):
                path = os.path.join(root, file)
                new_content = None
                with open(path) as f:
                    old_content = f.read()
                match = re.search(pattern, old_content)
                if match:
                    new_content = re.sub(pattern, ".h", old_content)
                if new_content:
                    with open(path, 'w') as f:
                        f.write(new_content)

def main():
    p = argparse.ArgumentParser()
    p.add_argument(
        "paths",
        help="directories containing sources to rename",
        nargs="+"
    )
    parsed = p.parse_args()

    for p in parsed.paths:
        rename_header_files(p)
        fix_includes(p)

if __name__ == '__main__':
    main()
