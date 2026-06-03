# Casio FZ-1 OLED Display Replacement

A low cost DIY replacement display for the Casio FZ-1 sampler/synthesizer using a Raspberry Pi Pico and a generic SSD1309 OLED module.

This project replaces the original ageing FZ-1 LCD with inexpensive, readily available parts. The goal is to make the repair practical and affordable for ordinary owners: no custom PCB, no rare display module, no specialist programmable logic, and no expensive donor parts. The working prototype uses a Pi Pico, a 128x64 SPI SSD1309 OLED, simple resistor dividers for 5V-to-3.3V logic conversion, and a 3D printed mounting plate.

> Project status: working prototype. The firmware renders the FZ-1 menu and boot screens correctly on real hardware. More long-term testing is still in progress. Please report any observed issues or unwanted behaviour.

## Why This Exists

Many Casio FZ-1 units still sound wonderful, but the original display hardware is now a weak point. Replacement displays are difficult to source, and many modern display modules do not speak the same protocol as the original LCD.

This project keeps the synthesizer mostly original while replacing only the display system. The FZ-1 still drives its original display bus; the Pico listens to that bus, decodes the commands, and renders the same content onto a modern OLED.

## Why A Raspberry Pi Pico?

The RP2040 is especially well suited to this job because of its PIO peripheral.

The FZ-1 display bus is a timing-sensitive 4-bit parallel interface with separate clock, chip-enable, and command/data signals. A normal microcontroller interrupt loop can easily miss edges or jitter under load. The Pico's PIO state machine can sample the bus independently of the CPU, while DMA moves captured samples into memory. The main firmware can then decode and render the display data without needing to bit-bang the vintage bus directly.

In short:

- PIO gives reliable edge-triggered bus capture.
- DMA keeps capture overhead low.
- Hardware SPI drives the OLED quickly.
- The Pico is cheap, common, and easy to flash by copying a UF2 file.

## Repository Contents

- `main.cpp` - Pico firmware, OLED driver, FZ-1 protocol parser, and renderer.
- `fz1_capture.pio` - PIO program for sampling the FZ-1 display bus.
- `font.h` - 256-character FZ-compatible 6x8 font table.
- `FZ1wiring.md` - Detailed wiring reference.
- `images/` - Photos of the original display board, cut connector board, assembled electronics, and installed working display.
- `3D printed mount/` - STL files for printing the display mounting plate and brackets.

## Parts

Core parts:

- Casio FZ-1 with original display connector/board available.
- Raspberry Pi Pico or compatible RP2040 Pico board.
- 128x64 SPI OLED module using SSD1309 controller.
- 1kΩ and 2kΩ resistors for voltage dividers on the FZ-1 bus signals.
- Hookup wire, soldering tools, and basic mounting hardware.
- 3D printed OLED mount and brackets from `3D printed mount/`.

The prototype uses resistor dividers rather than active level-shifter ICs. Bidirectional auto-sensing level shifters were originally tested and proved incompatible with the FZ-1 display bus.

## Hardware Overview

The Pico passively listens to the FZ-1 display bus. It does not replace or emulate the FZ-1 CPU; it only translates the original display traffic into SSD1309 OLED drawing commands.

The FZ-1 display signals are 5V logic, while the Pico GPIOs are 3.3V-only. Each FZ-1 bus signal connected to the Pico must pass through a resistor divider:

- 1kΩ from FZ-1 signal to Pico GPIO.
- 2kΩ from Pico GPIO to ground.

This project currently captures seven FZ-1 signals:

| FZ-1 signal | Pico GPIO |
| --- | --- |
| D8 | GP0 |
| D4 | GP1 |
| D2 | GP2 |
| D1 | GP3 |
| PH2 clock | GP4 |
| #CE | GP5 |
| #OP | GP6 |

The OLED is connected directly to the Pico at 3.3V:

| OLED signal | Pico GPIO |
| --- | --- |
| CS | GP17 |
| SCK | GP18 |
| MOSI | GP19 |
| DC/A0 | GP20 |
| RES/RST | GP21 |

Power:

- FZ-1 +5V can power Pico `VSYS`.
- Pico `3V3_OUT` powers the OLED.
- All grounds must be common.

Important: disconnect the Pico from USB when powering it from the FZ-1 `VSYS` connection, unless your hardware setup specifically prevents back-powering.

See `FZ1wiring.md` for the full wiring table.

## Physical Installation

The prototype reuses part of the original display board as a connector breakout. The original display PCB is cut so that the FZ-1 ribbon connector section can be retained, and wires are soldered to that retained connector section.

If you do not want to cut the original display board, or if your FZ-1 no longer has that board available, the [Deftaudio/Casio-FZ-1 repository](https://github.com/Deftaudio/Casio-FZ-1) includes an alternative [IDC adapter PCB design](https://github.com/Deftaudio/Casio-FZ-1/tree/main/OLED%20upgrade%20by%20Dmitrins/fz1_oled_IDC_adapter). That adapter provides a way to build a new connector interface rather than harvesting the original display board connector.

The OLED is held in a custom 3D printed mount designed to match the shape of the display module and closely follow the original FZ-1 mounting plate. Additional brackets secure the assembly inside the synthesizer.

Photos and installation references should be placed in `images/`. 3D printable files should be placed in `3D printed mount/`.

## Firmware Overview

The firmware has four main jobs:

1. Initialize the SSD1309 OLED over SPI.
2. Use PIO to sample the FZ-1 display bus on PH2 clock edges while #CE is active.
3. Decode the FZ-1 command/data packets.
4. Render text and graphics to the OLED using the FZ-1 font.

Implementation details:

- FZ-1 data lines are active-low.
- The physical data wiring is reversed, so decoded nibbles are bit-reversed with a lookup table.
- Hardware testing showed that each nibble is captured twice in adjacent pairs. The parser conservatively collapses packets only when every adjacent pair matches.
- The FZ-1 cursor arrow uses extended font glyph `0xEE`, so the renderer uses the full 256-entry font table.

## Building The Firmware

Install the Raspberry Pi Pico SDK and an ARM embedded GCC toolchain. On macOS, the tested working setup for this project uses the `gcc-arm-embedded` Homebrew cask.

Configure and build:

```bash
cmake -S . -B build -DPICO_SDK_PATH=/path/to/pico-sdk
cmake --build build
```

The build produces:

```text
build/FZ1_display.uf2
```

## Flashing The Pico

The easiest route is to download the prebuilt `FZ1_display.uf2` file from the repository's GitHub Releases page. If you are modifying the firmware, build it locally and use the UF2 produced at `build/FZ1_display.uf2`.

1. Hold the Pico `BOOTSEL` button while connecting it over USB.
2. The Pico appears as a USB mass-storage device.
3. Copy `FZ1_display.uf2` to the Pico.
4. The Pico reboots automatically into the firmware.

Once installed in the FZ-1, the firmware is silent: there is no boot test pattern and no serial output.

Maintainers can create a GitHub Release by pushing a version tag such as `v0.1.0`. The release workflow builds the firmware and attaches `FZ1_display.uf2` to the release automatically.

## Testing

Basic checks:

- OLED should clear at startup.
- FZ-1 boot screen should render correctly.
- Main menu text should be aligned and readable.
- Cursor arrow should appear and move with the FZ-1 cursor buttons.
- Switching pages and menus should not leave stale pixels or corrupt text.

If the display is blank, verify power, common ground, OLED SPI wiring, and reset/DC pins.

If the display is garbled, verify the FZ-1 bus wiring order, resistor dividers, and the GP0-GP6 contiguous capture wiring.

## Safety Notes

This modification requires opening the synthesizer and working near vintage electronics. Work carefully, disconnect power before soldering, and double-check the 5V-to-3.3V dividers before connecting the Pico.

The Pico GPIOs are not 5V tolerant.

## Credits

This project was developed with reference to the OLED upgrade material preserved in the [Deftaudio/Casio-FZ-1 repository](https://github.com/Deftaudio/Casio-FZ-1). Those files include a copy of the original AVR-based FZ/VZ OLED design by Dmitri Sytov, known as Dmitrins, who shared it on the [Yamahamusicians forum](https://yamahamusicians.com/forum/viewtopic.php?t=10666&start=240). The RP2040 firmware in this repository ports the concept to a Raspberry Pi Pico using PIO, DMA, and SPI.

## License

This project is released under the MIT License. See `LICENSE` for the full license text.

Third-party reference material is credited above and remains the work of its original authors.
