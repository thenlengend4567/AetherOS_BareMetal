#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include "idt.h"

// Scancode definitions (Set 1)
#define KEY_ESC         0x01
#define KEY_1           0x02
#define KEY_2           0x03
#define KEY_3           0x04
#define KEY_4           0x05
#define KEY_5           0x06
#define KEY_6           0x07
#define KEY_7           0x08
#define KEY_8           0x09
#define KEY_9           0x0A
#define KEY_0           0x0B
#define KEY_MINUS       0x0C
#define KEY_EQUAL       0x0D
#define KEY_BACKSPACE   0x0E
#define KEY_TAB         0x0F

// WASD and gameplay keys
#define KEY_Q           0x10
#define KEY_W           0x11
#define KEY_E           0x12
#define KEY_R           0x13
#define KEY_T           0x14
#define KEY_Y           0x15
#define KEY_U           0x16
#define KEY_I           0x17
#define KEY_O           0x18
#define KEY_P           0x19
#define KEY_A           0x1E
#define KEY_S           0x1F
#define KEY_D           0x20
#define KEY_F           0x21

#define KEY_ENTER       0x1C
#define KEY_LCTRL       0x1D
#define KEY_LSHIFT      0x2A
#define KEY_RSHIFT      0x36
#define KEY_LALT        0x38
#define KEY_SPACE       0x39
#define KEY_CAPSLOCK    0x3A

// Arrow keys (standard Set 1 base scancodes)
#define KEY_UP          0x48
#define KEY_LEFT        0x4B
#define KEY_RIGHT       0x4D
#define KEY_DOWN        0x50

// Concurrent global scancode state array
// 1 = Pressed, 0 = Released
extern volatile uint8_t key_states[256];

/**
 * @brief Initialize the PS/2 Keyboard driver.
 * Registers the keyboard interrupt handler and unmasks IRQ 1 on PIC.
 */
void keyboard_init(void);

/**
 * @brief The PS/2 Keyboard IRQ 1 C handler.
 */
void keyboard_handler(registers_t *state);

/**
 * @brief Pop a scancode from the keyboard ring buffer.
 * @param code Pointer to store the popped scancode.
 * @return 1 if a scancode was popped, 0 if buffer was empty.
 */
int keyboard_pop_scancode(uint8_t *code);

#endif // KEYBOARD_H
