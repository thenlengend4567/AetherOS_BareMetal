#include "idt.h"

// The 256-entry Interrupt Descriptor Table
idt_gate_t idt[256];
idt_register_t idt_reg;

// Exception messages mapping to exceptions 0-31
static const char* exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};

// Declaring external assembly stubs
extern void isr_stub_0(void);
extern void isr_stub_1(void);
extern void isr_stub_2(void);
extern void isr_stub_3(void);
extern void isr_stub_4(void);
extern void isr_stub_5(void);
extern void isr_stub_6(void);
extern void isr_stub_7(void);
extern void isr_stub_8(void);
extern void isr_stub_9(void);
extern void isr_stub_10(void);
extern void isr_stub_11(void);
extern void isr_stub_12(void);
extern void isr_stub_13(void);
extern void isr_stub_14(void);
extern void isr_stub_15(void);
extern void isr_stub_16(void);
extern void isr_stub_17(void);
extern void isr_stub_18(void);
extern void isr_stub_19(void);
extern void isr_stub_20(void);
extern void isr_stub_21(void);
extern void isr_stub_22(void);
extern void isr_stub_23(void);
extern void isr_stub_24(void);
extern void isr_stub_25(void);
extern void isr_stub_26(void);
extern void isr_stub_27(void);
extern void isr_stub_28(void);
extern void isr_stub_29(void);
extern void isr_stub_30(void);
extern void isr_stub_31(void);

extern void isr_stub_32(void);
extern void isr_stub_33(void);
extern void isr_stub_34(void);
extern void isr_stub_35(void);
extern void isr_stub_36(void);
extern void isr_stub_37(void);
extern void isr_stub_38(void);
extern void isr_stub_39(void);
extern void isr_stub_40(void);
extern void isr_stub_41(void);
extern void isr_stub_42(void);
extern void isr_stub_43(void);
extern void isr_stub_44(void);
extern void isr_stub_45(void);
extern void isr_stub_46(void);
extern void isr_stub_47(void);

// Array of stub function pointers for ease of iteration
static void* isr_stubs[48] = {
    isr_stub_0,  isr_stub_1,  isr_stub_2,  isr_stub_3,
    isr_stub_4,  isr_stub_5,  isr_stub_6,  isr_stub_7,
    isr_stub_8,  isr_stub_9,  isr_stub_10, isr_stub_11,
    isr_stub_12, isr_stub_13, isr_stub_14, isr_stub_15,
    isr_stub_16, isr_stub_17, isr_stub_18, isr_stub_19,
    isr_stub_20, isr_stub_21, isr_stub_22, isr_stub_23,
    isr_stub_24, isr_stub_25, isr_stub_26, isr_stub_27,
    isr_stub_28, isr_stub_29, isr_stub_30, isr_stub_31,
    isr_stub_32, isr_stub_33, isr_stub_34, isr_stub_35,
    isr_stub_36, isr_stub_37, isr_stub_38, isr_stub_39,
    isr_stub_40, isr_stub_41, isr_stub_42, isr_stub_43,
    isr_stub_44, isr_stub_45, isr_stub_46, isr_stub_47
};

// Array of registered C handlers for IRQs 0-15
typedef void (*irq_handler_t)(registers_t *regs);
static irq_handler_t irq_handlers[16] = {0};

// Local helper functions for VGA output (avoiding standard library dependency)
static size_t local_strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void unsigned_to_dec_str(uint32_t value, char* str) {
    char temp[16];
    int i = 0;
    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    while (value > 0) {
        temp[i++] = (value % 10) + '0';
        value /= 10;
    }
    int j = 0;
    while (i > 0) {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}

static void hex_to_str(uint32_t val, char* buf) {
    const char* hex_chars = "0123456789ABCDEF";
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; i++) {
        buf[9 - i] = hex_chars[(val >> (i * 4)) & 0x0F];
    }
    buf[10] = '\0';
}

static volatile uint16_t* const vga_buffer = (volatile uint16_t*)0xB8000;

static void vga_clear(uint8_t color) {
    uint16_t attribute = color << 8;
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = ' ' | attribute;
    }
}

static void vga_print_string(const char* str, int row, int col, uint8_t color) {
    uint16_t attribute = color << 8;
    int index = row * 80 + col;
    while (*str) {
        if (*str == '\n') {
            row++;
            col = 0;
            index = row * 80 + col;
        } else {
            vga_buffer[index++] = *str | attribute;
        }
        str++;
        if (index >= 80 * 25) break;
    }
}

// 8259 PIC helper functions
void outb(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void io_wait(void) {
    // Port 0x80 is commonly used for dummy writes to introduce an IO delay
    outb(0x80, 0);
}

// Remap the 8259 PIC
void pic_remap(void) {
    uint8_t a1, a2;

    // Save current masks
    a1 = inb(0x21);
    a2 = inb(0xA1);

    // ICW1: Start initialization sequence in cascade mode
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();

    // ICW2: Vector offset mapping
    outb(0x21, 0x20); // Master PIC: map IRQ 0-7 to 0x20-0x27
    io_wait();
    outb(0xA1, 0x28); // Slave PIC: map IRQ 8-15 to 0x28-0x2F
    io_wait();

    // ICW3: Tell Master PIC there is a slave PIC at IRQ2 (0000 0100)
    outb(0x21, 0x04);
    io_wait();
    // ICW3: Tell Slave PIC its cascade identity (0000 0010)
    outb(0xA1, 0x02);
    io_wait();

    // ICW4: Set 8086 mode
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    // Unmask all hardware interrupts on PICs
    outb(0x21, 0x00);
    outb(0xA1, 0x00);
}

// Register a single IDT gate entry
void idt_set_gate(uint8_t num, uint32_t base_addr, uint16_t sel, uint8_t flags) {
    idt[num].offset_low = (base_addr & 0xFFFF);
    idt[num].selector = sel;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
    idt[num].offset_high = (base_addr >> 16) & 0xFFFF;
}

// Install IDT handlers
void isr_install(void) {
    // Map Exception stubs (0-31) and IRQ stubs (32-47) into the IDT table
    for (int i = 0; i < 48; i++) {
        idt_set_gate(i, (uint32_t)isr_stubs[i], 0x08, 0x8E);
    }

    // Set remaining IDT gates to 0 (ignored/unhandled)
    for (int i = 48; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // Load IDT into CPU using 'lidt' instruction via inline assembly
    __asm__ __volatile__("lidt %0" : : "m"(idt_reg));
}

// Initialize PIC and IDT
void idt_init(void) {
    idt_reg.limit = (sizeof(idt_gate_t) * 256) - 1;
    idt_reg.base = (uint32_t)&idt;

    // Remap PIC to 0x20-0x2F
    pic_remap();

    // Populate IDT and load it
    isr_install();
}

// Register a driver-level handler for a specific IRQ (0-15)
void irq_register_handler(uint8_t irq_num, irq_handler_t handler) {
    if (irq_num < 16) {
        irq_handlers[irq_num] = handler;
    }
}

// C exception dispatcher
void isr_handler(registers_t *regs) {
    // Clear screen to bright red with white text (0x4F)
    vga_clear(0x4F);

    vga_print_string("================================================================================", 0, 0, 0x4F);
    vga_print_string("                                KERNEL PANIC!                                   ", 1, 0, 0x4F);
    vga_print_string("================================================================================", 2, 0, 0x4F);

    char temp[32];
    vga_print_string(" Exception: ", 4, 2, 0x4F);
    if (regs->int_no < 32) {
        const char* msg = exception_messages[regs->int_no];
        vga_print_string(msg, 4, 14, 0x4F);
        vga_print_string(" (", 4, 14 + local_strlen(msg), 0x4F);
        unsigned_to_dec_str(regs->int_no, temp);
        vga_print_string(temp, 4, 16 + local_strlen(msg), 0x4F);
        vga_print_string(")", 4, 16 + local_strlen(msg) + local_strlen(temp), 0x4F);
    } else {
        vga_print_string("Unknown Exception (", 4, 14, 0x4F);
        unsigned_to_dec_str(regs->int_no, temp);
        vga_print_string(temp, 4, 33, 0x4F);
        vga_print_string(")", 4, 33 + local_strlen(temp), 0x4F);
    }

    vga_print_string(" Error Code: ", 5, 2, 0x4F);
    hex_to_str(regs->err_code, temp);
    vga_print_string(temp, 5, 15, 0x4F);

    vga_print_string(" REGISTER DUMP:", 7, 2, 0x4F);

    // Row 9: EAX, EBX, ECX, EDX
    vga_print_string("  EAX: ", 9, 2, 0x4F);
    hex_to_str(regs->eax, temp);
    vga_print_string(temp, 9, 9, 0x4F);

    vga_print_string("EBX: ", 9, 22, 0x4F);
    hex_to_str(regs->ebx, temp);
    vga_print_string(temp, 9, 27, 0x4F);

    vga_print_string("ECX: ", 9, 40, 0x4F);
    hex_to_str(regs->ecx, temp);
    vga_print_string(temp, 9, 45, 0x4F);

    vga_print_string("EDX: ", 9, 58, 0x4F);
    hex_to_str(regs->edx, temp);
    vga_print_string(temp, 9, 63, 0x4F);

    // Row 10: ESI, EDI, EBP, ESP
    vga_print_string("  ESI: ", 10, 2, 0x4F);
    hex_to_str(regs->esi, temp);
    vga_print_string(temp, 10, 9, 0x4F);

    vga_print_string("EDI: ", 10, 22, 0x4F);
    hex_to_str(regs->edi, temp);
    vga_print_string(temp, 10, 27, 0x4F);

    vga_print_string("EBP: ", 10, 40, 0x4F);
    hex_to_str(regs->ebp, temp);
    vga_print_string(temp, 10, 45, 0x4F);

    vga_print_string("ESP: ", 10, 58, 0x4F);
    hex_to_str(regs->esp, temp);
    vga_print_string(temp, 10, 63, 0x4F);

    // Row 11: EIP, CS, EFLAGS
    vga_print_string("  EIP: ", 11, 2, 0x4F);
    hex_to_str(regs->eip, temp);
    vga_print_string(temp, 11, 9, 0x4F);

    vga_print_string("CS:  ", 11, 22, 0x4F);
    hex_to_str(regs->cs, temp);
    vga_print_string(temp, 11, 27, 0x4F);

    vga_print_string("EFL: ", 11, 40, 0x4F);
    hex_to_str(regs->eflags, temp);
    vga_print_string(temp, 11, 45, 0x4F);

    // For Page Fault exception (14), read and print CR2 register
    if (regs->int_no == 14) {
        uint32_t cr2;
        __asm__ __volatile__("mov %%cr2, %0" : "=r"(cr2));
        vga_print_string("  CR2: ", 13, 2, 0x4F);
        hex_to_str(cr2, temp);
        vga_print_string(temp, 13, 9, 0x4F);
        vga_print_string(" (Faulting Address / Page Fault)", 13, 21, 0x4F);
    }

    vga_print_string("================================================================================", 15, 0, 0x4F);
    vga_print_string("                                SYSTEM HALTED.                                  ", 16, 0, 0x4F);
    vga_print_string("================================================================================", 17, 0, 0x4F);

    // Halt CPU completely
    while (1) {
        __asm__ __volatile__("cli; hlt");
    }
}

// C hardware interrupt dispatcher
void irq_handler(registers_t *regs) {
    uint8_t irq_num = regs->int_no - 32;
    if (irq_num < 16 && irq_handlers[irq_num] != NULL) {
        irq_handlers[irq_num](regs);
    }
}
