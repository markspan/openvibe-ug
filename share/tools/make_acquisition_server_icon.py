#!/usr/bin/env python3
# UG fork: new file, not in upstream OpenViBE. See UG.md, 2.37.
"""
Draws the acquisition server's icon, share/ov-acquisition-server-icon.ico.

It speaks the language of Designer's icon (share/ov-icon.ico): a white tile with rounded corners,
a pink to blue gradient from left to right, strokes 21 px wide at 256 px with round nodes at their
ends, and a faint glow of the same colours. Designer's icon is one trace with a spike between two
nodes. This one is three channels that come in on the left, the middle one with Designer's spike,
and merge into one node on the right: many electrodes in, one stream out.

Run it after changing the drawing, and commit the .ico:

    pip install pillow
    python share/tools/make_acquisition_server_icon.py

Everything is drawn at SUPERSAMPLE times 256 px and scaled down, which is what smooths the edges.
"""

from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

# ---- Measured from share/ov-icon.ico, at 256 px ---------------------------------------------------
TILE_RADIUS = 64                  # corner radius of the white tile
STROKE = 21                       # width of the traces
NODE_RADIUS = 18                  # end nodes of the output, as Designer's
INPUT_NODE_RADIUS = 14            # input nodes, smaller: there are three of them
PINK = (238, 82, 120)             # gradient at GRADIENT_LEFT
BLUE = (79, 120, 218)             # gradient at GRADIENT_RIGHT
GRADIENT_LEFT, GRADIENT_RIGHT = 33, 222
GLOW_OPACITY = 0.10               # Designer's glow is barely there: keep it so

# ---- The drawing, at 256 px ------------------------------------------------------------------------
INPUT_X = 50                      # centres of the three input nodes
INPUT_YS = (76, 128, 180)
MERGE = (166, 128)                # where the three channels meet
OUTPUT = (210, 128)               # centre of the output node
# The middle channel's spike, as in Designer's icon: flat, down, up high, back to the line
SPIKE = [(INPUT_X, 128), (70, 128), (82, 144), (98, 106), (112, 128), MERGE]

SIZES = (16, 24, 32, 48, 64, 128, 256)
SUPERSAMPLE = 4
OUTPUT_FILE = Path(__file__).resolve().parent.parent / "ov-acquisition-server-icon.ico"


def scaled(points, factor):
    return [(x * factor, y * factor) for x, y in points]


def bezier(p0, p1, p2, p3, steps=48):
    """Points along a cubic Bezier curve, for the outer channels bending into the merge point."""
    points = []
    for i in range(steps + 1):
        t = i / steps
        u = 1 - t
        points.append((u**3 * p0[0] + 3 * u**2 * t * p1[0] + 3 * u * t**2 * p2[0] + t**3 * p3[0],
                       u**3 * p0[1] + 3 * u**2 * t * p1[1] + 3 * u * t**2 * p2[1] + t**3 * p3[1]))
    return points


def outer_channel(y):
    """A channel that leaves its node level, then bends smoothly into the merge point."""
    straight_end = (108, y)
    return [(INPUT_X, y)] + bezier(straight_end, (138, y), (140, MERGE[1]), MERGE)


def stroke_mask(size, factor):
    """White where the traces and nodes are, black elsewhere."""
    mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(mask)
    width = round(STROKE * factor)
    paths = [outer_channel(INPUT_YS[0]), SPIKE, outer_channel(INPUT_YS[2]), [MERGE, OUTPUT]]
    for path in paths:
        draw.line(scaled(path, factor), fill=255, width=width, joint="curve")
        # Round caps and joints: Pillow's lines have square ends
        for x, y in scaled(path, factor):
            r = width / 2
            draw.ellipse((x - r, y - r, x + r, y + r), fill=255)
    for y in INPUT_YS:
        r = INPUT_NODE_RADIUS * factor
        draw.ellipse((INPUT_X * factor - r, y * factor - r, INPUT_X * factor + r, y * factor + r), fill=255)
    r = NODE_RADIUS * factor
    draw.ellipse((OUTPUT[0] * factor - r, OUTPUT[1] * factor - r, OUTPUT[0] * factor + r, OUTPUT[1] * factor + r), fill=255)
    return mask


def gradient(size, factor):
    """The pink to blue gradient, constant beyond its two ends."""
    row = Image.new("RGB", (size, 1))
    for x in range(size):
        t = min(max((x / factor - GRADIENT_LEFT) / (GRADIENT_RIGHT - GRADIENT_LEFT), 0.0), 1.0)
        row.putpixel((x, 0), tuple(round(a + (b - a) * t) for a, b in zip(PINK, BLUE)))
    return row.resize((size, size))


def draw_icon():
    factor = SUPERSAMPLE
    size = 256 * factor

    tile_mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(tile_mask).rounded_rectangle((0, 0, size - 1, size - 1), radius=TILE_RADIUS * factor, fill=255)

    colours = gradient(size, factor)
    strokes = stroke_mask(size, factor)

    icon = Image.new("RGBA", (size, size), (255, 255, 255, 255))
    glow_alpha = strokes.filter(ImageFilter.GaussianBlur(28 * factor)).point(lambda a: round(a * GLOW_OPACITY))
    icon.paste(colours, (0, 0), glow_alpha)
    icon.paste(colours, (0, 0), strokes)
    icon.putalpha(ImageChops.multiply(tile_mask, Image.new("L", (size, size), 255)))
    return icon.resize((256, 256), Image.LANCZOS)


def main():
    icon = draw_icon()
    icon.save(OUTPUT_FILE, format="ICO", sizes=[(s, s) for s in SIZES])
    print(f"Wrote {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
