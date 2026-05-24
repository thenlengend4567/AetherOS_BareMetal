# AetherOS: Bare-Metal Doom OS 👾⚙️

AetherOS is a custom, freestanding, single-tasking 32-bit x86 operating system written from scratch in C and x86 Assembly that boots directly into the 1993 classic game **DOOM**! 

Unlike modern operating systems, AetherOS runs directly on bare-metal hardware without any underlying system like Windows or Linux. It contains its own custom bootloader, VGA graphic drivers, memory management heaps, PS/2 keyboard interface, and CPU hardware clock system.

![AetherOS Logo](https://img.shields.io/badge/OS-Bare--Metal-brightgreen)
![Architecture](https://img.shields.io/badge/Architecture-x86_32--bit-blue)
![Language](https://img.shields.io/badge/Languages-C%20%2F%20Assembly-orange)

---

## 🚀 Features Under the Hood

* 💾 **16-bit Assembly Bootloader (`boot.asm`):** Fits exactly inside Sector 0 (512 bytes) of the disk. Configures segments, enables the CPU A20 high-memory address gate, remaps the motherboard's Programmable Interrupt Controller (PIC), establishes Global Descriptor Table (GDT) segmentation, and jumps the CPU from 16-bit Real Mode into **32-bit Protected Mode**.
* 🧠 **Custom $O(1)$ Heap Allocator:** A sequential bump allocator starting at memory base `0x01000000`. Features in-place expansion optimization for consecutive `realloc` calls, cutting memory usage dramatically and solving heap exhaustion during WAD parsing loops.
* 🖥️ **VGA Mode 13h Graphic Driver:** Communicates directly with VGA ports to program $320 \times 200$ with 256 colors. Renders to a double frame-buffer at `0x00700000` before using high-speed assembly string operations (`rep movsd`) to blit frames directly to physical Video RAM at `0xA0000`.
* ⌨️ **PS/2 Keyboard Matrix Driver:** Communicates directly with the 8042 controller on IRQ 1. Features an advanced initialization sequence that drains BIOS buffer leftovers, enables hardware scanning (`0xF4`), and maps raw Set 1 scancodes to engine keypress events.
* 🔊 **PC Speaker PIT Audio Driver:** Programs Channel 2 of the Programmable Interval Timer (PIT) by writing square wave frequencies to ports `0x43`/`0x42`. Connects them to the motherboard's speaker gate (`0x61`) to play a satisfying retro console double-chime startup sound on boot!
* ⏱️ **Nanosecond-Accurate TSC Clock:** Bypasses software timer interrupts. Reads the silicon clock cycle counts directly from the CPU core using the x86 `rdtsc` instruction, ensuring a perfectly smooth, drift-free, and hyper-accurate 35 FPS game loop.
* 📁 **Virtual File System (VFS):** A lightweight VFS parser that loads the game WAD from raw hard drive sectors starting at sector 1000 and maps file IO operations straight to memory.

---

## 📂 Repository File Map

```text
AetherOS_BareMetal/
├── build.py                  # The automated compilation pipeline
├── linker.ld                 # Custom GCC linker memory map script
├── src/
│   ├── kernel_main.c         # Main kernel initializer & event loop
│   ├── boot/
│   │   └── boot.asm          # 16-bit Assembly bootloader
│   ├── drivers/
│   │   ├── ata.c / .h        # LBA PIO hard drive sector reader
│   │   ├── audio.c / .h      # PC Speaker frequency generator & chime
│   │   ├── idt.c / .h        # 256-gate Interrupt Descriptor Table (IDT)
│   │   ├── isr.asm           # Assembly ISR stubs and PIC EOI signals
│   │   ├── keyboard.c / .h   # PS/2 keyboard matrix driver
│   │   └── vga.c / .h        # VGA Mode 13h double-buffered renderer
│   ├── libc/
│   │   ├── freestanding.c    # Mem operations, malloc/realloc, and 64-bit math
│   │   └── freestanding.h    # Custom type overrides and context frames
│   ├── vfs/
│   │   ├── vfs.c / .h        # Memory-backed WAD Virtual File System
│   └── doom/
│       └── PureDOOM.h        # Stripped, OS-independent Doom engine
```

---

## 🛠️ How to Compile & Run

### 1. Prerequisites
You need the following installed on your host system:
* **Python 3** (to execute the build script)
* **GCC / MinGW** (for 32-bit compilation)
* **NASM** (for assembling the bootloader and interrupts)
* **QEMU** (to run the bootable disk image)
* **`doom1.wad`** (Place the official Shareware WAD in the root directory)

### 2. Build the OS
Compile all C/Assembly source files, link the kernel at the `0x100000` memory offset, and package the boot sector, kernel, and WAD into a flat bootable disk image:
```bash
python build.py
```

### 3. Play AetherOS in QEMU
Run the following command to boot AetherOS with DirectSound PC Speaker audio and automatic high-quality GTK display window scaling:
```powershell
& "C:\Program Files\qemu\qemu-system-i386.exe" -drive file="aetheros.img",format=raw,index=0,media=disk -serial stdio -display gtk,zoom-to-fit=on -audiodev dsound,id=snd0 -machine pcspk-audiodev=snd0
```

### 🎮 Game Controls
* **Move / Look:** Arrow Keys
* **Shoot:** `Left Control` (Ctrl)
* **Activate / Use:** `Space`
* **Weapon Select:** `1` - `7`
* **Menu Options:** `Enter`
* **Menu Back / Exit:** `Escape`
* **Capture Focus:** Click inside the QEMU window. (Press `Ctrl + Alt` together to release mouse/keyboard capture at any time).
