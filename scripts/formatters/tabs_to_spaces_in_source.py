#!/usr/bin/env python3

# recursively tries to convert any tabs to spaces in the source tree
#
# (tabs sometimes slip into the source tree from editing files in terminals)

import argparse
import os

def num_leading_tabs(s):
    rv = 0
    for c in s:
        if c == '\t':
            rv += 1
        else:
            break
    return rv

def read_lines(path):
    with open(path) as f:
        return f.read().splitlines()

def replace_leading_tabs(line):
    n = num_leading_tabs(line)
    if n:
        return (n*'    ' + line[n:], True)
    else:
        return (line, False)

def replace_leading_tabs_with_spaces_in(dirpath):
    for root, subdirs, files in os.walk(dirpath):
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
                with open(path, "w") as f:
                    f.write(new_content)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("dirpath")
    parsed = p.parse_args()
    replace_leading_tabs_with_spaces_in(parsed.dirpath)

if __name__ == '__main__':
    main()
