#!/usr/bin/env python3
"""Nepenthe skin: draws vst/art/*.png and writes vst/layout.conf (python3 vst/skin/make_skin.py, needs Pillow).

Upstream's editor (upstream/reference/Editor.cpp, LookAndFeel.cpp) at MPC size: six vertical sliders on #060710,
names above in Karla (upstream's font, OFL, vst/skin/fonts/), values below. Each slider is upstream's drawLinearSlider:
an 8 px round-capped track, #181d40 fading out towards the thumb above it, the thumb's colour at 40 % fading out
towards the bottom below it, a 14 px thumb with 3 px corners; Time/Delay #c13791, Bass/Treble #6b42ba,
Width/Mix #2953c9. Sizes here are upstream's x SCALE.
MPC limits (Open MPC docs/NOTES.md): the plugin area is 1280 x 628 (layout y = skin y + 86); slider filmstrips are
frames of the slider's own size, at most 12288 px of strip, so the thumb moves in (12288 // h) steps; live text is
Titillium Web. The title "Nepenthe Reverb" under the values is the port's own (Playball, OFL, the font of upstream's web page; the sliders' colours).
Edit this script, not layout.conf.
"""
import os

from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
VST = os.path.dirname(HERE)
ART = os.path.join(VST, "art")
KARLA = os.path.join(HERE, "fonts", "Karla.ttf")
PLAYBALL = os.path.join(HERE, "fonts", "Playball-Regular.ttf")   # the title, as upstream's site (OFL, fonts/Playball-OFL.txt)

W, H, PAGE_Y = 1280, 628, 86
SCALE = 2.1
SS = 4                      # supersampling for the slider frames
MAX_STRIP = 12288

BG = (0x06, 0x07, 0x10)
TRACK = (0x18, 0x1d, 0x40)
TIME_C, EQ_C, MIX_C = (0xc1, 0x37, 0x91), (0x6b, 0x42, 0xba), (0x29, 0x53, 0xc9)
SLIDERS = [("time", "Time", TIME_C), ("delay", "Delay", TIME_C), ("bass", "Bass", EQ_C),
           ("treble", "Treble", EQ_C), ("width", "Width", MIX_C), ("mix", "Mix", MIX_C)]

COL_W = 190
LEFT = (W - COL_W * len(SLIDERS)) // 2
LABEL_Y = 52                # skin y of the names' centre
SL_TOP, SL_H = 96, 416      # the slider box (skin y); its thumb travels between the box's top and bottom + INDENT
SL_W = 64
INDENT = 20                 # JUCE's thumb indent (12 px upstream) at this size
THUMB = round(14 * SCALE)
CORNER = 3 * SCALE
STROKE = 8 * SCALE
LABEL_PX = round(16 * SCALE)
VALUE_PX = 34
TITLE = "Nepenthe Reverb"   # the port's title, under the values (user, 2026-11-22)
TITLE_Y, TITLE_PX = 588, 50


def lerp(a, b, t):
    return a + (b - a) * t


def capsule_mask(w, h, x, y0, y1, width):
    """A round-capped vertical stroke from y0 to y1 (JUCE PathStrokeType rounded), as an L mask."""
    m = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(m)
    r = width / 2
    if y1 > y0:
        d.rectangle((x - r, y0, x + r, y1), fill=255)
    d.ellipse((x - r, y0 - r, x + r, y0 + r), fill=255)
    d.ellipse((x - r, y1 - r, x + r, y1 + r), fill=255)
    return m


def gradient(w, h, colour, a0, y0, a1, y1):
    """JUCE ColourGradient (linear, vertical, colours clamped beyond the ends): alpha a0 at y0 to a1 at y1."""
    g = Image.new("RGBA", (1, h))
    px = g.load()
    for y in range(h):
        t = 0.0 if y1 == y0 else min(1.0, max(0.0, (y + 0.5 - y0) / (y1 - y0)))
        px[0, y] = colour + (round(255 * lerp(a0, a1, t)),)
    return g.resize((w, h), Image.NEAREST)


def frame(colour, pos):
    """One slider frame, pos 0 (minimum, thumb at the bottom) .. 1."""
    w, h = SL_W * SS, SL_H * SS
    im = Image.new("RGBA", (w, h), BG + (255,))
    x = w / 2
    start, end = INDENT * SS, (SL_H - INDENT) * SS
    mid = lerp(end, start, pos)
    half = THUMB * SS / 2
    stroke = STROKE * SS
    # upper: track colour at the top, transparent at the thumb's top edge
    up = gradient(w, h, TRACK, 1.0, start, 0.0, mid - half)
    im.alpha_composite(Image.composite(up, Image.new("RGBA", (w, h), (0, 0, 0, 0)), capsule_mask(w, h, x, start, mid, stroke)))
    # lower: thumb colour at 40 % from the thumb's bottom edge, transparent at the end
    lo = gradient(w, h, colour, 0.4, mid + half, 0.0, end)
    im.alpha_composite(Image.composite(lo, Image.new("RGBA", (w, h), (0, 0, 0, 0)), capsule_mask(w, h, x, mid, end, stroke)))
    ImageDraw.Draw(im).rounded_rectangle((x - half, mid - half, x + half, mid + half), CORNER * SS, fill=colour + (255,))
    return im.resize((SL_W, SL_H), Image.LANCZOS)


def title(bg, s):
    """TITLE centred at the bottom in one run (the script font's own kerning and joins), filled with a
    pink -> violet -> blue gradient (the sliders' colours)."""
    f = ImageFont.truetype(PLAYBALL, TITLE_PX * s)
    mask = Image.new("L", bg.size, 0)
    md = ImageDraw.Draw(mask)
    md.text((W * s / 2, TITLE_Y * s), TITLE, font=f, fill=255, anchor="mm")
    x0, _, x1, _ = md.textbbox((W * s / 2, TITLE_Y * s), TITLE, font=f, anchor="mm")
    total = x1 - x0
    grad = Image.new("RGB", (bg.size[0], 1))
    px = grad.load()
    stops = [TIME_C, EQ_C, MIX_C]
    for i in range(bg.size[0]):
        t = min(1.0, max(0.0, (i - x0) / max(1.0, total))) * (len(stops) - 1)
        a, b = stops[min(int(t), len(stops) - 2)], stops[min(int(t), len(stops) - 2) + 1]
        u = t - min(int(t), len(stops) - 2)
        px[i, 0] = tuple(round(lerp(a[k], b[k], u)) for k in range(3))
    bg.paste(grad.resize(bg.size, Image.NEAREST), (0, 0), mask)


def main():
    os.makedirs(ART, exist_ok=True)
    bg = Image.new("RGB", (W * 2, H * 2), BG)
    d = ImageDraw.Draw(bg)
    f = ImageFont.truetype(KARLA, LABEL_PX * 2)
    n = MAX_STRIP // SL_H
    lines = []
    for i, (key, name, col) in enumerate(SLIDERS):
        cx = LEFT + COL_W * i + COL_W // 2
        d.text((cx * 2, LABEL_Y * 2), name, font=f, fill=(255, 255, 255), anchor="mm")
        strip = Image.new("RGBA", (SL_W, SL_H * n))
        for k in range(n):
            strip.paste(frame(col, k / (n - 1)), (0, SL_H * k))
        strip.convert("RGB").save(os.path.join(ART, "slider_%s.png" % key))
        lines.append('slider_v cx=%d cy=%d w=%d h=%d label="" key=%s strip=art/slider_%s.png frames=%d ns=0 vs=%d bw=%d'
                     % (cx, SL_TOP + SL_H // 2 + PAGE_Y, SL_W, SL_H, key, key, n, VALUE_PX, COL_W - 20))
    title(bg, 2)
    bg.resize((W, H), Image.LANCZOS).save(os.path.join(ART, "bg.png"))

    head = """# Nepenthe (MPC port). GENERATED by vst/skin/make_skin.py: edit the script, not this file.
theme_bg=060710
theme_panel=0b0d1c
theme_line=181d40
theme_ink=ffffff
theme_ink_dim=ffffff
theme_ink_faint=6b6f8f
theme_accent=2953c9
theme_accent_hi=6b8ee8
theme_knob_face=181d40
theme_knob_ring=2953c9
theme_knob_dot=ffffff
theme_bar=060710
theme_btn_text=ffffff
theme_tab_on=2953c9
theme_tab_on_tx=ffffff
theme_lcd=060710

[tab NEPENTHE]
art file=art/bg.png fit=stretch
"""
    with open(os.path.join(VST, "layout.conf"), "w", newline="\n") as fh:
        fh.write(head + "\n".join(lines) + '\nqlinks "NEPENTHE" = time,delay,bass,treble,width,mix\n')
    print("wrote art/bg.png, %d-frame slider strips, layout.conf" % n)


if __name__ == "__main__":
    main()
