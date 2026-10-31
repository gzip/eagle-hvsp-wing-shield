#define F_CPU 1000000UL  // 1 MHz internal oscillator (factory default with CKDIV8)

// Suppress false-positive -Warray-bounds warning emitted by GCC 12+ on AVR I/O register dereferencing
#pragma GCC diagnostic ignored "-Warray-bounds"

#include <avr/io.h>
#include <avr/power.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <stdint.h>

// Factory default fuse configuration for ATtiny25/45/85:
// HFUSE = 0xDF (SPIEN=0, BOD disabled, RST enabled)
// LFUSE = 0x62 (Internal 8MHz RC oscillator with CKDIV8 = 1MHz default)
#define HFUSE 0xDF
#define LFUSE 0x62

/*
 * Pin bit definitions on PORTB of programmer ATtiny85:
 *
 * Right Side (JP2 Header) -> Shield Pins 8, 9, 10:
 *   PB0 (Pin 5 / JP2 Pin 4) -> Shield Pin 10 (SII - Serial Instruction Input)
 *   PB1 (Pin 6 / JP2 Pin 3) -> Shield Pin 9  (SDI - Serial Data Input)
 *   PB2 (Pin 7 / JP2 Pin 2) -> Shield Pin 8  (Target Switched VCC)
 *
 * (Shield Pin 11 is SDO - Serial Data Output, unused in blind mode)
 *
 * Left Side (JP1 Header) -> Shield Pins 12, 13, 14:
 *   PB3 (Pin 2 / JP1 Pin 2) -> Shield Pin 12 (SCI - Serial Clock Input)
 *   PB4 (Pin 3 / JP1 Pin 3) -> Shield Pin 13 (HV_RST - 2N3904 12V Trigger)
 *   PB5 (Pin 1)             -> Programmer Reset (Kept as standard ISP Reset)
 */
#define INSTOUT     PB0
#define DATAOUT     PB1
#define TARGET_VCC  PB2
#define CLKOUT      PB3
#define HV_RST      PB4

static inline void clock_pulse(void) {
    _delay_us(10);
    PORTB |= (1 << CLKOUT);
    _delay_us(10);
    PORTB &= ~(1 << CLKOUT);
    _delay_us(10);
}

static void send_frame(uint8_t data, uint8_t instr) {
    // Start bit: SDI = 0, SII = 0, clock pulse
    PORTB &= ~((1 << DATAOUT) | (1 << INSTOUT));
    clock_pulse();

    // 8 data & instruction bits (MSB first)
    for (int8_t i = 7; i >= 0; i--) {
        if (data & (1 << i)) {
            PORTB |= (1 << DATAOUT);
        } else {
            PORTB &= ~(1 << DATAOUT);
        }

        if (instr & (1 << i)) {
            PORTB |= (1 << INSTOUT);
        } else {
            PORTB &= ~(1 << INSTOUT);
        }

        clock_pulse();
    }

    // End bits: SDI = 0, SII = 0, two clock pulses
    PORTB &= ~((1 << DATAOUT) | (1 << INSTOUT));
    clock_pulse();
    clock_pulse();
}

int main(void) {
    // Force internal clock prescaler to divide-by-8 (1 MHz system clock).
    // Guarantees F_CPU timing matches 1 MHz regardless of whether the
    // programmer's CKDIV8 fuse is programmed (0x62) or unprogrammed (0xE2).
    clock_prescale_set(clock_div_8);

    // 1. Configure all 5 pins as outputs
    DDRB |= (1 << TARGET_VCC) | (1 << HV_RST) | (1 << CLKOUT) | (1 << DATAOUT) | (1 << INSTOUT);

    // Initial idle state:
    //    Target VCC OFF (0V),
    //    HV_RST HIGH (2N3904 is turned ON, pulling Target RESET to 0V / 12V supply cut),
    //    All other signals LOW
    PORTB = (1 << HV_RST);
    _delay_ms(50);

    // Enter High-Voltage Serial Programming (HVSP) mode:
    //    a) Apply VCC (4.5V - 5.5V)
    //    b) Wait 20 - 60 us (Datasheet tVHV: min 20us, max 60us)
    //    c) Apply 12V to RESET (HV_RST LOW shuts off 2N3904, pulling RESET to 12V)
    //    d) Hold for at least 10 us (tHVRST) and wait 300us for internal stabilization
    PORTB |= (1 << TARGET_VCC);
    _delay_us(40);
    PORTB &= ~(1 << HV_RST);
    _delay_us(10);
    _delay_us(300);

    // Write High Fuse (HFUSE = 0xDF)
    send_frame(0x40, 0x4C);
    send_frame(HFUSE, 0x2C);
    send_frame(0x00, 0x74);
    send_frame(0x00, 0x7C);
    _delay_ms(30);  // Allow plenty of time for charge pump burn (datasheet max 4.5ms)

    // Write Low Fuse (LFUSE = 0x62)
    send_frame(0x40, 0x4C);
    send_frame(LFUSE, 0x2C);
    send_frame(0x00, 0x64);
    send_frame(0x00, 0x6C);
    _delay_ms(30);

    // Exit Programming Mode safely
    PORTB &= ~((1 << CLKOUT) | (1 << DATAOUT) | (1 << INSTOUT));
    PORTB |= (1 << HV_RST);       // Cut 12V from RESET
    _delay_us(40);
    PORTB &= ~(1 << TARGET_VCC);  // Cut Target VCC

    // Halt CPU in deep power-down sleep until next power cycle or reset
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();

    while (1) {
        // Fallback halt
    }
}
