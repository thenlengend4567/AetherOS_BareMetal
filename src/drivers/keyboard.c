#include "keyboard.h"

// Concurrent global scancode state array (1 for press, 0 for release)
volatile uint8_t key_states[256] = {0};

#define KEYBOARD_BUFFER_SIZE 256
static uint8_t keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile int keyboard_head = 0;
static volatile int keyboard_tail = 0;

/**
 * @brief PS/2 Keyboard IRQ 1 Handler.
 * Reads scancodes from keyboard controller buffer (Port 0x60)
 * and updates key_states array.
 */
void keyboard_handler(registers_t *state) {
    (void)state; // Prevent unused parameter compiler warning

    // Read the keyboard controller data port
    uint8_t scancode = inb(0x60);

    // Standard PS/2 Scancode Set 1 handling:
    // Scancodes with the high bit set (0x80) indicate key releases.
    // Scancodes without the high bit indicate key presses.
    if (scancode & 0x80) {
        uint8_t key = scancode & 0x7F;
        key_states[key] = 0; // Key released
    } else {
        key_states[scancode] = 1; // Key pressed
    }

    // Push to buffer
    int next = (keyboard_head + 1) % KEYBOARD_BUFFER_SIZE;
    if (next != keyboard_tail) {
        keyboard_buffer[keyboard_head] = scancode;
        keyboard_head = next;
    }
}

/**
 * @brief Pop a scancode from the keyboard ring buffer.
 */
int keyboard_pop_scancode(uint8_t *code) {
    if (keyboard_head == keyboard_tail) {
        return 0; // Empty
    }
    *code = keyboard_buffer[keyboard_tail];
    keyboard_tail = (keyboard_tail + 1) % KEYBOARD_BUFFER_SIZE;
    return 1;
}

/**
 * @brief Initialize the PS/2 Keyboard driver.
 */
void keyboard_init(void) {
    // Safely clear the key states array
    for (int i = 0; i < 256; i++) {
        key_states[i] = 0;
    }
    keyboard_head = 0;
    keyboard_tail = 0;

    // 1. Drain the PS/2 output buffer to clear BIOS leftovers
    while (inb(0x64) & 1) {
        inb(0x60);
    }

    // 2. Enable first PS/2 port (keyboard)
    outb(0x64, 0xAE);

    // 3. Read the controller configuration byte
    outb(0x64, 0x20);
    while (!(inb(0x64) & 1));
    uint8_t status = inb(0x60);

    // 4. Set bit 0 (enable first port interrupt)
    // Clear bit 4 (disable first port clock) to make sure keyboard interface is active
    status |= 0x01;  // Enable interrupt for first PS/2 port
    status &= ~0x10; // Make sure first port clock is enabled

    // 5. Write the configuration byte back
    outb(0x64, 0x60);
    while (inb(0x64) & 2); // Wait for input buffer to be empty
    outb(0x60, status);

    // 6. Tell the keyboard to enable scanning (sending 0xF4 to keyboard)
    while (inb(0x64) & 2);
    outb(0x60, 0xF4);
    
    // Read the acknowledgment (0xFA) if it arrives
    for (int i = 0; i < 1000; i++) {
        if (inb(0x64) & 1) {
            inb(0x60);
            break;
        }
    }

    // Register keyboard interrupt handler at IRQ 1 (remap offset handled by idt.c)
    irq_register_handler(1, keyboard_handler);
}

