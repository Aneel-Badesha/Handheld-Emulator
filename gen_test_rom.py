#!/usr/bin/env python3
"""
Generate a minimal NROM (mapper 0) NES test ROM.
Displays a solid red screen — clearly distinct from the grey 2048.nes background.
Writes the result to components/nofrendo/builtin.nes.
"""

import os

prg = bytearray(0x4000)   # 16 KB PRG ROM
chr_rom = bytearray(0x2000)  # 8 KB CHR ROM

# --- CHR ROM ---
# Tile 0 (offset   0): all zeros = blank
# Tile 1 (offset  16): both planes all 1s → pixel value 3 (uses palette color 3)
for i in range(8):
    chr_rom[16 + i] = 0xFF  # tile 1 plane 0
    chr_rom[24 + i] = 0xFF  # tile 1 plane 1

# --- PRG ROM ---
# NMI handler at $C000 (PRG offset 0x0000): just RTI
prg[0x0000] = 0x40  # RTI

# Reset handler at $C100 (PRG offset 0x0100)
pos = 0x0100

def w(*b):
    global pos
    for x in b:
        prg[pos] = x & 0xFF
        pos += 1

# ---- startup ----
w(0x78)              # SEI
w(0xD8)              # CLD
w(0xA2, 0xFF)        # LDX #$FF
w(0x9A)              # TXS
w(0xA9, 0x00)        # LDA #0

# clear zero page
w(0xA2, 0x00)        # LDX #0
# loop: STA $00,X / INX / BNE (-5)
w(0x95, 0x00)        # STA $00,X   ← loop
w(0xE8)              # INX
w(0xD0, 0xFB)        # BNE -5

# wait for PPU warmup — 2 vblanks
w(0x2C, 0x02, 0x20)  # BIT $2002 (clear stale VBL flag)
w(0x2C, 0x02, 0x20)  # BIT $2002  ← vbl1 loop
w(0x10, 0xFB)        # BPL -5
w(0x2C, 0x02, 0x20)  # BIT $2002  ← vbl2 loop
w(0x10, 0xFB)        # BPL -5

# ---- write palette to $3F00 ----
w(0xA9, 0x3F)        # LDA #$3F
w(0x8D, 0x06, 0x20)  # STA $2006
w(0xA9, 0x00)        # LDA #$00
w(0x8D, 0x06, 0x20)  # STA $2006

# 32 palette bytes (4 BG + 4 sprite palettes, 4 colors each)
# BG palette 0: black / pink / light-pink / RED  ← tile 1 uses color 3 = red
palette = [
    0x0F, 0x15, 0x25, 0x16,  # BG pal 0: black, pink, light-pink, red
    0x0F, 0x11, 0x21, 0x31,  # BG pal 1
    0x0F, 0x1A, 0x2A, 0x3A,  # BG pal 2
    0x0F, 0x08, 0x18, 0x28,  # BG pal 3
    0x0F, 0x15, 0x25, 0x16,  # SPR pal 0
    0x0F, 0x11, 0x21, 0x31,  # SPR pal 1
    0x0F, 0x1A, 0x2A, 0x3A,  # SPR pal 2
    0x0F, 0x08, 0x18, 0x28,  # SPR pal 3
]
for c in palette:
    w(0xA9, c)
    w(0x8D, 0x07, 0x20)  # STA $2007

# ---- fill nametable at $2000 ----
w(0xA9, 0x20)        # LDA #$20
w(0x8D, 0x06, 0x20)  # STA $2006
w(0xA9, 0x00)        # LDA #$00
w(0x8D, 0x06, 0x20)  # STA $2006

# write 960 tiles (30 rows × 32 cols) of tile index 1
# 960 = 3×256 + 192
w(0xA9, 0x01)        # LDA #1 (tile index)
for _ in range(3):
    w(0xA2, 0x00)        # LDX #0
    w(0x8D, 0x07, 0x20)  # STA $2007  ← loop (-6)
    w(0xE8)              # INX
    w(0xD0, 0xFA)        # BNE -6

w(0xA2, 0x00)        # LDX #0
w(0x8D, 0x07, 0x20)  # STA $2007  ← loop
w(0xE8)              # INX
w(0xE0, 0xC0)        # CPX #192
w(0xD0, 0xF8)        # BNE -8

# write 64 attribute bytes (all 0 = use BG palette 0 everywhere)
w(0xA9, 0x00)        # LDA #0
w(0xA2, 0x00)        # LDX #0
w(0x8D, 0x07, 0x20)  # STA $2007  ← loop
w(0xE8)              # INX
w(0xE0, 0x40)        # CPX #64
w(0xD0, 0xF8)        # BNE -8

# ---- reset scroll to 0,0 ----
w(0xA9, 0x00)
w(0x8D, 0x05, 0x20)  # STA $2005 (X scroll = 0)
w(0x8D, 0x05, 0x20)  # STA $2005 (Y scroll = 0)

# ---- enable rendering ----
w(0xA9, 0x1E)        # LDA #$1E (show BG + sprites, no clipping)
w(0x8D, 0x01, 0x20)  # STA $2001 (PPUMASK)
w(0xA9, 0x80)        # LDA #$80 (NMI enable, BG pattern at $0000)
w(0x8D, 0x00, 0x20)  # STA $2000 (PPUCTRL)

# ---- infinite loop ----
forever = 0xC000 + pos
w(0x4C, forever & 0xFF, forever >> 8)

# ---- interrupt vectors at $FFFA–$FFFF ----
prg[0x3FFA] = 0x00; prg[0x3FFB] = 0xC0  # NMI   → $C000
prg[0x3FFC] = 0x00; prg[0x3FFD] = 0xC1  # RESET → $C100
prg[0x3FFE] = 0x00; prg[0x3FFF] = 0xC0  # IRQ   → $C000

# --- iNES header ---
header = bytes([
    0x4E, 0x45, 0x53, 0x1A,  # "NES\x1A"
    0x01,                     # 1 × 16 KB PRG
    0x01,                     # 1 × 8 KB CHR
    0x00,                     # mapper 0, horizontal mirror
    0x00,                     # mapper 0
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
])

out = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                   'components', 'nofrendo', 'src', 'games', 'builtin.nes')
with open(out, 'wb') as f:
    f.write(header + prg + chr_rom)

total = len(header) + len(prg) + len(chr_rom)
print(f"written {total} bytes → {out}")
print(f"PRG code: {pos - 0x100} bytes")
