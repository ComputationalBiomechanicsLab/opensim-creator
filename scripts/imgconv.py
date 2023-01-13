# this depends on `Pillow` being installed
#
# - open SVG input in inkscape
# - save it as a high (e.g. 1024x1024) resolution PNG file (e.g. logoBIG.png)
# - input to this script, output as ICO/ICNS:
# -     e.g. `imgconv.py icon.svg -o icon.icns`
#
# - for ICO:
#     - script outputs a single ico file with sizes: [(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (255, 255)]
#     - (see: https://stackoverflow.com/questions/45507/is-there-a-python-library-for-generating-ico-files)

import argparse
import os
from PIL import Image

def convert(input_filename, output_filename):
    assert os.path.exists(input_filename)

    img = Image.open(input_filename)
    img.save(output_filename)

def main():
    parser = argparse.ArgumentParser(description="Convert image using pillow")
    parser.add_argument("input", help="Input File")
    parser.add_argument("-o", required=True, help="Ouput File")
    args = parser.parse_args()
    convert(args.input, args.o)

if __name__ == "__main__":
    main()