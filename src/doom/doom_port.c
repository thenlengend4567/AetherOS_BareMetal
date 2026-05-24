#include <stddef.h>
#include "doom_port.h"
#include "../libc/freestanding.h"

// Define the DOOM implementation using the PureDOOM single-header library.
// We explicitly do NOT define DOOM_IMPLEMENT_MALLOC, DOOM_IMPLEMENT_FILE_IO,
// or DOOM_IMPLEMENT_PRINT, because we override them with our own custom bare-metal versions!
#define DOOM_IMPLEMENTATION
#include "PureDOOM.h"

/* -------------------------------------------------------------------------
   GLOBAL VARIABLES
   ------------------------------------------------------------------------- */

// Global keyboard status array is declared externally in doom_port.h and defined in keyboard.c

// Default VGA double buffer pointer to physical VGA memory in low address memory segment.
// Can be changed by standard drivers or linker scripts.
unsigned char *secondary_vga_buffer = (unsigned char *)0x000A0000;

/* -------------------------------------------------------------------------
   CUSTOM MEMORY ALLOCATOR (Boundary Tag Coalescing Allocator)
   ------------------------------------------------------------------------- */

#define HEAP_START 0x00700000
#define HEAP_SIZE  (16 * 1024 * 1024) // 16 MB custom heap in safe extended memory area

typedef struct block_header {
    int size;                      // Size of payload. Negative if allocated, positive if free.
    struct block_header *next;     // Pointer to next block in address order
    struct block_header *prev;     // Pointer to previous block in address order
} block_header_t;

#define ALIGNMENT 16
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1))
#define HEADER_SIZE ALIGN(sizeof(block_header_t))

static block_header_t *free_list = NULL;
static int heap_initialized = 0;

static void my_heap_init(void) {
    block_header_t *initial_block = (block_header_t *)HEAP_START;
    int total_usable_size = HEAP_SIZE - HEADER_SIZE;
    
    initial_block->size = total_usable_size;
    initial_block->next = NULL;
    initial_block->prev = NULL;
    
    free_list = initial_block;
    heap_initialized = 1;
}

static void* my_malloc(int size) {
    if (!heap_initialized) {
        my_heap_init();
    }
    
    if (size <= 0) {
        return NULL;
    }
    
    size = ALIGN(size);
    
    block_header_t *curr = free_list;
    while (curr != NULL) {
        if (curr->size >= size) {
            // Found a block that fits! Check if we can split it.
            int remaining = curr->size - size - HEADER_SIZE;
            if (remaining >= (int)ALIGNMENT) {
                // Split the block
                block_header_t *new_block = (block_header_t *)((unsigned char *)curr + HEADER_SIZE + size);
                new_block->size = remaining;
                new_block->next = curr->next;
                new_block->prev = curr;
                if (curr->next) {
                    curr->next->prev = new_block;
                }
                curr->next = new_block;
                curr->size = -size; // mark current as allocated (negative size)
            } else {
                // Use the entire block as-is
                curr->size = -curr->size; // negate to mark as allocated
            }
            
            return (void *)((unsigned char *)curr + HEADER_SIZE);
        }
        curr = curr->next;
    }
    
    // Out of memory
    return NULL;
}

static void my_free(void* ptr) {
    if (!ptr) {
        return;
    }
    
    block_header_t *block = (block_header_t *)((unsigned char *)ptr - HEADER_SIZE);
    if (block->size >= 0) {
        // Block is already free
        return;
    }
    
    // Mark as free (positive size)
    block->size = -block->size;
    
    // Coalesce with next block if it exists and is free
    if (block->next && block->next->size >= 0) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    // Coalesce with previous block if it exists and is free
    if (block->prev && block->prev->size >= 0) {
        block->prev->size += HEADER_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

/* -------------------------------------------------------------------------
   CUSTOM MEMORY-MAPPED VFS (doom1.wad and mock writes)
   ------------------------------------------------------------------------- */

#define MAX_VIRTUAL_FILES 8

typedef struct {
    char name[64];
    unsigned char *data;
    int size;
    int max_size;
    int offset;
    int in_use;
    int is_open;
    int is_writable;
} virtual_file_t;

static virtual_file_t virtual_files[MAX_VIRTUAL_FILES] = {0};

static int is_doom1_wad(const char *filename) {
    if (!filename) return 0;
    
    // Extract basename
    const char *basename = filename;
    for (const char *p = filename; *p; p++) {
        if (*p == '/' || *p == '\\') {
            basename = p + 1;
        }
    }
    
    // Direct case-insensitive match for "doom1.wad"
    const char *p1 = basename;
    const char *p2 = "doom1.wad";
    while (*p1 && *p2) {
        char c1 = *p1;
        char c2 = *p2;
        if (c1 >= 'A' && c1 <= 'Z') c1 = c1 - 'A' + 'a';
        if (c2 >= 'A' && c2 <= 'Z') c2 = c2 - 'A' + 'a';
        if (c1 != c2) break;
        p1++;
        p2++;
    }
    if (*p1 == '\0' && *p2 == '\0') {
        return 1;
    }
    
    // Check if ends with "doom1.wad"
    int len1 = 0;
    while (basename[len1]) len1++;
    int len2 = 9; // "doom1.wad" length
    if (len1 >= len2) {
        const char *suffix = basename + (len1 - len2);
        p1 = suffix;
        p2 = "doom1.wad";
        int match = 1;
        while (*p1 && *p2) {
            char c1 = *p1;
            char c2 = *p2;
            if (c1 >= 'A' && c1 <= 'Z') c1 = c1 - 'A' + 'a';
            if (c2 >= 'A' && c2 <= 'Z') c2 = c2 - 'A' + 'a';
            if (c1 != c2) { match = 0; break; }
            p1++; p2++;
        }
        if (match) return 1;
    }
    
    return 0;
}

static void* my_doom_open_impl(const char* filename, const char* mode) {
    if (!filename) return NULL;
    
    // Extract basename for local name-matching
    const char *basename = filename;
    for (const char *p = filename; *p; p++) {
        if (*p == '/' || *p == '\\') {
            basename = p + 1;
        }
    }
    
    // Check if this file is already in our virtual VFS list (e.g. mock save or config file)
    for (int i = 0; i < MAX_VIRTUAL_FILES; i++) {
        if (virtual_files[i].in_use) {
            const char *p1 = virtual_files[i].name;
            const char *p2 = basename;
            int match = 1;
            while (*p1 || *p2) {
                char c1 = *p1;
                char c2 = *p2;
                if (c1 >= 'A' && c1 <= 'Z') c1 = c1 - 'A' + 'a';
                if (c2 >= 'A' && c2 <= 'Z') c2 = c2 - 'A' + 'a';
                if (c1 != c2) { match = 0; break; }
                p1++; p2++;
            }
            if (match) {
                // Reopen the file! Reset offset.
                int is_write = 0;
                for (const char *m = mode; *m; m++) {
                    if (*m == 'w' || *m == 'a') {
                        is_write = 1;
                        break;
                    }
                }
                if (is_write) {
                    virtual_files[i].size = 0; // truncate on write
                }
                virtual_files[i].offset = 0;
                virtual_files[i].is_open = 1;
                return &virtual_files[i];
            }
        }
    }
    
    // Find an empty slot
    int slot = -1;
    for (int i = 0; i < MAX_VIRTUAL_FILES; i++) {
        if (!virtual_files[i].in_use) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return NULL;
    
    // Check if loading the preloaded WAD
    if (is_doom1_wad(basename)) {
        int j = 0;
        for (; basename[j] && j < 63; j++) {
            virtual_files[slot].name[j] = basename[j];
        }
        virtual_files[slot].name[j] = '\0';
        // Memory-mapped bounds for preloaded WAD: 0x00200000 to 0x0063FFFF (inclusive)
        virtual_files[slot].data = (unsigned char *)0x00200000;
        virtual_files[slot].size = 0x00440000; // 0x0063FFFF - 0x00200000 + 1 = 4,456,448 bytes (4.25 MB)
        virtual_files[slot].max_size = 0x00440000;
        virtual_files[slot].offset = 0;
        virtual_files[slot].in_use = 1;
        virtual_files[slot].is_open = 1;
        virtual_files[slot].is_writable = 0;
        return &virtual_files[slot];
    }
    
    // For non-WAD files (configurations/savegames), check if opening for write
    int is_write = 0;
    for (const char *m = mode; *m; m++) {
        if (*m == 'w' || *m == 'a') {
            is_write = 1;
            break;
        }
    }
    
    if (is_write) {
        // Create a mock writable file in memory
        static unsigned char mock_buffers[MAX_VIRTUAL_FILES][65536]; // 64KB static write buffer per slot
        
        int j = 0;
        for (; basename[j] && j < 63; j++) {
            virtual_files[slot].name[j] = basename[j];
        }
        virtual_files[slot].name[j] = '\0';
        virtual_files[slot].data = mock_buffers[slot];
        virtual_files[slot].size = 0;
        virtual_files[slot].max_size = 65536;
        virtual_files[slot].offset = 0;
        virtual_files[slot].in_use = 1;
        virtual_files[slot].is_open = 1;
        virtual_files[slot].is_writable = 1;
        return &virtual_files[slot];
    }
    
    // Reads of non-existing files return NULL
    return NULL;
}

static void my_doom_close_impl(void* handle) {
    if (!handle) return;
    virtual_file_t *vf = (virtual_file_t *)handle;
    vf->is_open = 0;
}

static int my_doom_read_impl(void* handle, void *buf, int count) {
    if (!handle || !buf || count <= 0) return 0;
    virtual_file_t *vf = (virtual_file_t *)handle;
    if (vf->offset >= vf->size) return 0;
    
    int available = vf->size - vf->offset;
    int to_read = (count > available) ? available : count;
    
    for (int i = 0; i < to_read; i++) {
        ((unsigned char *)buf)[i] = vf->data[vf->offset + i];
    }
    
    vf->offset += to_read;
    return to_read;
}

static int my_doom_write_impl(void* handle, const void *buf, int count) {
    if (!handle || !buf || count <= 0) return 0;
    virtual_file_t *vf = (virtual_file_t *)handle;
    if (!vf->is_writable) return 0;
    
    int available = vf->max_size - vf->offset;
    if (available <= 0) return 0;
    
    int to_write = (count > available) ? available : count;
    
    for (int i = 0; i < to_write; i++) {
        vf->data[vf->offset + i] = ((const unsigned char *)buf)[i];
    }
    
    vf->offset += to_write;
    if (vf->offset > vf->size) {
        vf->size = vf->offset;
    }
    return to_write;
}

static int my_doom_seek_impl(void* handle, int offset, doom_seek_t origin) {
    if (!handle) return -1;
    virtual_file_t *vf = (virtual_file_t *)handle;
    
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
    
    if (new_offset < 0) new_offset = 0;
    if (new_offset > vf->size) new_offset = vf->size;
    
    vf->offset = new_offset;
    return 0;
}

static int my_doom_tell_impl(void* handle) {
    if (!handle) return -1;
    virtual_file_t *vf = (virtual_file_t *)handle;
    return vf->offset;
}

static int my_doom_eof_impl(void* handle) {
    if (!handle) return 1;
    virtual_file_t *vf = (virtual_file_t *)handle;
    return (vf->offset >= vf->size);
}

/* -------------------------------------------------------------------------
   MOCK SYSTEM CALLBACKS FOR BARE-METAL COMPATIBILITY
   ------------------------------------------------------------------------- */

static void my_print(const char* str) {
    // In a bare-metal kernel, this writes to a serial port or diagnostic log.
    // For standalone compiling, we keep it as a no-op or volatile log target.
    (void)str;
}

static void my_gettime(int* sec, int* usec) {
    // Increments a tick timer representing 35 ticks per second (Doom internal rate)
    static int dummy_sec = 0;
    static int dummy_usec = 0;
    
    dummy_usec += 28571; // 1,000,000 / 35
    if (dummy_usec >= 1000000) {
        dummy_sec += dummy_usec / 1000000;
        dummy_usec %= 1000000;
    }
    if (sec) *sec = dummy_sec;
    if (usec) *usec = dummy_usec;
}

static void my_exit(int code) {
    (void)code;
    // Bare-metal halt loop
    while (1) {
        __asm__ __volatile__("hlt");
    }
}

static char* my_getenv(const char* var) {
    (void)var;
    return NULL; // No system environment variables on bare metal
}

/* -------------------------------------------------------------------------
   KEYBOARD TRANSLATION SCANNER
   ------------------------------------------------------------------------- */

static doom_key_t map_scan_code_to_doom_key(int code) {
    // 1. Map PS/2 Scan Codes (Set 1) commonly used in bare-metal keyboard drivers
    switch (code) {
        case 0x01: return DOOM_KEY_ESCAPE;
        case 0x1C: return DOOM_KEY_ENTER;
        case 0x39: return DOOM_KEY_SPACE;
        case 0x0F: return DOOM_KEY_TAB;
        case 0x0E: return DOOM_KEY_BACKSPACE;
        
        // Arrows
        case 0x48: return DOOM_KEY_UP_ARROW;
        case 0x50: return DOOM_KEY_DOWN_ARROW;
        case 0x4B: return DOOM_KEY_LEFT_ARROW;
        case 0x4D: return DOOM_KEY_RIGHT_ARROW;
        
        // Modifiers
        case 0x1D: return DOOM_KEY_CTRL;
        case 0x2A: case 0x36: return DOOM_KEY_SHIFT;
        case 0x38: return DOOM_KEY_ALT;
        
        // Letters (PS/2 Set 1 scancodes mapping to lowercase letters)
        case 0x1E: return DOOM_KEY_A;
        case 0x30: return DOOM_KEY_B;
        case 0x2E: return DOOM_KEY_C;
        case 0x20: return DOOM_KEY_D;
        case 0x12: return DOOM_KEY_E;
        case 0x21: return DOOM_KEY_F;
        case 0x22: return DOOM_KEY_G;
        case 0x23: return DOOM_KEY_H;
        case 0x17: return DOOM_KEY_I;
        case 0x24: return DOOM_KEY_J;
        case 0x25: return DOOM_KEY_K;
        case 0x26: return DOOM_KEY_L;
        case 0x32: return DOOM_KEY_M;
        case 0x31: return DOOM_KEY_N;
        case 0x18: return DOOM_KEY_O;
        case 0x19: return DOOM_KEY_P;
        case 0x10: return DOOM_KEY_Q;
        case 0x13: return DOOM_KEY_R;
        case 0x1F: return DOOM_KEY_S;
        case 0x14: return DOOM_KEY_T;
        case 0x16: return DOOM_KEY_U;
        case 0x2F: return DOOM_KEY_V;
        case 0x11: return DOOM_KEY_W;
        case 0x2D: return DOOM_KEY_X;
        case 0x15: return DOOM_KEY_Y;
        case 0x2C: return DOOM_KEY_Z;
        
        // Digits
        case 0x0B: return DOOM_KEY_0;
        case 0x02: return DOOM_KEY_1;
        case 0x03: return DOOM_KEY_2;
        case 0x04: return DOOM_KEY_3;
        case 0x05: return DOOM_KEY_4;
        case 0x06: return DOOM_KEY_5;
        case 0x07: return DOOM_KEY_6;
        case 0x08: return DOOM_KEY_7;
        case 0x09: return DOOM_KEY_8;
        case 0x0A: return DOOM_KEY_9;
    }
    
    // 2. Fallback to standard ASCII / char values
    if (code >= 'a' && code <= 'z') return (doom_key_t)code;
    if (code >= 'A' && code <= 'Z') return (doom_key_t)(code - 'A' + 'a');
    if (code >= '0' && code <= '9') return (doom_key_t)code;
    
    switch (code) {
        case 27:  return DOOM_KEY_ESCAPE;
        case 13:  return DOOM_KEY_ENTER;
        case 32:  return DOOM_KEY_SPACE;
        case 9:   return DOOM_KEY_TAB;
        case 8:   return DOOM_KEY_BACKSPACE;
        case 127: return DOOM_KEY_BACKSPACE;
    }
    
    return DOOM_KEY_UNKNOWN;
}

static unsigned char last_key_states[256] = {0};

/* -------------------------------------------------------------------------
   PUBLIC API PORTING HOOKS
   ------------------------------------------------------------------------- */

void doom_port_init(void) {
    // 1. Set custom malloc and free bounds
    doom_set_malloc(my_malloc, my_free);
    
    // 2. Bind memory-mapped file I/O VFS
    doom_set_file_io(
        my_doom_open_impl,
        my_doom_close_impl,
        my_doom_read_impl,
        my_doom_write_impl,
        my_doom_seek_impl,
        my_doom_tell_impl,
        my_doom_eof_impl
    );
    
    // 3. Set standard bare-metal callbacks
    doom_set_print(my_print);
    doom_set_gettime(my_gettime);
    doom_set_exit(my_exit);
    doom_set_getenv(my_getenv);
    
    // 4. Force standard 320x200 software renderer resolution
    doom_set_resolution(320, 200);
    
    // 5. Initialize the PureDOOM engine with a default command argument
    char *args[] = {"doom", NULL};
    doom_init(1, args, 0);
}

void doom_port_update(void) {
    // 1. Scan key_states array for press / release transitions and report to PureDOOM
    for (int i = 0; i < 256; i++) {
        unsigned char pressed = key_states[i];
        if (pressed != last_key_states[i]) {
            last_key_states[i] = pressed;
            doom_key_t doom_key = map_scan_code_to_doom_key(i);
            if (doom_key != DOOM_KEY_UNKNOWN) {
                if (pressed) {
                    doom_key_down(doom_key);
                } else {
                    doom_key_up(doom_key);
                }
            }
        }
    }
    
    // 2. Drive the Doom game simulation state machine (updates sound, events, logic at 35 FPS)
    doom_update();
}

void doom_draw_screen(void) {
    // Acquire the indexed (palette mode) 8-bit framebuffer pointer from PureDOOM software renderer.
    // 1 channel = 8-bit index color per pixel (320 * 200 = 64,000 bytes)
    const unsigned char *doom_fb = doom_get_framebuffer(1);
    
    if (doom_fb && secondary_vga_buffer) {
        // Copy the screen index values directly to our secondary VGA double buffer
        for (int i = 0; i < 320 * 200; i++) {
            secondary_vga_buffer[i] = doom_fb[i];
        }
    }
}
