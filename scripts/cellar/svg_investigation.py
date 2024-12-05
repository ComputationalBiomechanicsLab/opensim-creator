#!/usr/bin/env python

# `svg_investigation.py`
#
# Throwaway script for figuring out the extents of all SVG icons used
# by OpenSim Creator

import glob
import xml.etree.ElementTree as ET

min_max_x = [10000,-10000]
min_max_y = [10000,-10000]
for svg_path in glob.glob(R'C:\Users\adamk\Desktop\opensim-creator\build_resources\icons\*.svg'):
    with open(svg_path, 'r') as f:
        tree = ET.ElementTree(ET.fromstring(f.read()))
    root = tree.getroot()
    viewbox = [int(v) for v in root.attrib.get('viewBox').split()]
    min_max_x = [min(min_max_x[0], viewbox[2]), max(min_max_x[1], viewbox[2])]
    min_max_y = [min(min_max_y[0], viewbox[3]), max(min_max_y[1], viewbox[3])]
print(min_max_x)  # prints [128, 640] (most fonts lie in the 512-wide range but some are extended to 18/16*512: https://docs.fontawesome.com/v5/web/use-kits/icon-design-guidelines)
print(min_max_y)  # probably prints [512, 512] (all fonts are 512 high)
