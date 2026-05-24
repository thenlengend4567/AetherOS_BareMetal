/**
 * @file vga.h
 * @brief VGA Mode 13h (320x200, 256 colors) Driver Declarations
 *
 * Provides a fully functional bare-metal interface to VGA hardware in Mode 13h,
 * supporting direct framebuffer access, 0x00700000-based double buffering,
 * assembly-optimized blitting, and hardware palette configurations scaled for Doom.
 */

#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH       320
#define VGA_HEIGHT      200
#define VGA_SCREEN_SIZE (VGA_WIDTH * VGA_HEIGHT)

/* Physical RAM allocations matching OS spec */
#define VGA_FRONT_BUFFER_PHYS 0x000A0000
#define VGA_BACK_BUFFER_PHYS  0x00700000

/**
 * @brief Initializes the VGA subsystem by entering Mode 13h and clearing buffers.
 */
void vga_init(void);

/**
 * @brief Direct low-level entry into Mode 13h (320x200 256-color) by programming hardware registers.
 */
void vga_enter_mode13h(void);

/**
 * @brief Sets the whole 256-color palette.
 * @param palette_data Pointer to 768 bytes (256 * 3) of RGB triplet data.
 *                     Doom's 8-bit color channels (0-255) are scaled to VGA 6-bit (0-63).
 */
void vga_set_palette(const uint8_t *palette_data);

/**
 * @brief Sets a single entry in the hardware VGA palette.
 * @param index Palette index (0-255)
 * @param r Red component (8-bit, will be scaled to 6-bit)
 * @param g Green component (8-bit, will be scaled to 6-bit)
 * @param b Blue component (8-bit, will be scaled to 6-bit)
 */
void vga_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Blits (copies) the entire back buffer (64,000 bytes) at 0x00700000 to the
 *        VGA hardware segment 0x000A0000 using assembly string copy operations (rep movsd).
 */
void vga_blit(void);

/**
 * @brief Clears the back buffer with a specified color.
 * @param color 8-bit palette index color.
 */
void vga_clear_back_buffer(uint8_t color);

/**
 * @brief Clears the front (VGA video segment) buffer with a specified color.
 * @param color 8-bit palette index color.
 */
void vga_clear_front_buffer(uint8_t color);

/**
 * @brief Writes a pixel directly to the front (VGA video segment) buffer.
 * @param x X coordinate (0-319)
 * @param y Y coordinate (0-199)
 * @param color 8-bit palette index color
 */
void vga_write_pixel(int x, int y, uint8_t color);

/**
 * @brief Writes a pixel directly to the back buffer at 0x00700000.
 * @param x X coordinate (0-319)
 * @param y Y coordinate (0-199)
 * @param color 8-bit palette index color
 */
void vga_write_back_pixel(int x, int y, uint8_t color);

/**
 * @brief Reads a pixel directly from the front (VGA video segment) buffer.
 * @param x X coordinate (0-319)
 * @param y Y coordinate (0-199)
 * @return 8-bit palette index color, or 0 if out of bounds.
 */
uint8_t vga_read_pixel(int x, int y);

/**
 * @brief Reads a pixel directly from the back buffer.
 * @param x X coordinate (0-319)
 * @param y Y coordinate (0-199)
 * @return 8-bit palette index color, or 0 if out of bounds.
 */
uint8_t vga_read_back_pixel(int x, int y);

/**
 * @brief Draws a filled rectangle on the front buffer.
 */
void vga_draw_rect(int x, int y, int width, int height, uint8_t color);

/**
 * @brief Draws a filled rectangle on the back buffer.
 */
void vga_draw_rect_back(int x, int y, int width, int height, uint8_t color);

#endif /* VGA_H */
