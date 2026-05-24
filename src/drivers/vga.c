/**
 * @file vga.c
 * @brief VGA Mode 13h Driver Implementation
 */

#include "vga.h"

/* VGA buffer pointers */
#define VGA_FRONT_BUFFER ((volatile uint8_t *)VGA_FRONT_BUFFER_PHYS)
#define VGA_BACK_BUFFER  ((volatile uint8_t *)VGA_BACK_BUFFER_PHYS)

/* Register IO Helper functions */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* VGA controller register arrays to program Mode 13h (320x200 256-color) */
static const uint8_t mode13h_misc = 0x63;

static const uint8_t mode13h_seq[] = {
    0x03, /* Reset */
    0x01, /* Clocking Mode */
    0x0F, /* Map Mask */
    0x00, /* Character Map Select */
    0x0E  /* Memory Mode */
};

static const uint8_t mode13h_crtc[] = {
    0x5F, /* Horizontal Total */
    0x4F, /* Horizontal Display Enable End */
    0x50, /* Start Horizontal Blanking */
    0x82, /* End Horizontal Blanking */
    0x54, /* Start Horizontal Retrace */
    0x80, /* End Horizontal Retrace */
    0xBF, /* Vertical Total */
    0x1F, /* Overflow */
    0x00, /* Preset Row Scan */
    0x41, /* Maximum Scan Line */
    0x00, /* Cursor Start */
    0x00, /* Cursor End */
    0x00, /* Start Address High */
    0x00, /* Start Address Low */
    0x00, /* Cursor Location High */
    0x00, /* Cursor Location Low */
    0x9C, /* Vertical Retrace Start */
    0x8E, /* Vertical Retrace End */
    0x8F, /* Vertical Display Enable End */
    0x28, /* Offset */
    0x40, /* Underline Location */
    0x96, /* Start Vertical Blanking */
    0xB9, /* End Vertical Blanking */
    0xA3, /* Mode Control */
    0xFF  /* Line Compare */
};

static const uint8_t mode13h_gc[] = {
    0x00, /* Set/Reset */
    0x00, /* Enable Set/Reset */
    0x00, /* Color Compare */
    0x00, /* Data Rotate */
    0x00, /* Read Map Select */
    0x40, /* Mode */
    0x05, /* Miscellaneous */
    0x0F, /* Color Don't Care */
    0xFF  /* Bit Mask */
};

static const uint8_t mode13h_ac[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, /* Palette Registers */
    0x41, /* Mode Control */
    0x00, /* Overscan Color */
    0x0F, /* Color Plane Enable */
    0x00, /* Horizontal Pixel Panning */
    0x00  /* Color Select */
};

void vga_init(void) {
    vga_enter_mode13h();
    vga_clear_front_buffer(0);
    vga_clear_back_buffer(0);
}

void vga_enter_mode13h(void) {
    /* 1. Write Miscellaneous Output Register */
    outb(0x3C2, mode13h_misc);

    /* 2. Write Sequencer Registers */
    for (uint8_t i = 0; i < 5; i++) {
        outb(0x3C4, i);
        outb(0x3C5, mode13h_seq[i]);
    }

    /* 3. Unlock CRTC registers: CRTC[17] (0x11) bit 7 controls write-protect of CRTC[0..7].
     * CRTC[3] (0x03) bit 7 must also be set for standard compatibility. */
    outb(0x3D4, 0x03);
    outb(0x3D5, inb(0x3D5) | 0x80);
    outb(0x3D4, 0x11);
    outb(0x3D5, inb(0x3D5) & ~0x80);

    /* 4. Write CRT Controller Registers */
    for (uint8_t i = 0; i < 25; i++) {
        outb(0x3D4, i);
        outb(0x3D5, mode13h_crtc[i]);
    }

    /* 5. Write Graphics Controller Registers */
    for (uint8_t i = 0; i < 9; i++) {
        outb(0x3CE, i);
        outb(0x3CF, mode13h_gc[i]);
    }

    /* 6. Write Attribute Controller Registers */
    for (uint8_t i = 0; i < 21; i++) {
        inb(0x3DA); /* Reset Attribute Controller internal flip-flop */
        outb(0x3C0, i);
        outb(0x3C0, mode13h_ac[i]);
    }

    /* 7. Unblank screen (enable palette & video output) */
    inb(0x3DA);
    outb(0x3C0, 0x20);
}

void vga_set_palette(const uint8_t *palette_data) {
    outb(0x3C8, 0); /* Start at palette index 0 */
    for (int i = 0; i < 256 * 3; i++) {
        /* Doom uses standard 8-bit color channels (0-255).
         * VGA DAC registers are 6-bit (0-63).
         * We scale down by shifting right by 2 (equivalent to division by 4). */
        uint8_t color_6bit = palette_data[i] >> 2;
        outb(0x3C9, color_6bit);
    }
}

void vga_set_palette_entry(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);
    outb(0x3C9, r >> 2);
    outb(0x3C9, g >> 2);
    outb(0x3C9, b >> 2);
}

void vga_blit(void) {
    uint32_t esi_val = VGA_BACK_BUFFER_PHYS;
    uint32_t edi_val = VGA_FRONT_BUFFER_PHYS;
    uint32_t ecx_val = VGA_SCREEN_SIZE / 4; /* Copying 32-bit dwords: 64,000 / 4 = 16,000 */

    __asm__ volatile (
        "cld\n\t"
        "rep movsd"
        : "+S"(esi_val), "+D"(edi_val), "+c"(ecx_val)
        :
        : "memory"
    );
}

void vga_clear_back_buffer(uint8_t color) {
    uint32_t val32 = ((uint32_t)color << 24) | ((uint32_t)color << 16) | ((uint32_t)color << 8) | color;
    uint32_t edi_val = VGA_BACK_BUFFER_PHYS;
    uint32_t ecx_val = VGA_SCREEN_SIZE / 4; /* Writing 32-bit dwords: 16,000 dwords */

    __asm__ volatile (
        "cld\n\t"
        "rep stosl"
        : "+D"(edi_val), "+c"(ecx_val)
        : "a"(val32)
        : "memory"
    );
}

void vga_clear_front_buffer(uint8_t color) {
    uint32_t val32 = ((uint32_t)color << 24) | ((uint32_t)color << 16) | ((uint32_t)color << 8) | color;
    uint32_t edi_val = VGA_FRONT_BUFFER_PHYS;
    uint32_t ecx_val = VGA_SCREEN_SIZE / 4;

    __asm__ volatile (
        "cld\n\t"
        "rep stosl"
        : "+D"(edi_val), "+c"(ecx_val)
        : "a"(val32)
        : "memory"
    );
}

void vga_write_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        VGA_FRONT_BUFFER[y * VGA_WIDTH + x] = color;
    }
}

void vga_write_back_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        VGA_BACK_BUFFER[y * VGA_WIDTH + x] = color;
    }
}

uint8_t vga_read_pixel(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        return VGA_FRONT_BUFFER[y * VGA_WIDTH + x];
    }
    return 0;
}

uint8_t vga_read_back_pixel(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        return VGA_BACK_BUFFER[y * VGA_WIDTH + x];
    }
    return 0;
}

void vga_draw_rect(int x, int y, int width, int height, uint8_t color) {
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > VGA_WIDTH) { width = VGA_WIDTH - x; }
    if (y + height > VGA_HEIGHT) { height = VGA_HEIGHT - y; }

    for (int cy = y; cy < y + height; cy++) {
        for (int cx = x; cx < x + width; cx++) {
            VGA_FRONT_BUFFER[cy * VGA_WIDTH + cx] = color;
        }
    }
}

void vga_draw_rect_back(int x, int y, int width, int height, uint8_t color) {
    if (x < 0) { width += x; x = 0; }
    if (y < 0) { height += y; y = 0; }
    if (x + width > VGA_WIDTH) { width = VGA_WIDTH - x; }
    if (y + height > VGA_HEIGHT) { height = VGA_HEIGHT - y; }

    for (int cy = y; cy < y + height; cy++) {
        for (int cx = x; cx < x + width; cx++) {
            VGA_BACK_BUFFER[cy * VGA_WIDTH + cx] = color;
        }
    }
}
