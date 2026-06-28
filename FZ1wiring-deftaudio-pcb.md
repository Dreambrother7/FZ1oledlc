# Deftaudio PCB Build Wiring

This wiring reference is for the Deftaudio Pico OLED PCB method. The PCB files are in `pcb/deftaudio_pico_oled/`.

Use the `deftaudio_pcb` firmware files with this board.

## What The PCB Does

The Deftaudio PCB replaces most of the point-to-point wiring used by the original hand-wired build. It provides footprints/connectors for:

- Raspberry Pi Pico or Pico 2.
- SSD1309 128x64 SPI OLED module.
- FZ-1 display connector/header connection.
- The resistor-divider network used to bring FZ-1 5V bus signals down to Pico-safe logic levels.

The PCB layout mounts the OLED inverted compared with the original hand-wired build. It also routes OLED DC/A0 and RES/RST differently from the original wiring. The `deftaudio_pcb` firmware variant handles both differences in software by swapping the OLED control pins and rotating the SSD1309 output 180 degrees.

## PCB Files

The KiCad project is in:

```text
pcb/deftaudio_pico_oled/
```

The ready-to-upload manufacturing files are in:

```text
pcb/deftaudio_pico_oled/Pico_FZ1_OLED_PCB_gerbers.zip
```

The same Gerber and drill files are also available loose in:

```text
pcb/deftaudio_pico_oled/export1.0/
```

The zip file is the simplest option for PCB manufacturers such as PCBWay or similar board houses.

## FZ Display-Bus Header

The PCB FZ display-bus connector is `J2`, a 2x5 2.00 mm header in the KiCad files.

| J2 pin | PCB net | Function |
| --- | --- | --- |
| 1 | D4 | FZ display data D4 |
| 2 | D8 | FZ display data D8 |
| 3 | D1 | FZ display data D1 |
| 4 | D2 | FZ display data D2 |
| 5 | GND | Ground |
| 6 | +5 | FZ +5V supply |
| 7 | CE | FZ display chip enable |
| 8 | RH2 | FZ PH2 display clock; labelled `RH2` in the PCB files |
| 9 | GND | Ground |
| 10 | OP | FZ command/data select |

The PCB includes the resistor-divider network for the FZ 5V display-bus signals. The divided Pico-side nets route to the firmware capture pins as follows:

| FZ signal | Pico GPIO |
| --- | --- |
| D8 | GP0 |
| D4 | GP1 |
| D2 | GP2 |
| D1 | GP3 |
| PH2 clock / `RH2` PCB net | GP4 |
| CE | GP5 |
| OP | GP6 |

## OLED Header

The PCB OLED header is `J1`, a 1x7 2.54 mm header in the KiCad files. It is labelled for the OLED module signals:

| J1 pin | PCB/OLED signal | Pico GPIO | Purpose |
| --- | --- | --- | --- |
| 1 | GND | GND | Ground |
| 2 | 3.3V | 3V3_OUT | OLED power from Pico 3V3 |
| 3 | SCK | GP18 | SPI clock |
| 4 | SDA | GP19 | SPI MOSI data |
| 5 | RES | GP20 | OLED reset |
| 6 | DC | GP21 | OLED data/command |
| 7 | CS | GP17 | OLED chip select |

Because this board uses the Deftaudio routing and physical orientation, do not flash the `original` firmware for this method.

## Bring-Up Checklist

- Inspect the assembled board for solder bridges before installing it in the FZ-1.
- Confirm Pico orientation and OLED orientation against the PCB silkscreen/KiCad files before soldering.
- Flash `FZ1_oled_test_deftaudio_pcb_*.uf2` first and power only the Pico/OLED side to confirm the display works.
- The OLED test should cycle through checkerboard, text, white, and blank screens.
- After the OLED test passes, flash `FZ1_display_deftaudio_pcb_*.uf2`.
- Install/connect the board in the FZ-1 only after the display-side test is working.

Disconnect the Pico USB cable when powering the Pico from the FZ-1 unless your setup specifically prevents back-powering.
