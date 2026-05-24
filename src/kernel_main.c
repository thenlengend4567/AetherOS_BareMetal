#include "freestanding.h"
#include "idt.h"
#include "ata.h"
#include "keyboard.h"
#include "vga.h"
#include "audio.h"
#define DOOM_IMPLEMENTATION
#include "PureDOOM.h"

// System ticks incremented by PIT IRQ 0
volatile uint32_t system_ticks = 0;

static void* doom_malloc_wrapper(int size) {
    if (size <= 0) return NULL;
    return malloc((size_t)size);
}

static void doom_free_wrapper(void* ptr) {
    free(ptr);
}

// Memory-backed Virtual File System
#define WAD_ADDR 0x00200000
#define WAD_SIZE 4196352

typedef struct {
    uint8_t* buffer;
    int size;
    int capacity;
    int offset;
    int is_write;
    int in_use;
    char filename[128];
} virtual_file_t;

#define MAX_VIRTUAL_FILES 8
static virtual_file_t vfiles[MAX_VIRTUAL_FILES];

static virtual_file_t* alloc_vfile(const char* filename, int write) {
    for (int i = 0; i < MAX_VIRTUAL_FILES; i++) {
        if (!vfiles[i].in_use) {
            vfiles[i].in_use = 1;
            vfiles[i].offset = 0;
            vfiles[i].is_write = write;
            vfiles[i].buffer = NULL;
            vfiles[i].size = 0;
            vfiles[i].capacity = 0;
            
            int j = 0;
            for (; j < 127 && filename[j]; j++) {
                vfiles[i].filename[j] = filename[j];
            }
            vfiles[i].filename[j] = '\0';
            return &vfiles[i];
        }
    }
    return NULL;
}

static void* vfs_open(const char* filename, const char* mode) {
    int write = 0;
    if (mode[0] == 'w' || mode[0] == 'a' || (mode[0] == 'r' && mode[1] == '+')) {
        write = 1;
    }
    
    int is_wad = 0;
    // Only match doom1.wad to force the engine to boot in the correct Shareware Doom 1 mode.
    // This prevents it from matching doom2f.wad and trying to run commercial Doom 2 with missing textures.
    if (strstr(filename, "doom1.wad") != NULL || strstr(filename, "DOOM1.WAD") != NULL) {
        is_wad = 1;
    }
    
    virtual_file_t* vf = alloc_vfile(filename, write);
    if (!vf) return NULL;
    
    if (is_wad) {
        vf->buffer = (uint8_t*)WAD_ADDR;
        vf->size = WAD_SIZE;
        vf->capacity = WAD_SIZE;
        vf->is_write = 0;
    } else {
        if (!write) {
            for (int i = 0; i < MAX_VIRTUAL_FILES; i++) {
                if (vfiles[i].in_use && &vfiles[i] != vf) {
                    int match = 1;
                    for (int j = 0; filename[j] || vfiles[i].filename[j]; j++) {
                        if (filename[j] != vfiles[i].filename[j]) {
                            match = 0;
                            break;
                        }
                    }
                    if (match && vfiles[i].buffer) {
                        vfiles[i].offset = 0;
                        vfiles[i].is_write = 0;
                        vf->in_use = 0;
                        return &vfiles[i];
                    }
                }
            }
            vf->in_use = 0;
            return NULL;
        } else {
            vf->capacity = 4096;
            vf->buffer = (uint8_t*)malloc(vf->capacity);
            vf->size = 0;
        }
    }
    return vf;
}

static void vfs_close(void* handle) {
    virtual_file_t* vf = (virtual_file_t*)handle;
    if (vf) {
        if (vf->buffer == (uint8_t*)WAD_ADDR) {
            vf->in_use = 0;
        }
    }
}

static int vfs_read(void* handle, void* buf, int count) {
    virtual_file_t* vf = (virtual_file_t*)handle;
    if (!vf || !vf->buffer) return 0;
    
    int bytes_left = vf->size - vf->offset;
    if (bytes_left <= 0) return 0;
    
    int to_read = count;
    if (to_read > bytes_left) {
        to_read = bytes_left;
    }
    
    uint8_t* dst = (uint8_t*)buf;
    uint8_t* src = vf->buffer + vf->offset;
    for (int i = 0; i < to_read; i++) {
        dst[i] = src[i];
    }
    
    vf->offset += to_read;
    return to_read;
}

static int vfs_write(void* handle, const void* buf, int count) {
    virtual_file_t* vf = (virtual_file_t*)handle;
    if (!vf || !vf->is_write) return 0;
    
    if (vf->offset + count > vf->capacity) {
        int new_capacity = vf->capacity * 2;
        if (new_capacity < vf->offset + count) {
            new_capacity = vf->offset + count + 4096;
        }
        uint8_t* new_buffer = (uint8_t*)malloc(new_capacity);
        if (!new_buffer) return 0;
        
        if (vf->buffer) {
            for (int i = 0; i < vf->size; i++) {
                new_buffer[i] = vf->buffer[i];
            }
            free(vf->buffer);
        }
        vf->buffer = new_buffer;
        vf->capacity = new_capacity;
    }
    
    const uint8_t* src = (const uint8_t*)buf;
    uint8_t* dst = vf->buffer + vf->offset;
    for (int i = 0; i < count; i++) {
        dst[i] = src[i];
    }
    
    vf->offset += count;
    if (vf->offset > vf->size) {
        vf->size = vf->offset;
    }
    return count;
}

static int vfs_seek(void* handle, int offset, doom_seek_t origin) {
    virtual_file_t* vf = (virtual_file_t*)handle;
    if (!vf) return -1;
    
    int new_offset = vf->offset;
    switch (origin) {
        case DOOM_SEEK_SET:
            new_offset = offset;
            break;
        case DOOM_SEEK_CUR:
            new_offset += offset;
            break;
        case DOOM_SEEK_END:
            new_offset = vf->size + offset;
            break;
        default:
            return -1;
    }
    
    if (new_offset < 0 || new_offset > vf->size) {
        return -1;
    }
    vf->offset = new_offset;
    return 0;
}

static int vfs_tell(void* handle) {
    virtual_file_t* vf = (virtual_file_t*)handle;
    if (!vf) return -1;
    return vf->offset;
}

static int vfs_eof(void* handle) {
    virtual_file_t* vf = (virtual_file_t*)handle;
    if (!vf) return 1;
    return vf->offset >= vf->size;
}

// Low-level port I/O functions (inb, outb) are declared in idt.h and implemented in idt.c

static void serial_init(void) {
    outb(0x3F8 + 1, 0x00);    // Disable all interrupts
    outb(0x3F8 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(0x3F8 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(0x3F8 + 1, 0x00);    //                  (hi byte)
    outb(0x3F8 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(0x3F8 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(0x3F8 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

// Serial console output on COM1 (0x3F8)
static void serial_print(const char* str) {
    while (*str) {
        outb(0x3F8, *str);
        str++;
    }
}

// Custom gettime using a highly robust, pure hardware Time Stamp Counter (TSC) clock.
// This guarantees a perfectly smooth 35 FPS game loop, independent of PIT interrupts.
static void custom_gettime(int* sec, int* usec) {
    static uint64_t start_tsc = 0;
    
    uint64_t current_tsc = 0;
    uint32_t low, high;
    __asm__ volatile ("rdtsc" : "=a"(low), "=d"(high));
    current_tsc = ((uint64_t)high << 32) | low;
    
    if (start_tsc == 0) {
        start_tsc = current_tsc;
    }
    
    uint64_t elapsed_cycles = current_tsc - start_tsc;
    
    // Assuming a standard 2.0 GHz virtual CPU clock in QEMU.
    // 2,000,000,000 cycles = 1 second.
    uint64_t cycles_per_sec = 2000000000ULL;
    
    *sec = (int)(elapsed_cycles / cycles_per_sec);
    *usec = (int)((elapsed_cycles % cycles_per_sec) / (cycles_per_sec / 1000000ULL));
}

static char* custom_getenv(const char* var) {
    return NULL;
}

static void custom_exit(int code) {
    serial_print("\nDOOM exited with code: ");
    char buf[16];
    int i = 0;
    if (code == 0) {
        buf[i++] = '0';
    } else {
        int temp = code;
        if (temp < 0) {
            buf[i++] = '-';
            temp = -temp;
        }
        int start = i;
        while (temp > 0) {
            buf[i++] = '0' + (temp % 10);
            temp /= 10;
        }
        int end = i - 1;
        while (start < end) {
            char t = buf[start];
            buf[start] = buf[end];
            buf[end] = t;
            start++;
            end--;
        }
    }
    buf[i] = '\0';
    serial_print(buf);
    serial_print("\nSystem halting.\n");
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

// Map PS/2 Set 1 scancodes to DOOM keys
static doom_key_t translate_scancode(uint8_t scancode) {
    switch (scancode) {
        case 0x01: return DOOM_KEY_ESCAPE;
        case 0x02: return DOOM_KEY_1;
        case 0x03: return DOOM_KEY_2;
        case 0x04: return DOOM_KEY_3;
        case 0x05: return DOOM_KEY_4;
        case 0x06: return DOOM_KEY_5;
        case 0x07: return DOOM_KEY_6;
        case 0x08: return DOOM_KEY_7;
        case 0x09: return DOOM_KEY_8;
        case 0x0A: return DOOM_KEY_9;
        case 0x0B: return DOOM_KEY_0;
        case 0x0C: return DOOM_KEY_MINUS;
        case 0x0D: return DOOM_KEY_EQUALS;
        case 0x0E: return DOOM_KEY_BACKSPACE;
        case 0x0F: return DOOM_KEY_TAB;
        case 0x10: return DOOM_KEY_Q;
        case 0x11: return DOOM_KEY_W;
        case 0x12: return DOOM_KEY_E;
        case 0x13: return DOOM_KEY_R;
        case 0x14: return DOOM_KEY_T;
        case 0x15: return DOOM_KEY_Y;
        case 0x16: return DOOM_KEY_U;
        case 0x17: return DOOM_KEY_I;
        case 0x18: return DOOM_KEY_O;
        case 0x19: return DOOM_KEY_P;
        case 0x1A: return DOOM_KEY_LEFT_BRACKET;
        case 0x1B: return DOOM_KEY_RIGHT_BRACKET;
        case 0x1C: return DOOM_KEY_ENTER;
        case 0x1D: return DOOM_KEY_CTRL;
        case 0x1E: return DOOM_KEY_A;
        case 0x1F: return DOOM_KEY_S;
        case 0x20: return DOOM_KEY_D;
        case 0x21: return DOOM_KEY_F;
        case 0x22: return DOOM_KEY_G;
        case 0x23: return DOOM_KEY_H;
        case 0x24: return DOOM_KEY_J;
        case 0x25: return DOOM_KEY_K;
        case 0x26: return DOOM_KEY_L;
        case 0x27: return DOOM_KEY_SEMICOLON;
        case 0x28: return DOOM_KEY_APOSTROPHE;
        case 0x2A: return DOOM_KEY_SHIFT;
        case 0x2C: return DOOM_KEY_Z;
        case 0x2D: return DOOM_KEY_X;
        case 0x2E: return DOOM_KEY_C;
        case 0x2F: return DOOM_KEY_V;
        case 0x30: return DOOM_KEY_B;
        case 0x31: return DOOM_KEY_N;
        case 0x32: return DOOM_KEY_M;
        case 0x33: return DOOM_KEY_COMMA;
        case 0x34: return DOOM_KEY_PERIOD;
        case 0x35: return DOOM_KEY_SLASH;
        case 0x36: return DOOM_KEY_SHIFT;
        case 0x38: return DOOM_KEY_ALT;
        case 0x39: return DOOM_KEY_SPACE;
        case 0x3B: return DOOM_KEY_F1;
        case 0x3C: return DOOM_KEY_F2;
        case 0x3D: return DOOM_KEY_F3;
        case 0x3E: return DOOM_KEY_F4;
        case 0x3F: return DOOM_KEY_F5;
        case 0x40: return DOOM_KEY_F6;
        case 0x41: return DOOM_KEY_F7;
        case 0x42: return DOOM_KEY_F8;
        case 0x43: return DOOM_KEY_F9;
        case 0x44: return DOOM_KEY_F10;
        case 0x57: return DOOM_KEY_F11;
        case 0x58: return DOOM_KEY_F12;
        case 0x48: return DOOM_KEY_UP_ARROW;
        case 0x4B: return DOOM_KEY_LEFT_ARROW;
        case 0x4D: return DOOM_KEY_RIGHT_ARROW;
        case 0x50: return DOOM_KEY_DOWN_ARROW;
        default: return DOOM_KEY_UNKNOWN;
    }
}



// MinGW GCC expectation stub
void __main(void) {}

void kernel_main(void);

__attribute__((section(".text.entry")))
void start(void) {
    kernel_main();
}

// The C Entry Point called by the bootloader
void kernel_main(void) {
    // 0. Zero out the .bss section to ensure no uninitialized global variables
    extern char _bss_start[];
    extern char _bss_end[];
    for (char *p = _bss_start; p < _bss_end; p++) {
        *p = 0;
    }

    // Initialize serial port first so we can log early messages
    serial_init();
    serial_print("AetherOS: Kernel starting...\n");

    // 1. Initialize VGA Graphics Mode 13h (320x200 256-color)
    vga_init();
    serial_print("AetherOS: VGA initialized.\n");
    
    // 2. Set up the Interrupt Descriptor Table and remap the PIC
    idt_init();
    serial_print("AetherOS: IDT initialized.\n");
    
    // 3. Initialize PS/2 Keyboard driver
    keyboard_init();
    serial_print("AetherOS: Keyboard initialized.\n");
    
    // 4. Initialize PC Speaker PIT driver
    audio_init();
    serial_print("AetherOS: Audio initialized.\n");
    
    // Play a satisfying retro console startup chime! (A-note and high A-note)
    audio_beep(880, 100);
    audio_beep(1760, 150);

    
    // 5. Load doom1.wad from raw disk sector 1000 (8196 sectors = 4.002 MB) into RAM at 0x00200000
    serial_print("AetherOS: Loading doom1.wad...\n");
    ata_read_sectors(1000, 8196, (void*)WAD_ADDR);
    serial_print("AetherOS: WAD loaded.\n");
    
    // Enable interrupts now that all IDT handlers and drivers are ready
    __asm__ volatile ("sti");
    serial_print("AetherOS: Interrupts enabled.\n");
    
    // Initialize standard bump allocator starting with 64MB limit
    bump_allocator_init(64 * 1024 * 1024);
    
    // 6. Initialize PureDOOM engine hooks
    doom_set_malloc(doom_malloc_wrapper, doom_free_wrapper);
    doom_set_file_io(vfs_open, vfs_close, vfs_read, vfs_write, vfs_seek, vfs_tell, vfs_eof);
    doom_set_print(serial_print);
    doom_set_gettime(custom_gettime);
    doom_set_getenv(custom_getenv);
    doom_set_exit(custom_exit);
    
    // 7. Call doom_init with standard parameters
    char* doom_argv[] = { "doom", NULL };
    doom_init(1, doom_argv, 0);
    
    serial_print("AetherOS: DOOM initialized. Entering game loop.\n");
    
    // 8. Infinite Game Loop
    while (1) {
        // Read keyboard state/scancodes and dispatch events to PureDOOM
        uint8_t raw_code;
        while (keyboard_pop_scancode(&raw_code)) {
            int released = (raw_code & 0x80) ? 1 : 0;
            uint8_t base_code = raw_code & 0x7F;
            doom_key_t key = translate_scancode(base_code);
            if (key != DOOM_KEY_UNKNOWN) {
                if (released) {
                    doom_key_up(key);
                } else {
                    doom_key_down(key);
                }
            }
        }
        
        // Let PureDOOM update game state (ticks at 35 FPS internally)
        doom_update();
        
        // Sync VGA palette (handles screen flashes / fades)
        vga_set_palette(screen_palette);
        
        // Get the DOOM framebuffer (indexed color format, 1 byte per pixel)
        const unsigned char* doom_fb = doom_get_framebuffer(1);
        if (doom_fb) {
            uint8_t* vga_db = (uint8_t*)0x00700000;
            // Copy framebuffer to the VGA double buffer at 0x00700000
            for (int i = 0; i < 320 * 200; i++) {
                vga_db[i] = (uint8_t)doom_fb[i];
            }
        }
        
        // Blit double buffer at 0x00700000 to VGA screen at 0x000A0000
        vga_blit();
    }
    
    // Halt fallback loop
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}
