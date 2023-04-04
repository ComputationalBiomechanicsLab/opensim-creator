#!/usr/bin/env python3

# recursively tries to strip whitespace from all files in OSC's source
# tree
#
# (whitespace sometimes slips in from various text editors etc.)

import os
import re

# trailing whitespace
pattern = re.compile(r"[ \t]+$", re.MULTILINE)

# tabs
# pattern = re.compile(r"\t", re.MULTILINE)

for root, subdirs, files in os.walk("src"):
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