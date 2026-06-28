# Casio FZ-1 OLED Display Replacement

A low cost DIY replacement display for the Casio FZ-1 sampler/synthesizer using a Raspberry Pi Pico and a generic SSD1309 OLED module.

This project replaces the original ageing FZ-1 LCD with inexpensive, readily available parts. The goal is to make the repair practical and affordable for ordinary owners: no rare display module, no specialist programmable logic, and no expensive donor parts. There are now two supported build methods: a hand-wired version using the 3D printed OLED mount, and a Deftaudio PCB version that carries the Pico and OLED on a purpose-made board.

> Project status: working prototype. The firmware renders the FZ-1 menu and boot screens correctly on real hardware. More long-term testing is still in progress. Please report any observed issues or unwanted behaviour.

## Current Revision

Documentation revision: `v0.2.0`

Recent additions since the original working prototype:

- Standalone OLED test firmware for checking the screen before connecting the FZ-1 display bus.
- Five-minute idle screensaver in the main FZ-1 display firmware.
- Deftaudio Pico OLED PCB support, including KiCad files and a ready-to-upload Gerber zip.
- Separate `original` and `deftaudio_pcb` firmware variants for the two hardware methods.
- Experimental Pico 2 / RP2350 builds. These build successfully, but should be treated as untested until confirmed on real hardware.

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

- `main.cpp` - Main FZ-1 display firmware, protocol parser, renderer, and screensaver.
- `oled_ssd1309.cpp` / `oled_ssd1309.h` - Shared SSD1309 OLED driver.
- `oled_test.cpp` - Standalone OLED wiring/display test firmware.
- `fz1_capture.pio` - PIO program for sampling the FZ-1 display bus.
- `font.h` - 256-character FZ-compatible 6x8 font table.
- `FZ1wiring-original.md` - Wiring reference for the original hand-wired build and 3D printed mount.
- `FZ1wiring-deftaudio-pcb.md` - Wiring and bring-up reference for the Deftaudio PCB build.
- `images/` - Photos of the original display board, cut connector board, assembled electronics, and installed working display.
- `3D printed mount/` - STL files for printing the display mounting plate and brackets.
- `pcb/deftaudio_pico_oled/` - Deftaudio KiCad PCB project and exported Gerber/drill manufacturing files.
- `pcb/deftaudio_connector_adapters/` - Deftaudio KiCad projects and Gerber zips for optional FZ-series IDC connector adapter boards.

## Image References

- `images/Original display board.jpeg` - Original FZ-1 display board before modification.
- `images/Original connector before cutting.jpeg` - Original connector area before cutting the donor display board.
- `images/Pico and display wiring.jpeg` - Pico, OLED, and resistor divider wiring.
- `images/Full display adapter.jpeg` - Assembled display adapter wiring.
- `images/Boot logo.jpeg` - Working OLED showing the FZ-1 boot logo.
- `images/Main menu.jpeg` - Working OLED showing the FZ-1 main menu.

## Build Options

The Pico passively listens to the FZ-1 display bus. It does not replace or emulate the FZ-1 CPU; it only translates the original display traffic into SSD1309 OLED drawing commands.

There are two supported hardware approaches.

### Original Hand-Wired Method

This is the original prototype method. It reuses the FZ-1 display connector or a cut-down section of the original display board, wires the Pico by hand through resistor dividers, and mounts the OLED with the 3D printed parts in `3D printed mount/`.

Use:

- Wiring file: `FZ1wiring-original.md`
- Main firmware: `FZ1_display_original_*.uf2`
- OLED test firmware: `FZ1_oled_test_original_*.uf2`

### Deftaudio PCB Method

This method uses the Deftaudio Pico OLED PCB. The board can be ordered by uploading `pcb/deftaudio_pico_oled/Pico_FZ1_OLED_PCB_gerbers.zip` to a PCB manufacturer such as PCBWay or a similar service. The loose Gerber/drill files are also included in `pcb/deftaudio_pico_oled/export1.0/`. The Pico and OLED attach to the PCB, and the PCB assembly then mounts directly in the synth.

Deftaudio also provides optional IDC connector adapter boards for connecting the synth display harness to the Pico OLED PCB. The FZ-1 adapter and the FZ-10M/FZ-20M/VZ-10M adapter are included in `pcb/deftaudio_connector_adapters/`, each with KiCad source files and a ready-to-upload Gerber zip.

Use:

- Wiring file: `FZ1wiring-deftaudio-pcb.md`
- PCB files: `pcb/deftaudio_pico_oled/`
- Optional connector adapter PCB files: `pcb/deftaudio_connector_adapters/`
- Main firmware: `FZ1_display_deftaudio_pcb_*.uf2`
- OLED test firmware: `FZ1_oled_test_deftaudio_pcb_*.uf2`

## Firmware Variants

The two hardware methods need different firmware because the OLED wiring and physical orientation are not the same.

| Variant | Use this for | OLED DC/A0 | OLED RES/RST | OLED orientation |
| --- | --- | --- | --- | --- |
| `original` | Hand wiring and the 3D printed mount in this repository | GP20 | GP21 | Original orientation |
| `deftaudio_pcb` | First Deftaudio PCB layout based on the earlier wiring guide | GP21 | GP20 | Rotated 180 degrees |

Important: disconnect the Pico from USB when powering it from the FZ-1 `VSYS` connection, unless your hardware setup specifically prevents back-powering.

For Pico 2 testing, the firmware builds with `PICO_BOARD=pico2` using the same GP0-GP6 FZ-1 bus pin mapping. The OLED pin mapping still depends on the selected `original` or `deftaudio_pcb` firmware variant.

## Parts

Core parts for both methods:

- Raspberry Pi Pico or compatible RP2040 Pico board.
- Raspberry Pi Pico 2 for experimental RP2350 testing.
- 128x64 SPI OLED module using SSD1309 controller.

For the original hand-wired method:

- Casio FZ-1 with original display connector/board available.
- 1k and 2k resistors for voltage dividers on the FZ-1 bus signals.
- Hookup wire, soldering tools, and basic mounting hardware.
- 3D printed OLED mount and brackets from `3D printed mount/`.

For the Deftaudio PCB method:

- PCB ordered from `pcb/deftaudio_pico_oled/Pico_FZ1_OLED_PCB_gerbers.zip`.
- Optional connector adapter PCB ordered from the matching Gerber zip in `pcb/deftaudio_connector_adapters/`.
- Components and headers required by the Deftaudio PCB design.
- Soldering tools and mounting hardware appropriate to the PCB assembly.

The prototype uses resistor dividers rather than active level-shifter ICs. Bidirectional auto-sensing level shifters were originally tested and proved incompatible with the FZ-1 display bus.

Pico 2 / RP2350 builds are provided for experimenters, but they have not yet been confirmed on real FZ-1 hardware. If you are building the mod today, use the resistor-divider wiring shown in the wiring guide for your hardware method. Do not connect FZ-1 5V display-bus signals directly to any Pico 2 pin unless you have checked the exact board, power conditions, and GPIO limits against the official [RP2350 datasheet](https://pip.raspberrypi.com/documents/RP-008373-DS-2-rp2350-datasheet.pdf) and [Pico 2 datasheet](https://pip.raspberrypi.com/documents/RP-008299-DS-pico-2-datasheet.pdf).

## Physical Installation

For the original hand-wired method, the prototype reuses part of the original display board as a connector breakout. The original display PCB is cut so that the FZ-1 ribbon connector section can be retained, and wires are soldered to that retained connector section. The OLED is held in a custom 3D printed mount designed to match the shape of the display module and closely follow the original FZ-1 mounting plate.

For the Deftaudio PCB method, order the main board by uploading `pcb/deftaudio_pico_oled/Pico_FZ1_OLED_PCB_gerbers.zip`, assemble the Pico and OLED onto the PCB, then install the completed PCB assembly in the synth. If you want a PCB adapter for the synth-side display connector, order the matching board from `pcb/deftaudio_connector_adapters/`: one design is for FZ-1, and the other is for FZ-10M/FZ-20M/VZ-10M. This avoids the large point-to-point wiring loom used by the original prototype method.

Photos and installation references should be placed in `images/`. 3D printable files should be placed in `3D printed mount/`.

## Firmware Overview

The firmware has five main jobs:

1. Initialize the SSD1309 OLED over SPI.
2. Use PIO to sample the FZ-1 display bus on PH2 clock edges while #CE is active.
3. Decode the FZ-1 command/data packets.
4. Render text and graphics to the OLED using the FZ-1 font.
5. Enter a framebuffer-backed idle screensaver after five minutes without FZ-1 display content changes.

Implementation details:

- FZ-1 data lines are active-low.
- The physical data wiring is reversed, so decoded nibbles are bit-reversed with a lookup table.
- Hardware testing showed that each nibble is captured twice in adjacent pairs. The parser conservatively collapses packets only when every adjacent pair matches.
- The FZ-1 cursor arrow uses extended font glyph `0xEE`, so the renderer uses the full 256-entry font table.
- The screensaver draws a sparse twinkling star field, then restores the last real FZ-1 display image before processing new bus activity.

## Building The Firmware

Install the Raspberry Pi Pico SDK and an ARM embedded GCC toolchain. On macOS, the tested working setup for this project uses the `gcc-arm-embedded` Homebrew cask.

Configure and build:

```bash
cmake -S . -B build -DPICO_SDK_PATH=/path/to/pico-sdk
cmake --build build
```

The default Pico build produces:

```text
build/FZ1_display_original.uf2
build/FZ1_display_deftaudio_pcb.uf2
build/FZ1_oled_test_original.uf2
build/FZ1_oled_test_deftaudio_pcb.uf2
```

To build for Pico 2:

```bash
cmake -S . -B build-pico2 -DPICO_SDK_PATH=/path/to/pico-sdk -DPICO_BOARD=pico2
cmake --build build-pico2
```

That produces:

```text
build-pico2/FZ1_display_original.uf2
build-pico2/FZ1_display_deftaudio_pcb.uf2
build-pico2/FZ1_oled_test_original.uf2
build-pico2/FZ1_oled_test_deftaudio_pcb.uf2
```

## Flashing The Pico

The easiest route is to download a prebuilt UF2 file from the repository's GitHub Releases page:

- `FZ1_display_original_pico.uf2` - Main firmware for Raspberry Pi Pico using the original hand-wired method.
- `FZ1_display_deftaudio_pcb_pico.uf2` - Main firmware for Raspberry Pi Pico using the first Deftaudio PCB layout.
- `FZ1_oled_test_original_pico.uf2` - OLED wiring test for Raspberry Pi Pico using the original hand-wired method.
- `FZ1_oled_test_deftaudio_pcb_pico.uf2` - OLED wiring test for Raspberry Pi Pico using the first Deftaudio PCB layout.
- `FZ1_display_original_pico2.uf2` - Experimental main firmware for Raspberry Pi Pico 2 using the original hand-wired method.
- `FZ1_display_deftaudio_pcb_pico2.uf2` - Experimental main firmware for Raspberry Pi Pico 2 using the first Deftaudio PCB layout.
- `FZ1_oled_test_original_pico2.uf2` - OLED wiring test for Raspberry Pi Pico 2 using the original hand-wired method.
- `FZ1_oled_test_deftaudio_pcb_pico2.uf2` - OLED wiring test for Raspberry Pi Pico 2 using the first Deftaudio PCB layout.

If you are modifying the firmware, build it locally and use the UF2 produced in your build directory.

1. Hold the Pico `BOOTSEL` button while connecting it over USB.
2. The Pico appears as a USB mass-storage device.
3. Copy the correct `FZ1_oled_test_*.uf2` file first if you are doing display-side bring-up, or the correct `FZ1_display_*.uf2` file for normal use.
4. The Pico reboots automatically into the firmware.

Once installed in the FZ-1, the firmware is silent: there is no boot test pattern and no serial output.

Maintainers can create a GitHub Release by pushing a version tag such as `v0.1.0`. The release workflow builds Pico and Pico 2 UF2 files and attaches them to the release automatically.

## OLED Wiring Test Firmware

The repository builds a special OLED test firmware for each hardware method. Use it before connecting the FZ-1 display bus, or before installing the Deftaudio PCB assembly in the synth, to confirm that the Pico can drive the OLED correctly.

Choose the test firmware that matches the hardware:

- Original hand-wired build: `FZ1_oled_test_original_pico.uf2` or `FZ1_oled_test_original_pico2.uf2`
- Deftaudio PCB build: `FZ1_oled_test_deftaudio_pcb_pico.uf2` or `FZ1_oled_test_deftaudio_pcb_pico2.uf2`

The test firmware cycles through:

- A full-screen checkerboard.
- A text screen showing `FZ-1 OLED TEST`, the selected firmware variant, and `SSD1309 SPI OK`.
- A full-white screen.
- A blank screen.

If this test works but the main FZ-1 firmware does not, the OLED side is probably good and the next area to check is the FZ-1 bus connection or the selected firmware variant.

## Testing

Basic checks:

- OLED should clear at startup.
- FZ-1 boot screen should render correctly.
- Main menu text should be aligned and readable.
- Cursor arrow should appear and move with the FZ-1 cursor buttons.
- Switching pages and menus should not leave stale pixels or corrupt text.
- After five minutes without FZ-1 display content changes, the screen should switch to a sparse twinkling star field and then restore the previous FZ-1 screen on the next real display update.

If the display is blank, verify power, common ground, OLED SPI wiring, and reset/DC pins.

If the display is garbled, verify the FZ-1 bus wiring order, resistor dividers, and the GP0-GP6 contiguous capture wiring.

## Safety Notes

This modification requires opening the synthesizer and working near vintage electronics. Work carefully, disconnect power before soldering, and double-check the 5V-to-3.3V dividers before connecting the Pico.

Original Pico/RP2040 GPIOs are not 5V tolerant.

Pico 2/RP2350 direct 5V input testing is experimental. Do not assume that a wiring change is safe unless the exact board, power sequence, and GPIO limits have been checked against the official Raspberry Pi documentation.

## Credits

Special thanks to [Deftaudio](https://github.com/Deftaudio) for developing the Pico OLED PCB, allowing the PCB files to be hosted in this repository, and testing the Deftaudio PCB firmware on real Casio FZ-1, FZ-10, and FZ-20 hardware. That work makes the PCB build a much more practical option for other users.

This project was also developed with reference to the OLED upgrade material preserved in the [Deftaudio/Casio-FZ-1 repository](https://github.com/Deftaudio/Casio-FZ-1). Those files include a copy of the original AVR-based FZ/VZ OLED design by Dmitri Sytov, known as Dmitrins, who shared it on the [Yamahamusicians forum](https://yamahamusicians.com/forum/viewtopic.php?t=10666&start=240). The RP2040 firmware in this repository ports the concept to a Raspberry Pi Pico using PIO, DMA, and SPI.

## License

This project is released under the MIT License. See `LICENSE` for the full license text.

Third-party reference material is credited above and remains the work of its original authors.
