# Deftaudio Connector Adapter PCBs

This folder contains Deftaudio's optional IDC connector adapter PCB designs for FZ-series OLED installations.

These boards are separate from the main Pico OLED PCB in `pcb/deftaudio_pico_oled/`. They provide model-specific connector adapter hardware for the synth-side display harness.

## Included Boards

| Folder | Use for | Upload-ready Gerber zip |
| --- | --- | --- |
| `fz1_oled_IDC_adapter_v2/` | Casio FZ-1 | `fz1_oled_IDC_adapter_v2_gerbers.zip` |
| `fz10m_fz20m_vz10m_oled_IDC_adapter/` | Casio FZ-10M, FZ-20M, and VZ-10M | `fz10m_fz20m_vz10m_oled_IDC_adapter_gerbers.zip` |

Each board folder includes the KiCad project files, local KiCad library files, loose Gerber/drill files in `export1.0/`, and a ready-to-upload Gerber zip for PCB manufacturers such as PCBWay.

Use the connector adapter that matches the synth model. The main firmware variant for the Deftaudio PCB method is still `deftaudio_pcb`.
