#!/usr/bin/env python
#
# Uses `build_resources/font-mappings.json` and FontForge to compile OpenSim
# Creator's raw SVG icons into an icon font.
#
# inspired by: https://github.com/RobertWinslow/Simple-SVG-to-Font-with-Fontforge
#
# The glyph/font attributes that are hard-coded here are based (roughly) on
# FontAwesome, which defines icons on a 16x16 grid (mostly). Wider icons
# are 20x16. SVGs from FontAwesome have viewboxes that map 16 --> 512px.
# E.g. an SVG with a viewbox that's 512 high maps to 16 font pixel units
# high (all fonts are 512 high). An SVG with a width of 640 maps to a
# font width of 20 pixels.
#
# note: in FontAwesome, default horizontal advance: 512, upper limit of icon advance: 640
#
# ON WINDOWS: you'll have to use `run_fontforge.bat` to run a python
# interpreter (the one that's installed alongside FontForge Windows edition)
# that can actually `import fontforge`.

import fontforge
import json
import os

# Create a new font
font = fontforge.font()
font.fontname = 'OpenSimCreatorIcons'
font.familyname = 'OpenSim Creator Icons'
font.fullname = 'OpenSim Creator Icons'
font.version = 'M/A'
font.copyright = 'Copyright (c) TODO'
font.comment = 'Compiled from a mixture of FontAwesome glyphs and OpenSim Creator glyphs (probably ;))'
font.default_base_filename = 'OpenSimCreatorIcons'
font.ascent = 448
font.descent = 64
font.em = 512
font.upos = -49
font.uwidth = 25

# Load `font-mappings.json` to figure out which SVGs to encode to which unicode
# codepoints in the font.
with open(R'C:\Users\adamk\Desktop\opensim-creator\build_resources\font-mappings.json', 'r') as f:
    mappings = json.load(f)

unicode_ints = []
for k, v in mappings.items():
    svg_file = R'C:\Users\adamk\Desktop\opensim-creator\build_resources\icons' + '\\' + v['file']
    assert os.path.exists(svg_file), svg_file
    unicode = v['unicode']
    unicode_ints.append(int(unicode, 16))  # for later

    # create character
    char = font.createChar(int(unicode, 16), f'u{unicode}')
    char.importOutlines(svg_file)

# Ensure all glyphs have desired widths
font.selection.all()
font.autoWidth(512, minBearing=512, maxBearing=640)

# Write the font over OpenSim Creator's runtime font
#
# (this is ultimately what is installed on users' computers)
font.generate(R'C:\Users\adamk\Desktop\opensim-creator\resources\oscar\fonts\OpenSimCreatorIconFont.ttf')

# emit `IconCodepoints.h`
with open(R'C:\Users\adamk\Desktop\opensim-creator\libopensimcreator\Platform\IconCodepoints.h', 'w') as f:
    f.write("// This was generated by OpenSim Creator's `compile_svgs_to_ttf.py` script\n")
    f.write('\n')
    f.write(f'#define OSC_ICON_MIN {hex(min(unicode_ints))}\n')
    f.write(f'#define OSC_ICON_MAX {hex(max(unicode_ints))}\n')
    f.write('\n')
    for k, v in mappings.items():
        unicode = v['unicode']
        hex_encoded_glyph = str(chr(int(unicode, 16)).encode('utf-8'))[2:-1]
        f.write(f'#define OSC_ICON_{k} "{hex_encoded_glyph}"  // U+{unicode}\n')
