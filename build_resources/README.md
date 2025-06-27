# `build_resources`

> Resources that are used by the OpenSim Creator build, or development.

This directory contains files that are used by OpenSim Creator to build the
final software product.

*How* they're used is resource-dependent. Here's a basic explanation of each
resource:

- `font-mappings.json`: maps icon codepoint identifiers, which are used in the
   OpenSim Creator source code (e.g. `OSC_ICON_LIGHTNING`) to a combination of
   a unicode codepoint (used to compile the runtime icon font) and svg file (in
   `resources/OpenSimCreator/icons/`). This file was initially generated via a
   script (`compile_svgs_to_ttf.py`) and subsequently manually updated whenever a
   new icon comes along.

- `icons/`: SVG icons, which are combined with `font-mappings.json` to create
  the OpenSim Creator icon font using a [FontForge](https://fontforge.org) script
  (`compile_svgs_to_ttf.py`). The license for each icon can be determined by reading
  the `author` field in `font-mappings.json`. If it says "Font Awesome" then the
  license is the [FontAwesome Free License](https://fontawesome.com/v4/license/). If
  it says "Adam Kewley" then you can assume it's a [Creative Commons Attribution 4.0 International License][cc-by].
