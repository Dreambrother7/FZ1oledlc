# FZ-1 OLED Replacement - Current Project State

## Status

The RP2040 Pico firmware is now producing correct output on the SSD1309 OLED from the Casio FZ-1 display bus.

Working firmware output:

- No boot checkerboard/test pattern.
- No serial diagnostics or heartbeat output.
- OLED initializes, clears, then renders FZ-1 display traffic.
- Current UF2: `build/FZ1_display.uf2`.

## Current Build

```bash
cmake --build build
```

The project currently builds on this machine using the `gcc-arm-embedded` Homebrew cask toolchain.

## Hardware Mapping

FZ-1 display connector to Pico inputs:

| FZ-1 Pin | Signal | Pico GPIO |
| --- | --- | --- |
| 1 | D8 | GP0 |
| 2 | D4 | GP1 |
| 3 | D2 | GP2 |
| 4 | D1 | GP3 |
| 9 | PH2 clock | GP4 |
| 11 | #CE | GP5 |
| 12 | #OP | GP6 |

Pico to SSD1309 OLED:

| Pico GPIO | OLED Signal |
| --- | --- |
| GP17 | CS |
| GP18 | SCK |
| GP19 | MOSI |
| GP20 | DC/A0 |
| GP21 | RES/RST |

## Important Implementation Notes

- FZ-1 data lines are active-low.
- Data line order is reversed on the Pico: D8 is on GP0 and D1 is on GP3.
- PIO samples GP0-GP6 on PH2 rising edge while #CE is low.
- Captured nibbles are decoded with active-low inversion plus the `nibbleflip` lookup.
- Hardware testing showed every nibble is captured twice in adjacent pairs. `flush_packet()` therefore collapses packets only when all adjacent pairs match, turning `A A B B C C...` into `A B C...`.
- Command/data packets are split by #OP state changes, with a timeout fallback for idle gaps.
- The FZ-1 font uses extended character codes. The menu cursor arrow is glyph `0xEE`, so rendering must use the full 256-entry `font` table rather than clamping characters above `0x7F` to spaces.

## Known Good Result

The duplicate-nibble collapse fixed the garbled display. Hardware testing showed correct menu and boot screens after the cleanup build.

## Main Files

- `main.cpp` - Firmware, OLED driver, capture parser, FZ-1 renderer.
- `fz1_capture.pio` - PIO capture program.
- `font.h` - FZ-1-compatible 6x8 font table.
- `FZ1wiring.md` - Wiring reference.
- `deftaudio_ref/fz1-oled/` - AVR reference implementation.
