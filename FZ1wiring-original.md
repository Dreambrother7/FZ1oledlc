# Original Hand-Wired Build Wiring

This wiring reference is for the original build method: reuse the FZ-1 display connector or connector section, wire the Pico by hand, and mount the OLED with the 3D printed parts in `3D printed mount/`.

Use the `original` firmware files with this wiring.

## FZ-1 Bus Capture

All FZ-1 bus signals are 5V logic. Each signal connected to the Pico must pass through a resistor divider:

1. Connect a 1k resistor from the FZ-1 signal to the Pico GPIO.
2. Connect a 2k resistor from that Pico GPIO to ground.

The Pico GPIOs must remain contiguous on GP0-GP6 because the PIO state machine samples them together.

| FZ-1 pin | FZ-1 signal | Divider output to Pico |
| --- | --- | --- |
| Pin 1 | D8 | GP0 |
| Pin 2 | D4 | GP1 |
| Pin 3 | D2 | GP2 |
| Pin 4 | D1 | GP3 |
| Pin 9 | PH2 clock | GP4 |
| Pin 11 | #CE | GP5 |
| Pin 12 | #OP | GP6 |

## OLED SPI

These connections are 3.3V logic and wire directly from the Pico to the SSD1309 OLED.

| OLED signal | Pico GPIO |
| --- | --- |
| CS | GP17 |
| SCK / SCL / D0 | GP18 |
| MOSI / SDA / D1 | GP19 |
| DC / A0 | GP20 |
| RES / RST | GP21 |

## Power

| Source | Destination | Purpose |
| --- | --- | --- |
| FZ-1 +5V | Pico VSYS | Pico power |
| Pico 3V3_OUT | OLED VCC / VDD | OLED power |
| FZ-1 GND | Pico GND and OLED GND | Common ground |

Disconnect the Pico USB cable when powering the Pico from FZ-1 VSYS unless your setup specifically prevents back-powering.

## Bench Checklist

- Verify about 5V on the FZ-1 power pin before connecting the Pico.
- Verify every resistor-divider output is around 3.3V or lower before connecting it to the Pico.
- Confirm GP0-GP6 are in the exact order shown above.
- Flash `FZ1_oled_test_original_*.uf2` and confirm the OLED test pattern before connecting the FZ-1 display bus.
- After the OLED test passes, flash `FZ1_display_original_*.uf2` for normal use.
