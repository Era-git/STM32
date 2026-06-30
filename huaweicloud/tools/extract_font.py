import re, urllib.request
u = "https://raw.githubusercontent.com/adafruit/Adafruit-GFX-Library/master/glcdfont.c"
raw = urllib.request.urlopen(u, timeout=30).read().decode("utf-8", errors="ignore")
nums = [int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]+", raw)]
# Adafruit font[]: 256 glyphs * 5 bytes
start, ln = 32 * 5, 96 * 5
chunk = nums[start : start + ln]
lines = []
for i in range(0, len(chunk), 12):
    lines.append("  " + ", ".join("0x%02X" % b for b in chunk[i : i + 12]) + ",")
open(r"c:\Users\51706\Documents\huaweicloud\Core\Src\font5x7_ascii.c", "w", encoding="utf-8").write(
    "/* Adafruit_GFX glcdfont ASCII 32-127, 5 bytes/glyph */\n"
    "#include <stdint.h>\nconst uint8_t font5x7_ascii[96 * 5] = {\n"
    + "\n".join(lines)
    + "\n};\n"
)
print("wrote", len(chunk), "bytes")
