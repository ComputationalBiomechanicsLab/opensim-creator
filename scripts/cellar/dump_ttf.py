#!/usr/bin/env python3

import argparse
import os
from PIL import Image, ImageDraw, ImageFont

# https://stackoverflow.com/questions/43060479/how-to-get-the-font-pixel-height-using-pils-imagefont-class

def dump_ttf(path):
    assert os.path.exists(path)
    #print(PIL.__version__)
    font = ImageFont.truetype(path, 32)
    chars = "adam"
    for char in chars:
        w, h = font.getsize(char)
        print(f"getsize = {font.getsize(char)}")
        print(f"getbox = {font.getbbox(char)}")
        print(f"getlength = {font.getlength(char)}")
        img = Image.new("RGBA", (w, h))
        drawer = ImageDraw.Draw(img)
        drawer.text((-2, 0), str(char), font=font, fill="#000000")
        img.save(f"dumped_{char}.png")


def main():
    p = argparse.ArgumentParser()
    p.add_argument(
        "ttf",
        help="path to the ttf file"
    )
    parsed = p.parse_args()

    dump_ttf(parsed.ttf)

if __name__ == '__main__':
    main()
