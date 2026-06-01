# Casio FZ-1 OLED Mod: Master Wiring Reference

## 1. Data Capture (FZ-1 -> Resistor Divider -> Pico)
*Note: All data lines must be contiguous on the Pico (GP0-GP6) for the PIO state machine to capture them in a single clock cycle.*

**The Voltage Divider:** For every data line, we drop the FZ-1's 5V logic to the Pico's 3.3V logic using two resistors:
1. Connect a **1kΩ resistor** from the FZ-1 pin to the Pico GPx pin.
2. Connect a **2kΩ resistor** from that same Pico GPx pin to Ground.

| FZ-1 Pin (Source) | Function | Resistor Divider Network | RP2040 Pico (Dest) |
| :--- | :--- | :--- | :--- |
| **Pin 1** | `D8` (Bit 3) | 1kΩ to GP0, 2kΩ from GP0 to GND | **GP0** |
| **Pin 2** | `D4` (Bit 2) | 1kΩ to GP1, 2kΩ from GP1 to GND | **GP1** |
| **Pin 3** | `D2` (Bit 1) | 1kΩ to GP2, 2kΩ from GP2 to GND | **GP2** |
| **Pin 4** | `D1` (Bit 0) | 1kΩ to GP3, 2kΩ from GP3 to GND | **GP3** |
| **Pin 9** | `PH2` (Clock)| 1kΩ to GP4, 2kΩ from GP4 to GND | **GP4** |
| **Pin 11** | `#CE` (Select)| 1kΩ to GP5, 2kΩ from GP5 to GND | **GP5** |
| **Pin 12** | `#OP` (Cmd/Dat)| 1kΩ to GP6, 2kΩ from GP6 to GND | **GP6** |

---

## 2. Power & Ground Distribution
*Note: The Pico's internal SMPS regulates the 5V from the FZ-1 down to 3.3V to power the OLED display.*

### The Power Rails
| Source Component | Pin | Destination Component | Pin | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| **FZ-1 Mainboard** | **Pin 5 (+5V)** | **RP2040 Pico** | **VSYS** (Pin 39) | Main System Power *(Note: Disconnect if using USB!)* |
| **RP2040 Pico** | **3V3_OUT** (Pin 36) | **SSD1309 OLED** | **VCC / VDD** | Display Power |

### The Common Ground Rail
| Source Component | Pin | Destination Component | Pin | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| **FZ-1 Mainboard** | **Pin 6 (GND)** | **RP2040 Pico** | **GND** (e.g., Pin 38)| System Ground |
| **RP2040 Pico** | **GND** | **SSD1309 OLED** | **GND** | System Ground |
| **Resistor Network** | **2kΩ Legs** | **RP2040 Pico** | **GND** | Divider Ground Reference |

---

## 3. Display Output (Pico -> OLED SPI)
*Note: These connections do not go through the level shifter. They are 3.3V logic wired directly from the Pico to the Display.*

| RP2040 Pico Pin | SPI Function | SSD1309 OLED Pin |
| :--- | :--- | :--- |
| **GP18** | SPI0 SCK | **SCL / SCK / D0** (Clock) |
| **GP19** | SPI0 TX | **SDA / MOSI / D1** (Data) |
| **GP20** | Gen GPIO | **RES / RST** (Reset) |
| **GP21** | Gen GPIO | **DC / A0** (Data/Command) |
| **GP17** | SPI0 CSn | **CS** (Chip Select) |

---

### Bench Checklist:
- [ ] **Check 5V First:** Before connecting the Pico or TXB0108, turn on the FZ-1 and use a multimeter to verify a steady ~5.0V on FZ-1 Pin 5 relative to Pin 6.
- [ ] **Verify Output Enable:** Ensure the `OE` pin on the TXB0108 is tied to 3.3V. If left floating or tied to ground, the chip will not transmit data.
- [ ] **Check Contiguity:** Verify that `GP0` through `GP6` are strictly in order, as the PIO state machine code relies on this physical sequence.