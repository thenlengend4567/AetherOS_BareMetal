#ifndef FREESTANDING_H
#define FREESTANDING_H

// Prevent standard type conflicts
#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
typedef unsigned int size_t;
#endif

#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
typedef signed int ssize_t;
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef unsigned int uintptr_t;
typedef signed int intptr_t;

// Memory operations
void* memcpy(void* dest, const void* src, size_t n);
void* memset(void* s, int c, size_t n);
void* memmove(void* dest, const void* src, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);

// String operations
size_t strlen(const char* s);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t n);
char* strchr(const char* s, int c);
char* strrchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);

// Extra conversion utility functions
char* itoa(int value, char* str, int base);
char* utoa(unsigned int value, char* str, int base);
int atoi(const char* str);

// Bump Allocator Configuration & API
#define BUMP_ALLOCATOR_START 0x01000000

typedef struct {
    size_t size;
    int is_free;
} alloc_header_t;

void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t size);
void free(void* ptr);

size_t get_allocated_bytes(void);
size_t get_free_bytes(void);
void bump_allocator_init(size_t limit_bytes);

// Context Recovery System
typedef struct {
    // Segment registers
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    // General purpose registers (pushad / popad format)
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t oesp; // ignored by popad
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    // Interrupt/Far jump return frame
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
} __attribute__((packed)) context_t;

void init_context_recovery(void);
void register_recovery_context(const context_t* ctx);
void trigger_context_recovery(void);

#endif // FREESTANDING_H
