#ifndef IDT_H
#define IDT_H

#include "../libc/freestanding.h"

// IDT Gate Entry struct
typedef struct {
    uint16_t offset_low;   // The lower 16 bits of the address to jump to
    uint16_t selector;     // Kernel segment selector (usually 0x08 for code)
    uint8_t  zero;         // This byte must always be zero
    uint8_t  type_attr;    // Gate type and attributes (e.g. 0x8E for 32-bit interrupt gate)
    uint16_t offset_high;  // The upper 16 bits of the address to jump to
} __attribute__((packed)) idt_gate_t;

// IDT Pointer struct (for lidt instruction)
typedef struct {
    uint16_t limit;        // Size of IDT - 1
    uint32_t base;         // The address of the first element in our idt_gate_t array
} __attribute__((packed)) idt_register_t;

// Register state struct passed to C interrupt handlers
typedef struct {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by 'pusha'
    uint32_t int_no;                                 // Interrupt number
    uint32_t err_code;                               // Error code (if pushed by CPU, or dummy 0)
    uint32_t eip, cs, eflags;                        // Pushed by CPU automatically
} registers_t;

// Function declarations
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base_addr, uint16_t sel, uint8_t flags);
void pic_remap(void);
void isr_install(void);

// C interrupt handlers called from assembly stubs
void isr_handler(registers_t *regs);
void irq_handler(registers_t *regs);

typedef void (*irq_handler_t)(registers_t *regs);
void irq_register_handler(uint8_t irq_num, irq_handler_t handler);

// Inline port IO helper functions
void outb(uint16_t port, uint8_t value);
uint8_t inb(uint16_t port);

#endif // IDT_H
