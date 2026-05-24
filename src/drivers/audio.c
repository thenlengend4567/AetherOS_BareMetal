/**
 * @file audio.c
 * @brief PC Speaker and PIT Channel 2 Driver Implementation
 */

#include "audio.h"

/* Standard Port I/O helpers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* PIT Clock base frequency */
#define PIT_BASE_FREQUENCY 1193182

void audio_init(void) {
    /* Silent speaker on start */
    audio_stop_tone();
}

void audio_play_tone(uint32_t frequency) {
    if (frequency == 0) {
        audio_stop_tone();
        return;
    }

    /* Calculate 16-bit division factor */
    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;
    if (divisor > 0xFFFF) {
        divisor = 0xFFFF;
    }
    if (divisor < 1) {
        divisor = 1;
    }

    /* Set command register: Channel 2, LOBYTE/HIBYTE, Mode 3 (Square Wave), Binary */
    outb(0x43, 0xB6);

    /* Set data register: send LOBYTE first, then HIBYTE */
    outb(0x42, (uint8_t)(divisor & 0xFF));
    outb(0x42, (uint8_t)((divisor >> 8) & 0xFF));

    /* Connect PIT Channel 2 output to speaker
     * - Bit 0 of 0x61 controls PIT Channel 2 gate
     * - Bit 1 of 0x61 controls speaker data line
     * Both bits must be set to 1 to produce the square wave tone. */
    uint8_t val = inb(0x61);
    if ((val & 0x03) != 0x03) {
        outb(0x61, val | 0x03);
    }
}

void audio_stop_tone(void) {
    /* Disconnect PIT Channel 2 output from speaker and shut off PIT Channel 2 gate.
     * Clearing the lowest two bits achieves this. */
    uint8_t val = inb(0x61);
    outb(0x61, val & 0xFC);
}

/**
 * @brief Simple busy-wait millisecond-level delay helper.
 *        Calibrated roughly for generic x86 processors running in bare-metal emulators.
 */
static void delay_ms(uint32_t ms) {
    for (volatile uint32_t i = 0; i < ms; i++) {
        /* Inner loop calibrated roughly to provide 1ms on typical hardware/emulators */
        for (volatile uint32_t j = 0; j < 120000; j++) {
            __asm__ volatile ("nop");
        }
    }
}

void audio_beep(uint32_t frequency, uint32_t duration_ms) {
    audio_play_tone(frequency);
    delay_ms(duration_ms);
    audio_stop_tone();
}
