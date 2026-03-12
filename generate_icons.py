#!/usr/bin/env python3
"""Generate 1-bit PNG icon assets for Philco AC Flipper Zero app."""

from PIL import Image, ImageDraw
import os
import math

OUT_DIR = os.path.join(os.path.dirname(__file__), "philco_ac", "images")
APP_DIR = os.path.join(os.path.dirname(__file__), "philco_ac")
os.makedirs(OUT_DIR, exist_ok=True)


def save(img, name, directory=None):
    path = os.path.join(directory or OUT_DIR, name)
    img.save(path)
    print(f"  Created {path}")


# ── Mode Icons (12x12) ──────────────────────────────────────────────

def mode_cool():
    """Snowflake: 6 lines from center + small ticks."""
    img = Image.new("1", (12, 12), 0)
    d = ImageDraw.Draw(img)
    cx, cy = 5, 5
    # 6 lines radiating from center
    for angle_deg in range(0, 360, 60):
        rad = math.radians(angle_deg)
        ex = cx + round(4 * math.cos(rad))
        ey = cy + round(4 * math.sin(rad))
        d.line([(cx, cy), (ex, ey)], fill=1)
        # Small tick marks near the ends
        tick_len = 1.5
        for tick_angle in [angle_deg + 45, angle_deg - 45]:
            tr = math.radians(tick_angle)
            mx = cx + round(3 * math.cos(rad))
            my = cy + round(3 * math.sin(rad))
            tx = mx + round(tick_len * math.cos(tr))
            ty = my + round(tick_len * math.sin(tr))
            d.line([(mx, my), (tx, ty)], fill=1)
    save(img, "mode_cool_12x12.png")


def mode_heat():
    """Sun: circle with 8 rays."""
    img = Image.new("1", (12, 12), 0)
    d = ImageDraw.Draw(img)
    cx, cy = 5, 5
    # Center circle
    d.ellipse([3, 3, 8, 8], outline=1, fill=1)
    # 8 rays
    for angle_deg in range(0, 360, 45):
        rad = math.radians(angle_deg)
        sx = cx + round(4 * math.cos(rad))
        sy = cy + round(4 * math.sin(rad))
        ex = cx + round(5.5 * math.cos(rad))
        ey = cy + round(5.5 * math.sin(rad))
        d.line([(sx, sy), (ex, ey)], fill=1)
    save(img, "mode_heat_12x12.png")


def mode_dry():
    """Water droplet shape."""
    img = Image.new("1", (12, 12), 0)
    d = ImageDraw.Draw(img)
    # Teardrop: tip at top, round at bottom
    # Using polygon for the drop shape
    points = [
        (6, 1),   # tip
        (3, 5),
        (2, 7),
        (2, 9),
        (4, 11),
        (8, 11),
        (10, 9),
        (10, 7),
        (9, 5),
        (6, 1),
    ]
    d.polygon(points, outline=1, fill=0)
    # Small highlight
    d.point((4, 7), fill=1)
    save(img, "mode_dry_12x12.png")


def mode_fan():
    """3-blade fan."""
    img = Image.new("1", (12, 12), 0)
    d = ImageDraw.Draw(img)
    cx, cy = 5, 5
    # Center dot
    d.rectangle([5, 5, 6, 6], fill=1)
    # 3 blades at 120° apart, each is a small arc/ellipse
    for angle_deg in [90, 210, 330]:
        rad = math.radians(angle_deg)
        # Blade center point
        bx = cx + round(3 * math.cos(rad))
        by = cy - round(3 * math.sin(rad))
        # Draw blade as small filled ellipse
        d.ellipse([bx - 2, by - 1, bx + 2, by + 1], fill=1, outline=1)
    save(img, "mode_fan_12x12.png")


def mode_off():
    """Power symbol: circle with line at top."""
    img = Image.new("1", (12, 12), 0)
    d = ImageDraw.Draw(img)
    # Circle (not fully closed at top)
    d.arc([2, 3, 10, 11], start=50, end=310, fill=1)
    # Vertical line through top
    d.line([(6, 1), (6, 6)], fill=1)
    save(img, "mode_off_12x12.png")


# ── Fan Speed Icons (20x8) ──────────────────────────────────────────

def fan_bars(name, filled_count, show_auto=False):
    """Graduated bars: 3 bars of increasing height, filled_count filled."""
    img = Image.new("1", (20, 8), 0)
    d = ImageDraw.Draw(img)
    bar_w = 4
    gap = 2
    heights = [4, 6, 8]  # short, medium, tall

    for i in range(3):
        x = i * (bar_w + gap)
        h = heights[i]
        y = 8 - h
        if i < filled_count:
            d.rectangle([x, y, x + bar_w - 1, 7], fill=1)
        else:
            d.rectangle([x, y, x + bar_w - 1, 7], outline=1)

    if show_auto:
        # Draw "A" at the right side
        # Simple 5px tall A
        ax = 15
        d.line([(ax + 1, 2), (ax, 7)], fill=1)  # left leg
        d.line([(ax + 1, 2), (ax + 3, 7)], fill=1)  # right leg
        d.line([(ax, 5), (ax + 3, 5)], fill=1)  # crossbar

    save(img, name)


# ── Swing Icons (8x8) ───────────────────────────────────────────────

def swing_on():
    """Oscillation arrows (up-down)."""
    img = Image.new("1", (8, 8), 0)
    d = ImageDraw.Draw(img)
    # Up arrow
    d.line([(3, 0), (3, 3)], fill=1)
    d.line([(1, 2), (3, 0), (5, 2)], fill=1)
    # Down arrow
    d.line([(3, 4), (3, 7)], fill=1)
    d.line([(1, 5), (3, 7), (5, 5)], fill=1)
    save(img, "swing_on_8x8.png")


def swing_off():
    """Horizontal line (static)."""
    img = Image.new("1", (8, 8), 0)
    d = ImageDraw.Draw(img)
    d.line([(1, 4), (6, 4)], fill=1)
    # Small end caps
    d.line([(1, 3), (1, 5)], fill=1)
    d.line([(6, 3), (6, 5)], fill=1)
    save(img, "swing_off_8x8.png")


# ── Send IR Icon (10x10) ────────────────────────────────────────────

def send_ir():
    """IR beam emanating from a point."""
    img = Image.new("1", (10, 10), 0)
    d = ImageDraw.Draw(img)
    # IR emitter (small dot on left)
    d.rectangle([0, 4, 2, 6], fill=1)
    # Radiating arcs
    d.arc([2, 2, 6, 8], start=-60, end=60, fill=1)
    d.arc([4, 1, 8, 9], start=-60, end=60, fill=1)
    d.arc([6, 0, 10, 10], start=-60, end=60, fill=1)
    save(img, "send_ir_10x10.png")


# ── App Icon (10x10) ────────────────────────────────────────────────

def app_icon():
    """Small snowflake for app list."""
    img = Image.new("1", (10, 10), 0)
    d = ImageDraw.Draw(img)
    cx, cy = 4, 4
    for angle_deg in range(0, 360, 60):
        rad = math.radians(angle_deg)
        ex = cx + round(4 * math.cos(rad))
        ey = cy + round(4 * math.sin(rad))
        d.line([(cx, cy), (ex, ey)], fill=1)
    # Center dot
    d.point((cx, cy), fill=1)
    save(img, "philco_ac.png", directory=APP_DIR)


# ── Generate All ────────────────────────────────────────────────────

if __name__ == "__main__":
    print("Generating Philco AC icons...")

    mode_cool()
    mode_heat()
    mode_dry()
    mode_fan()
    mode_off()

    fan_bars("fan_low_20x8.png", 1)
    fan_bars("fan_med_20x8.png", 2)
    fan_bars("fan_high_20x8.png", 3)
    fan_bars("fan_auto_20x8.png", 3, show_auto=True)

    swing_on()
    swing_off()

    send_ir()
    app_icon()

    print("Done!")
