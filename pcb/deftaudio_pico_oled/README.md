# Deftaudio Pico OLED PCB

This folder contains the Deftaudio Pico OLED PCB files for the Casio FZ-1 OLED replacement.

## Contents

- `Pico_FZ1_OLED_PCB.kicad_pro` - KiCad project.
- `Pico_FZ1_OLED_PCB.kicad_sch` - KiCad schematic.
- `Pico_FZ1_OLED_PCB.kicad_pcb` - KiCad PCB layout.
- `Pico_FZ1_OLED_PCB_gerbers.zip` - Ready-to-upload Gerber/drill package for PCB manufacture.
- `libs/` - Local KiCad symbol/footprint support files included with the project.
- `export1.0/` - Gerber and drill files for PCB manufacture.

Upload `Pico_FZ1_OLED_PCB_gerbers.zip` to a PCB manufacturer such as PCBWay or a similar service. The loose files in `export1.0/` are included for inspection or regeneration.

Use the repository firmware variant named `deftaudio_pcb` with this board. The original hand-wired firmware is not pin/orientation compatible with this PCB.
