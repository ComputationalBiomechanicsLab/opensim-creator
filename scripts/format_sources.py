#!/usr/bin/env python3

# recursively adds trailing newline to any files missing one in the given dir

import argparse
import os
import re

_blacklisted_directories = {'resources', 'MacOS', 'Linux', 'Windows'}

def num_leading_tabs(s):
    rv = 0
    for c in s:
        if c == '\t':
            rv += 1
        else:
            break
    return rv

def read_lines(path):
    with open(path, encoding='utf-8') as f:
        return f.read().splitlines()

def replace_leading_tabs(line):
    n = num_leading_tabs(line)
    if n:
        return (n*'    ' + line[n:], True)
    else:
        return (line, False)

def should_skip_dir(dirname):
    for blacklisted_directory in _blacklisted_directories:
        if blacklisted_directory in dirname:
            return True
    return False

# recursively tries to convert any tabs to spaces in the source tree
#
# (tabs sometimes slip into the source tree from editing files in terminals)
def replace_leading_tabs_with_spaces_in(dirpath):
    for root, subdirs, files in os.walk(dirpath):
        if should_skip_dir(root):
            continue
        for file in files:
            path = os.path.join(root, file)
            lines = read_lines(path)

            should_write = False
            new_lines = []
            for line in lines:
                content, changed = replace_leading_tabs(line)
                new_lines += [content]
                should_write = should_write or changed

            if should_write:
                new_content = "\n".join(new_lines) + "\n"  # trailing newline
                with open(path, "w", encoding='utf-8') as f:
                    f.write(new_content)

# recursively tries to strip whitespace from all files in a source
# tree
#
# (whitespace sometimes slips in from various text editors etc.)
def strip_whitespace_for_all_files_in(dirname):
    pattern  = re.compile(r"[ \t]+$", re.MULTILINE)
    for root, subdirs, files in os.walk(dirname):
        if should_skip_dir(root):
            continue
        for file in files:
            p = os.path.join(root, file)

            new_content = None
            with open(p, encoding='utf-8') as f:
                content = f.read()
                match = re.search(pattern, content)
                if match:
                    new_content = re.sub(pattern, "", content)
            if new_content:
                with open(p, "w", encoding='utf-8') as f:
                    f.write(new_content)

# recursively adds trailing newline to any files missing one in the given dir
def ensure_all_files_have_trailing_newline(dirpath):
    for root, subdirs, files in os.walk(dirpath):
        if should_skip_dir(root):
            continue
        for file in files:
            path = os.path.join(root, file)
            with open(path, "r", encoding='utf-8') as f:
                content = f.read()
            if not content.endswith("\n"):
                with open(path, "w", encoding='utf-8') as f:
                    f.write(content + "\n")



def main():
    p = argparse.ArgumentParser()
    p.add_argument(
        "dirpaths",
        help="directories containing files to recursively reformat",
        nargs="+"
    )
    parsed = p.parse_args()

    for p in parsed.dirpaths:
        ensure_all_files_have_trailing_newline(p)
        strip_whitespace_for_all_files_in(p)
        replace_leading_tabs_with_spaces_in(p)

if __name__ == '__main__':
    main()
