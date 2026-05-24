[org 0x7C00]
[bits 16]

    jmp 0:start ; Perform far jump to set CS to 0

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00 ; Temporary stack right before the bootloader
    sti

    mov [BOOT_DRIVE], dl ; Save the boot drive ID

    ; Let's load the kernel using BIOS Extended Read (LBA)
    ; We read 999 sectors in chunks of 64 sectors to fit into conventional memory.
    ; First check if LBA is supported.
    mov ah, 0x41
    mov bx, 0x55AA
    mov dl, [BOOT_DRIVE]
    int 0x13
    jc disk_error    ; Our large kernel requires LBA support
    cmp bx, 0xAA55
    jne disk_error

    ; LBA is supported! Let's loop-read 999 sectors.
    mov word [dap_segment], 0x1000
    mov dword [dap_lba], 1
    mov cx, 15       ; 15 chunks of 64 sectors = 960 sectors
.read_loop:
    push cx
    mov word [dap_sectors], 64
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    mov si, dap
    int 0x13
    jc disk_error

    add dword [dap_lba], 64
    add word [dap_segment], 0x0800
    pop cx
    loop .read_loop

    ; Read remaining 39 sectors (960 + 39 = 999 sectors)
    mov word [dap_sectors], 39
    mov ah, 0x42
    mov dl, [BOOT_DRIVE]
    mov si, dap
    int 0x13
    jc disk_error

.load_ok:
    ; Enable A20 gate
    call enable_a20

    ; Switch to Protected Mode
    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to 32-bit code segment
    jmp CODE_SEG:init_pm

disk_error:
    ; Print error message and hang
    mov si, msg_disk_error
print_string:
    lodsb
    or al, al
    jz .hang
    mov ah, 0x0E
    int 0x10
    jmp print_string
.hang:
    cli
    hlt
    jmp .hang

enable_a20:
    ; Try BIOS INT 15h
    mov ax, 0x2401
    int 0x15
    jnc .check_a20_done
    
    ; Try Fast A20
    in al, 0x92
    or al, 2
    out 0x92, al

.check_a20_done:
    ret

[bits 32]
init_pm:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000
    mov ebp, 0x90000

    ; Copy conventional kernel from 0x10000 to 0x100000 (1MB)
    mov esi, 0x10000
    mov edi, 0x100000
    mov ecx, 128000     ; 128000 * 4 = 512,000 bytes (1000 sectors)
    rep movsd

    ; Jump to kernel entry point
    jmp 0x100000

; Variables & GDT
BOOT_DRIVE db 0
msg_disk_error db "Disk read error!", 0

align 4
dap:
    db 0x10         ; Size of DAP
    db 0            ; Unused
dap_sectors:
    dw 64           ; Number of sectors to read
dap_offset:
    dw 0x0000       ; Destination offset
dap_segment:
    dw 0x1000       ; Destination segment
dap_lba:
    dd 1            ; Start LBA (low 32-bits)
    dd 0            ; Start LBA (high 32-bits)

align 4
gdt_start:
    dd 0
    dd 0
gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x9A
    db 0xCF
    db 0x00
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 0x92
    db 0xCF
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 510-($-$$) db 0
dw 0xAA55
