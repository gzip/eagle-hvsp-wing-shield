# ATtiny Fuse Recovery & HVSP Reference

This document details the AVR fuse settings that prevent standard low-voltage In-System Programming (ISP) on 8-pin ATtiny microcontrollers (e.g., ATtiny13, ATtiny25, ATtiny45, ATtiny85), and why High-Voltage Serial Programming (HVSP) is required to recover them.


## Background: AVR Fuse Conventions

In AVR architecture, fuse logic is inverted:
* **`0` = Programmed** (feature is active)
* **`1` = Unprogrammed** (feature is inactive)

When programming fuses, bit values of `0` active features, and `1` deactives them.

## ISP Requirements

Standard ISP programming requires four conditions to communicate with an ATtiny:
1. The **`RESET` pin (Pin 1 / PB5)** must be pulled low to `0V` to hold the core in reset.
2. The **`SPIEN`** fuse must be enabled (`0`) so the chip responds to SPI commands.
3. The chip must have an active **system clock** running.
4. The **`DWEN`** (debugWIRE) fuse must not be active.

If any of these conditions are violated, standard ISP becomes impossible.

## Fuses Requiring HVSP Recovery

### 1. `RSTDISBL` (Reset Disable) — *Most Common*

* **Fuse Register:** `HFUSE` (High Fuse), **Bit 7** (Mask: `$80`)
* **Factory Default:** `1` (Unprogrammed: Pin 1 acts as external reset)
* **Locked Value:** `0` (Programmed: Pin 1 converted to general-purpose I/O PB5 with reset disabled)

#### Why it locks out ISP:
Because 8-pin ATtiny devices have only 6 I/O pins, designers frequently program `RSTDISBL = 0` to reclaim Pin 1 (PB5) as a standard GPIO. 

However, standard ISP programmers require pulling Pin 1 to `0V` to enter programming mode. With `RSTDISBL` programmed, grounding Pin 1 is treated as a normal digital input reading `LOW` and the microcontroller continues normal program execution, ignoring all ISP commands.

#### Why HVSP recovers it:
HVSP applies **~12V** to Pin 1. An internal high-voltage detector overrides the `RSTDISBL` fuse configuration at the hardware level, forcing the device into serial programming mode.

### 2. `SPIEN` (Serial Programming Enable)

* **Fuse Register:** `HFUSE` (High Fuse), **Bit 5** (Mask: `$20`)
* **Factory Default:** `0` (Programmed: SPI ISP is enabled)
* **Locked Value:** `1` (Unprogrammed: SPI ISP is disabled)

#### Why it locks out ISP:
`SPIEN` is the master enable switch for low-voltage In-System Programming over MOSI/MISO/SCK. If set to `1`, the internal SPI programming interface is completely shut off, and the chip will not respond to any ISP programmer.

While tools like `avrdude` refuse to disable `SPIEN` over ISP as a safety safeguard, corrupted writes or third-party programmers can disable it.

#### Why HVSP recovers it:
HVSP does not use the standard SPI protocol or respect the `SPIEN` bit. The high-voltage entry sequence grants full access to rewrite all fuse registers regardless of `SPIEN`.

### 3. `DWEN` (debugWIRE Enable)

* **Fuse Register:** `HFUSE` (High Fuse), **Bit 6** (Mask: `$40`)
* **Factory Default:** `1` (Unprogrammed: debugWIRE disabled)
* **Locked Value:** `0` (Programmed: debugWIRE active)

#### Why it locks out ISP:
When `DWEN` is enabled (`0`), the reset line is repurposed as an open-drain asynchronous debug interface (debugWIRE). Standard ISP cannot take over the reset pin until debugWIRE is disabled via the debugger or cleared using high-voltage programming.

#### Why HVSP recovers it:
The ~12V pulse on Pin 1 immediately overrides debugWIRE and forces the chip into HVSP mode.

### 4. Clock Selection Fuses (`CKSEL[3:0]`)

* **Fuse Register:** `LFUSE` (Low Fuse), **Bits 3:0** (Mask: `$0F`)

#### Why it locks out ISP:
Standard ISP requires a running system clock on the target ATtiny (the ISP clock frequency must be less than 1/4 of the target clock speed).
* **External Crystal / Resonator (`CKSEL = 1111` or similar):** If programmed for an external crystal when the chip is used standalone without external crystal and loading capacitors, the chip has no clock source and cannot execute SPI commands.
* **External Clock Source (`CKSEL = 0000`):** Requires an active square-wave clock generator wired to Pin 2 (`PB3` / XTAL1).
* **Ultra-Low Clock Speed (128 kHz WDT Oscillator + `CKDIV8`):** Results in a 16 kHz system clock. An ISP programmer must clock under 4 kHz to communicate, which many USB programmers cannot achieve.

#### Why HVSP recovers it:
In HVSP mode, the microcontroller's internal clock circuitry is bypassed. The programmer supplies its own clock pulse directly on the **`SCI` pin (Serial Clock Input, Pin 2 / PB3)**, allowing programming regardless of what `CKSEL` is set to.

## Summary Reference Table

| Fuse Bit | Register & Bit | Bit Mask | Locked Value | Factory Default | Consequence for Standard ISP |
| :--- | :---: | :---: | :---: | :---: | :--- |
| **`RSTDISBL`** | `HFUSE` [7] | **`$80`** | **`0`** | `1` | Pin 1 is GPIO; device cannot enter reset mode. |
| **`DWEN`** | `HFUSE` [6] | **`$40`** | **`0`** | `1` | Reset line is hijacked by debugWIRE interface. |
| **`SPIEN`** | `HFUSE` [5] | **`$20`** | **`1`** | `0` | SPI programming interface is completely disabled. |
| **`CKSEL[3:0]`** | `LFUSE` [3:0] | **`$0F`** | *Missing clock* | `0010` (8MHz RC) | Target has no running clock to process SPI commands. |

## Recovery Target Values (Factory Defaults)

Programming the following fuse bytes via HVSP restores the device to a fully working factory state:

* **`HFUSE = 0xDF`** (`1101 1111`):
  * `RSTDISBL` = `1` (Reset enabled)
  * `DWEN` = `1` (debugWIRE disabled)
  * `SPIEN` = `0` (SPI ISP enabled)
  * `BODLEVEL` = `111` (Brown-Out Detection disabled)
* **`LFUSE = 0x62`** (`0110 0010`):
  * `CKDIV8` = `0` (Divide clock by 8 enabled $\rightarrow$ 1 MHz system clock)
  * `CKSEL` = `0010` (Calibrated internal 8 MHz RC oscillator)
  * `SUT` = `10` (Slowly rising power start-up delay)
