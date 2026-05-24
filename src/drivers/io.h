#ifndef IO_H
#define IO_H

#include <stdint.h>

/**
 * @brief Read a byte from an I/O port.
 */
static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * @brief Write a byte to an I/O port.
 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Read a word (16-bit) from an I/O port.
 */
static inline uint16_t inw(uint16_t port) {
    uint16_t value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/**
 * @brief Write a word (16-bit) to an I/O port.
 */
static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Read multiple words from an I/O port into memory (rep insw).
 */
static inline void insw(uint16_t port, void *addr, uint32_t count) {
    __asm__ volatile (
        "cld\n\t"
        "rep insw"
        : "+D"(addr), "+c"(count)
        : "d"(port)
        : "memory"
    );
}

/**
 * @brief Wait a very small amount of time (approx 1 to 4 microseconds).
 * Writes to port 0x80 which is usually unused.
 */
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif // IO_H
