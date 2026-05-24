#ifndef DOOM_PORT_H
#define DOOM_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#define VGA_WIDTH  320
#define VGA_HEIGHT 200
#define VGA_SIZE   (VGA_WIDTH * VGA_HEIGHT)

/**
 * @brief Initializes the custom memory-mapped virtual file system, custom heap allocator,
 * and sets all PureDOOM engine callbacks. Finally, it initializes the Doom engine.
 */
void doom_port_init(void);

/**
 * @brief Reads the current states from the global key_states array, translates scan codes
 * to PureDOOM key values, fires down/up key events into PureDOOM, and updates the engine.
 * Should be called in the main game loop at the frame tick rate (35 Hz).
 */
void doom_port_update(void);

/**
 * @brief Copies the current Doom software framebuffer (indexed 8-bit palette format)
 * straight into the secondary VGA double buffer.
 */
void doom_draw_screen(void);

/**
 * @brief Global pointer to the secondary VGA double buffer.
 * Defaults to the standard hardware video memory segment at 0x000A0000.
 */
extern unsigned char *secondary_vga_buffer;

#include <stdint.h>
extern volatile uint8_t key_states[256];

#ifdef __cplusplus
}
#endif

#endif // DOOM_PORT_H
