#!/usr/bin/env python3

# recursively tries to strip whitespace from all files in a source
# tree
#
# (whitespace sometimes slips in from various text editors etc.)

import argparse
import os
import re

# trailing whitespace/tabs
pattern = re.compile(r"[ \t]+$", re.MULTILINE)

def strip_whitespace_for_all_files_in(dirname):
    for root, subdirs, files in os.walk(dirname):
        for file in files:
            p = os.path.join(root, file)

            new_content = None
            with open(p) as f:
                content = f.read()
                match = re.search(pattern, content)
                if match:
                    new_content = re.sub(pattern, "", content)
            if new_content:
                with open(p, "w") as f:
                    f.write(new_content)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("dirpath")
    parsed = p.parse_args()
    strip_whitespace_for_all_files_in(parsed.dirpath)

if __name__ == '__main__':
    main()
