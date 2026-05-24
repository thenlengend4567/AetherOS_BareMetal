#include "freestanding.h"

// ============================================================================
// Memory Operations
// ============================================================================

void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    if (d < s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else if (d > s) {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] < p2[i]) return -1;
        if (p1[i] > p2[i]) return 1;
    }
    return 0;
}

// ============================================================================
// String Operations
// ============================================================================

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len] != '\0') {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    size_t i = 0;
    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
    return dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    size_t i = 0;
    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] < s2[i]) return -1;
        if (s1[i] > s2[i]) return 1;
        i++;
    }
    if (s1[i] == '\0' && s2[i] == '\0') return 0;
    return (s1[i] == '\0') ? -1 : 1;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0') {
            if ((unsigned char)s1[i] < (unsigned char)s2[i]) return -1;
            if ((unsigned char)s1[i] > (unsigned char)s2[i]) return 1;
            return 0;
        }
    }
    return 0;
}

char* strcat(char* dest, const char* src) {
    size_t dest_len = strlen(dest);
    size_t i = 0;
    while (src[i] != '\0') {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
    return dest;
}

char* strncat(char* dest, const char* src, size_t n) {
    size_t dest_len = strlen(dest);
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++) {
        dest[dest_len + i] = src[i];
    }
    dest[dest_len + i] = '\0';
    return dest;
}

char* strchr(const char* s, int c) {
    while (*s != '\0') {
        if (*s == (char)c) {
            return (char*)s;
        }
        s++;
    }
    if ((char)c == '\0') {
        return (char*)s;
    }
    return NULL;
}

char* strrchr(const char* s, int c) {
    char* last = NULL;
    while (*s != '\0') {
        if (*s == (char)c) {
            last = (char*)s;
        }
        s++;
    }
    if ((char)c == '\0') {
        return (char*)s;
    }
    return last;
}

char* strstr(const char* haystack, const char* needle) {
    if (*needle == '\0') {
        return (char*)haystack;
    }
    while (*haystack != '\0') {
        const char* h = haystack;
        const char* n = needle;
        while (*h != '\0' && *n != '\0' && *h == *n) {
            h++;
            n++;
        }
        if (*n == '\0') {
            return (char*)haystack;
        }
        haystack++;
    }
    return NULL;
}

// ============================================================================
// Data Conversion Utilities
// ============================================================================

char* itoa(int value, char* str, int base) {
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    char* rc = str;
    char* ptr = str;
    char* low;
    int is_negative = 0;
    unsigned int uval = value;

    if (value < 0 && base == 10) {
        is_negative = 1;
        uval = -value;
    }

    do {
        unsigned int digit = uval % base;
        *ptr++ = (digit < 10) ? (char)('0' + digit) : (char)('a' + digit - 10);
        uval /= base;
    } while (uval);

    if (is_negative) {
        *ptr++ = '-';
    }
    *ptr = '\0';

    low = rc;
    ptr--;
    while (low < ptr) {
        char temp = *low;
        *low++ = *ptr;
        *ptr-- = temp;
    }
    return rc;
}

char* utoa(unsigned int value, char* str, int base) {
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    char* rc = str;
    char* ptr = str;
    char* low;

    do {
        unsigned int digit = value % base;
        *ptr++ = (digit < 10) ? (char)('0' + digit) : (char)('a' + digit - 10);
        value /= base;
    } while (value);
    *ptr = '\0';

    low = rc;
    ptr--;
    while (low < ptr) {
        char temp = *low;
        *low++ = *ptr;
        *ptr-- = temp;
    }
    return rc;
}

int atoi(const char* str) {
    int res = 0;
    int sign = 1;
    int i = 0;

    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r' || str[i] == '\v' || str[i] == '\f') {
        i++;
    }

    if (str[i] == '-') {
        sign = -1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    while (str[i] >= '0' && str[i] <= '9') {
        res = res * 10 + (str[i] - '0');
        i++;
    }
    return sign * res;
}

// ============================================================================
// Bump Allocator Implementation (Physical Base: 0x01000000)
// ============================================================================

static uintptr_t current_bump = BUMP_ALLOCATOR_START;
static size_t heap_limit = 128 * 1024 * 1024; // Default to 128 MB heap size
static size_t allocated_bytes = 0;

void* malloc(size_t size) {
    if (size == 0) return NULL;

    // Align size to 8 bytes to avoid misaligned memory access
    size_t aligned_size = (size + 7) & ~7;
    size_t total_size = sizeof(alloc_header_t) + aligned_size;

    if (current_bump + total_size > BUMP_ALLOCATOR_START + heap_limit) {
        // Out of memory
        return NULL;
    }

    alloc_header_t* header = (alloc_header_t*)current_bump;
    header->size = aligned_size;
    header->is_free = 0;

    void* ptr = (void*)(current_bump + sizeof(alloc_header_t));
    current_bump += total_size;
    allocated_bytes += aligned_size;

    return ptr;
}

void* calloc(size_t num, size_t size) {
    size_t total = num * size;
    void* ptr = malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) {
        return malloc(size);
    }
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    alloc_header_t* header = (alloc_header_t*)((uintptr_t)ptr - sizeof(alloc_header_t));

    // If existing block is already large enough, reuse it
    if (header->size >= size) {
        return ptr;
    }

    // Optimize in-place expansion if this is the most recent block at the end of the heap.
    // This is crucial for loops calling realloc repeatedly (like lumpinfo allocation).
    size_t aligned_size = (size + 7) & ~7;
    uintptr_t block_end = (uintptr_t)ptr + header->size;
    if (block_end == current_bump) {
        size_t diff = aligned_size - header->size;
        if (current_bump + diff <= BUMP_ALLOCATOR_START + heap_limit) {
            current_bump += diff;
            allocated_bytes += diff;
            header->size = aligned_size;
            return ptr;
        }
    }

    // Allocate a new block
    void* new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL; // Out of memory
    }

    // Copy old content
    memcpy(new_ptr, ptr, header->size);

    // Free the old block
    free(ptr);

    return new_ptr;
}

void free(void* ptr) {
    if (!ptr) return;

    alloc_header_t* header = (alloc_header_t*)((uintptr_t)ptr - sizeof(alloc_header_t));
    header->is_free = 1;

    if (allocated_bytes >= header->size) {
        allocated_bytes -= header->size;
    } else {
        allocated_bytes = 0;
    }

    // Stack-like recovery optimization for consecutive malloc/free patterns
    uintptr_t block_end = (uintptr_t)ptr + header->size;
    if (block_end == current_bump) {
        current_bump = (uintptr_t)header;
    }
}

size_t get_allocated_bytes(void) {
    return allocated_bytes;
}

size_t get_free_bytes(void) {
    uintptr_t total_allocated = current_bump - BUMP_ALLOCATOR_START;
    if (total_allocated >= heap_limit) {
        return 0;
    }
    return heap_limit - total_allocated;
}

void bump_allocator_init(size_t limit_bytes) {
    current_bump = BUMP_ALLOCATOR_START;
    heap_limit = limit_bytes;
    allocated_bytes = 0;
}

// ============================================================================
// Context Recovery System (Physical Base: 0x0009F000)
// ============================================================================

void init_context_recovery(void) {
    // Trampoline byte representation:
    // 0xFA                            -> cli
    // 0xBC 0x50 0xF0 0x09 0x00        -> mov esp, 0x0009F050
    // 0x0F 0xA1                       -> pop gs
    // 0x0F 0xA9                       -> pop fs
    // 0x07                            -> pop es
    // 0x1F                            -> pop ds
    // 0x61                            -> popad
    // 0xCF                            -> iret
    const uint8_t trampoline_code[] = {
        0xFA,
        0xBC, 0x50, 0xF0, 0x09, 0x00,
        0x0F, 0xA1,
        0x0F, 0xA9,
        0x07,
        0x1F,
        0x61,
        0xCF
    };

    // Copy trampoline code to 0x0009F000
    memcpy((void*)0x0009F000, trampoline_code, sizeof(trampoline_code));

    // Zero out the saved context frame at 0x0009F050 initially
    memset((void*)0x0009F050, 0, sizeof(context_t));
}

void register_recovery_context(const context_t* ctx) {
    context_t* dest = (context_t*)0x0009F050;
    memcpy(dest, ctx, sizeof(context_t));
}

void trigger_context_recovery(void) {
    void (*recovery_entry)(void) = (void (*)(void))0x0009F000;
    recovery_entry();
}

// ============================================================================
// GCC compiler helper functions for 64-bit unsigned division and modulo
// ============================================================================

uint64_t __udivmoddi4(uint64_t num, uint64_t den, uint64_t *rem) {
    if (den == 0) {
        if (rem) *rem = 0;
        return 0;
    }

    uint64_t quot = 0;
    uint64_t temp_den = den;
    uint64_t qbit = 1;

    while ((temp_den < num) && ((temp_den & (1ULL << 63)) == 0)) {
        temp_den <<= 1;
        qbit <<= 1;
    }

    while (qbit > 0) {
        if (num >= temp_den) {
            num -= temp_den;
            quot |= qbit;
        }
        temp_den >>= 1;
        qbit >>= 1;
    }

    if (rem) {
        *rem = num;
    }
    return quot;
}

uint64_t __udivdi3(uint64_t num, uint64_t den) {
    return __udivmoddi4(num, den, NULL);
}

uint64_t __umoddi3(uint64_t num, uint64_t den) {
    uint64_t rem;
    __udivmoddi4(num, den, &rem);
    return rem;
}

