#!/usr/bin/env python3

# recursively adds trailing newline to any files missing one in the given dir

import argparse
import os
import re

_macro_fixups = {
    "ICON_FA_UNDO": "arrow-rotate-left.svg",
    "ICON_FA_REDO": "arrow-rotate-right.svg",
    "ICON_FA_WEIGHT": "weight-scale.svg",
    "ICON_FA_ARROWS_ALT": "arrows-up-down-left-right.svg",
    "ICON_FA_TIMES_CIRCLE": "circle-xmark.svg",
    "ICON_FA_TIMES": "xmark.svg",
    "ICON_FA_STEP_FORWARD": "forward-step.svg",
    "ICON_FA_STEP_BACKWARD": "backward-step.svg",
    "ICON_FA_SEARCH_PLUS": "magnifying-glass-plus.svg",
    "ICON_FA_SEARCH_MINUS": "magnifying-glass-minus.svg",
    "ICON_FA_SEARCH": "magnifying-glass.svg",
    "ICON_FA_SAVE": "floppy-disk.svg",
    "ICON_FA_PLUS_CIRCLE": "circle-plus.svg",
    "ICON_FA_MOUSE_POINTER": "arrow-pointer.svg",
    "ICON_FA_MAGIC": "wand-magic-sparkles.svg",
    "ICON_FA_INFO_CIRCLE": "circle-info.svg",
    "ICON_FA_HOME": "house-chimney.svg",
    "ICON_FA_COG": "gear.svg",
    "ICON_FA_FILE_ALT": "file.svg",
    "ICON_FA_COMPRESS_ARROWS_ALT": "arrows-to-dot.svg",
    "ICON_FA_DOT_CIRCLE": "circle-dot.svg",
    "ICON_FA_FAST_BACKWARD": "forward-fast.svg",
    "ICON_FA_FAST_FORWARD": "backward-fast.svg",
    "ICON_FA_EDIT": "pen-to-square.svg",
    "ICON_FA_EXPAND_ALT": "up-right-and-down-left-from-center.svg",
    "ICON_FA_EXPAND_ARROWS_ALT": "maximize.svg",
    "ICON_FA_EXTERNAL_LINK_ALT": "arrow-up-right-from-square.svg",
}

def find_all(name, path):
    result = set()
    for root, dirs, files in os.walk(path):
        if name in files:
            result.add(os.path.join(root, name))
    return result

def get_icons_used_in(dirpath):
    pattern = re.compile(r"ICON_FA.+?\b")
    els = set()

    for root, subdirs, files in os.walk(dirpath):
        for file in files:
            path = os.path.join(root, file)
            with open(path, "r") as f:
                for line in f:
                    for match in pattern.findall(line):
                        els.add(match)

    return els

def find_equivalent_svg(macro_name):
    if macro_name in _macro_fixups:
        fname = _macro_fixups[macro_name]
    else:
        fname = macro_name.replace("ICON_FA_", "").replace("_", "-").lower() + ".svg"

    results = find_all(fname, "C:\\Users\\adamk\\Downloads\\fontawesome-free-6.4.2-desktop\\fontawesome-free-6.4.2-desktop")
    for result in results:
        if "regular" in result:
            return result
    for result in results:
        if "solid" in result:
            return result
    return None

def main():
    p = argparse.ArgumentParser()
    p.add_argument(
        "dirs",
        help="directories containing files to recursively scan",
        nargs="+"
    )
    parsed = p.parse_args()

    for p in parsed.dirs:
        found = 0
        missing = 0
        for f in sorted(get_icons_used_in(p)):
            res = find_equivalent_svg(f)
            if res:
                found += 1
                print(res)
            else:
                missing += 1
                print(f"missing: {f}")
        print(f"found = {found} missing = {missing}")

if __name__ == '__main__':
    main()
