# ATtiny HVSP Rescue System & Connection Guide

This document describes how to use an **ATtiny Dev Board** as a standalone High-Voltage Serial Programmer (HVSP) controller to drive the **HVSP Wing Shield**, completely eliminating the need for an Arduino Uno. The tradeoff is that there's no serial terminal output to monitor and verify the operation so the execution is considered "blind".

## 1. System Overview

* **ATtiny Dev Board (Programmer)**: Runs the standalone rescue firmware ([`attiny/main.c`](attiny/main.c)) and generates the HVSP control signals. Powered by 5V (USB or header).
* **HVSP Wing Shield (Rescue Rig)**: Holds the 12V transistor circuit, the 12V DC input, and the 8-pin DIP socket where the target chip is placed for rescue.

## 2. Hardware Requirements

* 1x **ATtiny Dev Board** (running `attiny/main.c`)
* 1x **HVSP Wing Shield**
* 6x Dupont jumper wires (or two 1x3 ribbon cables)
* 1x **12V DC power source** (connected to the HVSP Shield)
* 1x **5V power source** (USB or power header for the Dev Board)

## 3. Flashing the Firmware to the Programmer ATtiny

Before wiring the boards together, flash `main.hex` to the ATtiny85 installed on the **ATtiny Dev Board** using your ISP programmer (e.g., Little Wire / USBtiny or USBasp connected to the Dev Board's 6-pin ISP header).

Note that the build script expects that `avrdude` is available somewhere in the system `PATH` and will exit if not found.

### Using `build.bat` (Windows CMD / PowerShell):
```cmd
:: Flash using Little Wire / USBtiny
build flash usbtiny

:: Or flash using USBasp
build flash usbasp
```

### Using `Makefile` (MSYS2 Terminal):
```bash
cd attiny

# Flash using Little Wire / USBtiny
make flash PROGRAMMER=usbtiny

# Or flash using USBasp
make flash
```

> [!NOTE]
> The rescue firmware runs with factory default fuses (`LFUSE = 0x62`, `HFUSE = 0xDF`), but also functions if configured for 8 MHz (`LFUSE = 0xE2`) because `main()` calls `clock_prescale_set(clock_div_8)` at startup to lock execution to 1 MHz. Its reset pin (`PB5`) remains a normal reset input, so you can reprogram it via standard ISP at any time.

## 4. Wiring

### HVSP Shield Header Layout
On the physical PCB silkscreen, pins **`8`** and **`12`** are labeled. 

The female header on the HVSP shield consists of pins 8 through 14 (arranged from left to right as viewed from the bottom edge of the board):

| Shield Pin | Silkscreen | Function | ATtiny Pin |
| :---: | :---: | :--- | :--- |
| **8** | `8` | **Target VCC** | `PB2` |
| **9** | | **SDI** (Serial Data In) | `PB1` |
| **10** | | **SII** (Serial Instruction In) | `PB0` |
| **11** | | **SDO** (Serial Data Out) | *(Unused)* |
| **12** | `12` | **SCI** (Serial Clock In) | `PB3` |
| **13** | | **HV Reset** (12V Transistor Trigger) | `PB4` |
| **14** | | **GND** | `GND` |

---

## 5. Rescue Operation

1. **Insert Chip**: Place the locked/bricked ATtiny into the 8-pin DIP socket on the **HVSP Wing Shield**.
2. **Connect 12V**: Plug a 12V DC power source into the HVSP shield.
3. **Power Programmer**: Power on the **ATtiny Dev Board** via the PWR-IN or ISP header. 
>*Pro Tip:* For USB powered 12V sources you can solder a wire to the positive rail of the incoming USB connector and connect the other end to the PWR-IN header of the ATtiny Dev Board.
4. **Reset Programmer**: Press the reset button on the **ATtiny Dev Board** to make sure it power cycles correctly while everything is connected.
5. **Instant Flash**: In under 150 milliseconds, the Dev Board executes the HVSP entry sequence, pulses 12V, flashes factory default fuses (`HFUSE = 0xDF`, `LFUSE = 0x62`), cuts power, and halts into deep sleep.
6. **Done**: Remove the rescued ATtiny from the DIP socket. It is now fully restored to factory defaults and responds to standard low-voltage ISP.
7. **Verify**: Place the chip back on your dev board and read the fuses with `avrdude`:

```bash
avrdude -c usbtiny -p t85 -U hfuse:r:-:h -U lfuse:r:-:h
```
```Expected Output
Processing -U lfuse:r:-:h
Reading lfuse memory ...
Writing 1 byte to output file <stdout>
0x62

Processing -U hfuse:r:-:h
Reading hfuse memory ...
Writing 1 byte to output file <stdout>
0xdf

Avrdude done.  Thank you.
```
