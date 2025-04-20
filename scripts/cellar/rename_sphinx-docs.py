#!/usr/bin/env python3

# rename-sphinx-docs: mini script that was used to rename parts of
# opensim-creator's sphinx documentation from "tut1", "tut2", etc.
# to "topidA", "topicB", etc.
#
# Kept in-tree in case future renames are desired

from pathlib import Path
import re

basename = 'preview-experimental-data'
pattern = r'_static/tut7_([^>`\n]+)'

with open(f'{basename}.rst', 'rt') as f:
    content = f.read()

matches = re.findall(pattern, content)
subbed = re.sub(pattern, f'_static/{basename}/\\1', content)
with open(f'{basename}.rst', 'wt') as f:
    f.write(subbed)

for m in matches:
    p = Path(f'_static/tut7_{m}')
    if p.exists():
        Path(f'_static/{basename}').mkdir(exist_ok=True)
        p.rename(f'_static/{basename}/{m}')
    else:
        print(f'skipping {p}')
