[bits 32]
section .text


; External C interrupt handlers
extern _isr_handler
extern _irq_handler

; Export all ISR stubs so they are accessible to idt.c
global _isr_stub_0
global _isr_stub_1
global _isr_stub_2
global _isr_stub_3
global _isr_stub_4
global _isr_stub_5
global _isr_stub_6
global _isr_stub_7
global _isr_stub_8
global _isr_stub_9
global _isr_stub_10
global _isr_stub_11
global _isr_stub_12
global _isr_stub_13
global _isr_stub_14
global _isr_stub_15
global _isr_stub_16
global _isr_stub_17
global _isr_stub_18
global _isr_stub_19
global _isr_stub_20
global _isr_stub_21
global _isr_stub_22
global _isr_stub_23
global _isr_stub_24
global _isr_stub_25
global _isr_stub_26
global _isr_stub_27
global _isr_stub_28
global _isr_stub_29
global _isr_stub_30
global _isr_stub_31

global _isr_stub_32
global _isr_stub_33
global _isr_stub_34
global _isr_stub_35
global _isr_stub_36
global _isr_stub_37
global _isr_stub_38
global _isr_stub_39
global _isr_stub_40
global _isr_stub_41
global _isr_stub_42
global _isr_stub_43
global _isr_stub_44
global _isr_stub_45
global _isr_stub_46
global _isr_stub_47

; =====================================================================
; EXCEPTION ISR STUBS (0-31)
; Some exceptions push error codes automatically, others require dummy 0s
; =====================================================================

; 0: Divide-by-zero Error
_isr_stub_0:
    push 0          ; Dummy error code
    push 0          ; Interrupt number
    jmp isr_common_stub

; 1: Debug
_isr_stub_1:
    push 0
    push 1
    jmp isr_common_stub

; 2: Non-maskable Interrupt
_isr_stub_2:
    push 0
    push 2
    jmp isr_common_stub

; 3: Breakpoint
_isr_stub_3:
    push 0
    push 3
    jmp isr_common_stub

; 4: Overflow
_isr_stub_4:
    push 0
    push 4
    jmp isr_common_stub

; 5: Bound Range Exceeded
_isr_stub_5:
    push 0
    push 5
    jmp isr_common_stub

; 6: Invalid Opcode
_isr_stub_6:
    push 0
    push 6
    jmp isr_common_stub

; 7: Device Not Available
_isr_stub_7:
    push 0
    push 7
    jmp isr_common_stub

; 8: Double Fault (pushes error code)
_isr_stub_8:
    push 8
    jmp isr_common_stub

; 9: Coprocessor Segment Overrun
_isr_stub_9:
    push 0
    push 9
    jmp isr_common_stub

; 10: Invalid TSS (pushes error code)
_isr_stub_10:
    push 10
    jmp isr_common_stub

; 11: Segment Not Present (pushes error code)
_isr_stub_11:
    push 11
    jmp isr_common_stub

; 12: Stack-Segment Fault (pushes error code)
_isr_stub_12:
    push 12
    jmp isr_common_stub

; 13: General Protection Fault (pushes error code)
_isr_stub_13:
    push 13
    jmp isr_common_stub

; 14: Page Fault (pushes error code)
_isr_stub_14:
    push 14
    jmp isr_common_stub

; 15: Reserved
_isr_stub_15:
    push 0
    push 15
    jmp isr_common_stub

; 16: x87 Floating-Point Exception
_isr_stub_16:
    push 0
    push 16
    jmp isr_common_stub

; 17: Alignment Check (pushes error code)
_isr_stub_17:
    push 17
    jmp isr_common_stub

; 18: Machine Check
_isr_stub_18:
    push 0
    push 18
    jmp isr_common_stub

; 19: SIMD Floating-Point Exception
_isr_stub_19:
    push 0
    push 19
    jmp isr_common_stub

; 20: Virtualization Exception
_isr_stub_20:
    push 0
    push 20
    jmp isr_common_stub

; 21: Control Protection Exception (pushes error code)
_isr_stub_21:
    push 21
    jmp isr_common_stub

; 22: Reserved
_isr_stub_22:
    push 0
    push 22
    jmp isr_common_stub

; 23: Reserved
_isr_stub_23:
    push 0
    push 23
    jmp isr_common_stub

; 24: Reserved
_isr_stub_24:
    push 0
    push 24
    jmp isr_common_stub

; 25: Reserved
_isr_stub_25:
    push 0
    push 25
    jmp isr_common_stub

; 26: Reserved
_isr_stub_26:
    push 0
    push 26
    jmp isr_common_stub

; 27: Reserved
_isr_stub_27:
    push 0
    push 27
    jmp isr_common_stub

; 28: Reserved
_isr_stub_28:
    push 0
    push 28
    jmp isr_common_stub

; 29: VMM Communication Exception (pushes error code)
_isr_stub_29:
    push 29
    jmp isr_common_stub

; 30: Security Exception (pushes error code)
_isr_stub_30:
    push 30
    jmp isr_common_stub

; 31: Reserved
_isr_stub_31:
    push 0
    push 31
    jmp isr_common_stub

; =====================================================================
; HARDWARE IRQ STUBS (0-15 mapped to vectors 32-47)
; Pushes dummy error code 0 and interrupt number
; =====================================================================

; IRQ 0: System Timer
_isr_stub_32:
    push 0
    push 32
    jmp irq_common_stub

; IRQ 1: Keyboard
_isr_stub_33:
    push 0
    push 33
    jmp irq_common_stub

; IRQ 2: Cascade (used internally)
_isr_stub_34:
    push 0
    push 34
    jmp irq_common_stub

; IRQ 3: COM2
_isr_stub_35:
    push 0
    push 35
    jmp irq_common_stub

; IRQ 4: COM1
_isr_stub_36:
    push 0
    push 36
    jmp irq_common_stub

; IRQ 5: LPT2
_isr_stub_37:
    push 0
    push 37
    jmp irq_common_stub

; IRQ 6: Floppy Disk
_isr_stub_38:
    push 0
    push 38
    jmp irq_common_stub

; IRQ 7: LPT1
_isr_stub_39:
    push 0
    push 39
    jmp irq_common_stub

; IRQ 8: Real Time Clock
_isr_stub_40:
    push 0
    push 40
    jmp irq_common_stub

; IRQ 9: Redirected IRQ2
_isr_stub_41:
    push 0
    push 41
    jmp irq_common_stub

; IRQ 10: Reserved
_isr_stub_42:
    push 0
    push 42
    jmp irq_common_stub

; IRQ 11: Reserved
_isr_stub_43:
    push 0
    push 43
    jmp irq_common_stub

; IRQ 12: PS/2 Mouse
_isr_stub_44:
    push 0
    push 44
    jmp irq_common_stub

; IRQ 13: Coprocessor
_isr_stub_45:
    push 0
    push 45
    jmp irq_common_stub

; IRQ 14: Primary ATA Hard Disk
_isr_stub_46:
    push 0
    push 46
    jmp irq_common_stub

; IRQ 15: Secondary ATA Hard Disk
_isr_stub_47:
    push 0
    push 47
    jmp irq_common_stub


; =====================================================================
; COMMON DISPATCHER STUBS
; Saves all registers, calls C handler, restores registers, sends EOI
; =====================================================================

; Common stub for exceptions (0-31)
isr_common_stub:
    pusha           ; Push edi, esi, ebp, esp, ebx, edx, ecx, eax

    push esp        ; Pass pointer to registers_t on stack to C handler
    call _isr_handler
    add esp, 4      ; Clean up argument from stack

    popa            ; Restore all general-purpose registers
    add esp, 8      ; Clean up interrupt number and error code
    iret            ; Return from interrupt

; Common stub for hardware IRQs (32-47)
irq_common_stub:
    pusha           ; Push edi, esi, ebp, esp, ebx, edx, ecx, eax

    push esp        ; Pass pointer to registers_t on stack to C handler
    call _irq_handler
    add esp, 4      ; Clean up argument from stack

    ; PIC End-of-Interrupt (EOI) logic in assembly
    ; Find the interrupt number: it is at [esp + 32] (due to pusha pushing 32 bytes)
    mov eax, [esp + 32]
    cmp eax, 40     ; IRQ 8-15 are vectors 40-47. If vector >= 40, we must notify slave PIC
    jl .skip_slave

    ; Send EOI to Slave PIC (port 0xA0)
    mov al, 0x20
    out 0xA0, al

.skip_slave:
    ; Send EOI to Master PIC (port 0x20)
    mov al, 0x20
    out 0x20, al

    popa            ; Restore all general-purpose registers
    add esp, 8      ; Clean up interrupt number and dummy error code
    iret            ; Return from interrupt
