"""Small dependency-free check of the shader's hue-region geometry."""
from colorsys import rgb_to_hls
from pathlib import Path
import re

shader = (Path(__file__).parents[1] / "data/effects/hsl.effect").read_text()
regions = {
    name: (float(center), float(radius))
    for name, center, radius in re.findall(
        r"float (\w+)_weight = hue_mask\(hsl\.x, ([\d.]+), ([\d.]+)\);", shader
    )
}
assert len(regions) == 8


def weight(hue, center, radius):
    distance = abs(hue - center) % 360
    distance = min(distance, 360 - distance) / radius
    t = min(1, max(0, distance))
    return 1 - t * t * (3 - 2 * t)


def weights(hue):
    raw = {name: weight(hue, *geometry) for name, geometry in regions.items()}
    total = sum(raw.values())
    return {name: value / total for name, value in raw.items()}


assert abs(weights(359)["red"] - weights(1)["red"]) < 0.1
assert weights(25)["orange"] > weights(25)["red"] > 0
assert weights(25)["blue"] == 0
assert weights(235)["blue"] > 0.5
assert abs(weights(14.9)["red"] - weights(15.1)["red"]) < 0.01
custom_radius = float(re.search(r"hue_mask\(selection_hue, selected\.x \* 360\.0, ([\d.]+)\)", shader)[1])
orange_hue = rgb_to_hls(1.0, 0.5, 0.0)[0] * 360
assert weight(25, orange_hue, custom_radius) > 0.9
assert weight(235, orange_hue, custom_radius) == 0
print("Hue masks: wrap, overlap, locality, and continuity OK")
