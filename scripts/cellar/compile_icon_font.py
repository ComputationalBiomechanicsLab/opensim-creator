#!/usr/bin/env python

# `compile_icon_font.py`
#
# This is a throwaway script that ensures each #define in the legacy
# OpenSim Creator icon font usage (e.g. OSC_ICON_UNDO) can be
# unambiguously found in FontAwesome's SVG download and linked to
# a unicode codepoint.
#
# It's throwaway because, once the JSON is created, the job is done
# and it will be manually edited, or edited with a different throwaway,
# later.

import json
import os
import shutil

# Font mappings, as-per OpenSim Creator's `IconCodepoints.h` header
_font_mappings = {
    "UNDO": {"file": "TODO", "unicode": "f0e2" },
    "REDO": {"file": "TODO", "unicode": "f01e" },
    "CARET_DOWN": {"file": "TODO", "unicode": "f0d7"},
    "SPIDER": {"file": "TODO", "unicode": "f717"},
    "COOKIE": {"file": "TODO", "unicode": "f563"},
    "BEZIER_CURVE": {"file": "TODO", "unicode": "f55b"},
    "ARROWS_ALT": {"file": "TODO", "unicode": "f0b2" },
    "EXPAND_ARROWS_ALT": {"file": "TODO", "unicode": "f424"},
    "EXPAND": {"file": "TODO", "unicode": "f065"},
    "EXPAND_ALT": {"file": "TODO", "unicode": "f424"},
    "COMPRESS_ARROWS_ALT": {"file": "TODO", "unicode": "f78c"},
    "DOT_CIRCLE": {"file": "TODO", "unicode": "f192"},
    "CIRCLE": {"file": "TODO", "unicode": "f111"},
    "LINK": {"file": "TODO", "unicode": "f0c1"},
    "CUBE": {"file": "TODO", "unicode": "f1b2"},
    "CUBES": {"file": "TODO", "unicode": "f1b3"},
    "MAP_PIN": {"file": "TODO", "unicode": "f276"},
    "MAGIC": {"file": "TODO", "unicode": "f0d0"},
    "FILE": {"file": "TODO", "unicode": "f15b"},
    "FILE_IMPORT": {"file": "TODO", "unicode": "f56f"},
    "FILE_EXPORT": {"file": "TODO", "unicode": "f56e"},
    "FOLDER": {"file": "TODO", "unicode": "f07b"},
    "FOLDER_OPEN": {"file": "TODO", "unicode": "f07c"},
    "SAVE": {"file": "TODO", "unicode": "f0c7"},
    "LOCK": {"file": "TODO", "unicode": "f023"},
    "UNLOCK": {"file": "TODO", "unicode": "f09c"},
    "TRASH": {"file": "TODO", "unicode": "f1f8"},
    "INFO_CIRCLE": {"file": "TODO", "unicode": "f05a"},
    "ERASER": {"file": "TODO", "unicode": "f12d"},
    "CHECK": {"file": "TODO", "unicode": "f00c"},
    "EXCLAMATION": {"file": "exclamation.svg", "unicode": "f12a"},
    "TIMES": {"file": "TODO", "unicode": "f00d"},
    "COG": {"file": "TODO", "unicode": "f013"},
    "PLAY": {"file": "TODO", "unicode": "f04b"},
    "EDIT": {"file": "TODO", "unicode": "f044"},
    "UPLOAD": {"file": "TODO", "unicode": "f574"},
    "COPY": {"file": "TODO", "unicode": "f0c5"},
    "RECYCLE": {"file": "TODO", "unicode": "f1b8"},
    "TIMES_CIRCLE": {"file": "TODO", "unicode": "f057"},
    "BARS": {"file": "TODO", "unicode": "f0c9"},
    "FAST_BACKWARD": {"file": "TODO", "unicode": "f049"},
    "STEP_BACKWARD": {"file": "TODO", "unicode": "f048"},
    "PAUSE": {"file": "TODO", "unicode": "f04c"},
    "STEP_FORWARD": {"file": "TODO", "unicode": "f051"},
    "FAST_FORWARD": {"file": "TODO", "unicode": "f050"},
    "PLUS": {"file": "plus.svg", "unicode": "f067"},
    "PLUS_CIRCLE": {"file": "TODO", "unicode": "f055"},
    "ARROW_UP": {"file": "TODO", "unicode": "f062"},
    "ARROW_DOWN": {"file": "TODO", "unicode": "f063"},
    "ARROW_LEFT": {"file": "TODO", "unicode": "f060"},
    "ARROW_RIGHT": {"file": "TODO", "unicode": "f061"},
    "EYE": {"file": "TODO", "unicode": "f06e"},
    "BOLT": {"file": "TODO", "unicode": "f0e7"},
    "MOUSE_POINTER": {"file": "TODO", "unicode": "f245"},
    "BORDER_ALL": {"file": "TODO", "unicode": "f84c"},
    "DIVIDE": {"file": "TODO", "unicode": "f529"},
    "WEIGHT": {"file": "TODO", "unicode": "f496"},
    "CAMERA": {"file": "TODO", "unicode": "f030"},
    "EXTERNAL_LINK_ALT": {"file": "TODO", "unicode": "f35d"},
    "PAINT_ROLLER": {"file": "TODO", "unicode": "f5aa"},
    "SEARCH":  {"file": "TODO", "unicode": "f002"},
    "SEARCH_MINUS": {"file": "TODO", "unicode": "f010"},
    "SEARCH_PLUS": {"file": "TODO", "unicode": "f00e"},
    "CALCULATOR": {"file": "TODO", "unicode": "f1ec"},
    "GRIP_LINES": {"file": "TODO", "unicode": "f7a4"},
    "HOME": {"file": "TODO", "unicode": "f015"},
    "BOOK": {"file": "TODO", "unicode": "f02d"},
    "CHART_LINE": {"file": "TODO", "unicode": "f201"},
    "CLIPBOARD": {"file": "TODO", "unicode": "f328"},
    "WINDOW_RESTORE": {"file": "TODO", "unicode": "f2d2"},
}

# use FontAwesome's metadata to join OpenSim Creator's definitions with
# FontAwesome's via the unicode codepoint.
with open(R'C:\Users\adamk\Downloads\fontawesome-free-6.7.1-desktop\fontawesome-free-6.7.1-desktop\metadata\icons.json', 'r') as f:
    data = json.load(f)

for k, v in _font_mappings.items():
    if v['file'] != 'TODO':
        filename = v['file']
    else:
        unicode = v['unicode']
        filename = None
        for fak, fav in data.items():
            if fav['unicode'] == v['unicode']:
                filename = f'{fak}.svg'
        assert filename != None, f'{k}'

    # Ensure the svg can be found and emit it to the JSON
    fspath = R'C:\Users\adamk\Downloads\fontawesome-free-6.7.1-desktop\fontawesome-free-6.7.1-desktop\svgs\solid\HERE'.replace('HERE', filename)
    assert os.path.exists(fspath), fspath
    shutil.copy(fspath, R'C:\Users\adamk\Desktop\opensim-creator\build_resources\icons')
    v['file'] = filename
    v['author'] = 'Font Awesome'

# Write the JSON to the `build_resources` dir
with open(R'C:\Users\adamk\Desktop\opensim-creator\build_resources\font-mappings.json', 'w') as f:
    json.dump(_font_mappings, f, indent=2)
