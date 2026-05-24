import os
import sys
import subprocess
import shutil

def run_cmd(cmd, cwd=None):
    print(f"[RUNNING] {' '.join(cmd)}")
    result = subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if result.returncode != 0:
        print(f"[ERROR] Command failed with exit code {result.returncode}")
        print("[STDOUT]")
        print(result.stdout)
        print("[STDERR]")
        print(result.stderr)
        sys.exit(1)
    if result.stdout.strip():
        print("[STDOUT]")
        print(result.stdout)
    if result.stderr.strip():
        print("[STDERR]")
        print(result.stderr)
    return result

def main():
    root_dir = os.path.abspath(os.path.dirname(__file__))
    src_dir = os.path.join(root_dir, "src")
    build_dir = os.path.join(root_dir, "build")

    # Recreate build directory
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)
    os.makedirs(build_dir, exist_ok=True)

    print("--- STEP 1: Locating and compiling boot.asm ---")
    boot_asm = None
    for r, d, files in os.walk(src_dir):
        for f in files:
            if f.lower() == "boot.asm":
                boot_asm = os.path.join(r, f)
                break
        if boot_asm:
            break

    if not boot_asm:
        print("[WARNING] boot.asm not found under 'src/'. Checking root...")
        root_boot = os.path.join(root_dir, "boot.asm")
        if os.path.exists(root_boot):
            boot_asm = root_boot

    boot_bin = os.path.join(build_dir, "boot.bin")
    if boot_asm:
        print(f"Found bootloader source at: {boot_asm}")
        run_cmd(["nasm", "-f", "bin", boot_asm, "-o", boot_bin])
    else:
        print("[ERROR] boot.asm could not be found anywhere! Build cannot proceed.")
        sys.exit(1)

    # Verify boot.bin size and signature
    boot_size = os.path.getsize(boot_bin)
    if boot_size != 512:
        print(f"[ERROR] boot.bin size is {boot_size} bytes, but it must be exactly 512 bytes!")
        sys.exit(1)
    
    with open(boot_bin, "rb") as f:
        boot_data = f.read()
    if boot_data[510:512] != b"\x55\xAA":
        print(f"[ERROR] boot.bin does not have valid 0xAA55 boot signature! Found {boot_data[510:512].hex()}")
        sys.exit(1)
    print("boot.bin is verified: exactly 512 bytes with 0xAA55 signature.")

    print("\n--- STEP 2: Gathering Include Directories ---")
    include_dirs = set()
    for r, d, files in os.walk(src_dir):
        if any(f.endswith(".h") for f in files):
            include_dirs.add(r)
    
    include_flags = []
    for d in sorted(include_dirs):
        print(f"Found header directory: {d}")
        include_flags.append(f"-I{d}")

    print("\n--- STEP 3: Compiling C and Assembly source files ---")
    obj_files = []
    
    # GCC Compilation Flags
    # -m32: 32-bit target
    # -O2: Optimization level 2
    # -ffreestanding: compilation target is freestanding
    # -fno-pie: no position independent executable
    # -fno-stack-protector: disable stack protector
    # -nostdlib: do not use system libraries
    # -nostdinc: do not use system include paths
    # -fno-builtin: do not recognize built-in functions
    c_flags = [
        "-m32", "-O2", "-ffreestanding", "-fno-pie", 
        "-fno-stack-protector", "-nostdlib", "-fno-builtin",
        "-std=gnu99", "-mno-sse", "-mno-sse2", "-mno-mmx", "-mno-3dnow"
    ]

    for r, d, files in os.walk(src_dir):
        for f in files:
            full_path = os.path.join(r, f)
            # Skip boot.asm since it is built directly into boot.bin
            if full_path == boot_asm:
                continue
            
            # Skip doom_port.c since VFS and callbacks are directly in kernel_main.c
            if f == "doom_port.c" or f == "doom_port.h":
                continue
            
            rel_path = os.path.relpath(full_path, src_dir)
            obj_name = rel_path.replace(os.sep, "_") + ".o"
            obj_path = os.path.join(build_dir, obj_name)

            if f.endswith(".c"):
                print(f"Compiling C file: {rel_path}")
                cmd = ["gcc"] + c_flags + include_flags + ["-c", full_path, "-o", obj_path]
                run_cmd(cmd)
                obj_files.append(obj_path)
            elif f.endswith(".asm"):
                print(f"Compiling NASM file: {rel_path}")
                cmd = ["nasm", "-f", "win32", full_path, "-o", obj_path]
                run_cmd(cmd)
                obj_files.append(obj_path)
            elif f.endswith(".s") or f.endswith(".S"):
                print(f"Compiling GCC Assembly file: {rel_path}")
                cmd = ["gcc", "-m32", "-c", full_path, "-o", obj_path]
                run_cmd(cmd)
                obj_files.append(obj_path)

    if not obj_files:
        print("[ERROR] No kernel object files compiled! There must be C/Assembly files under 'src/'.")
        sys.exit(1)

    print("\n--- STEP 4: Linking Kernel ---")
    kernel_exe = os.path.join(build_dir, "kernel.exe")
    linker_ld = os.path.join(root_dir, "linker.ld")
    
    # Link using GCC to avoid path resolution bugs in MinGW ld
    link_cmd = ["gcc", "-m32", "-nostdlib", "-Wl,-T," + linker_ld, "-Wl,--image-base,0x0", "-o", kernel_exe] + obj_files
    run_cmd(link_cmd)

    print("\n--- STEP 5: Converting to Flat Binary ---")
    kernel_bin = os.path.join(build_dir, "kernel.bin")
    run_cmd(["objcopy", "-O", "binary", kernel_exe, kernel_bin])

    kernel_size = os.path.getsize(kernel_bin)
    print(f"kernel.bin generated successfully. Size: {kernel_size} bytes.")

    print("\n--- STEP 6: Handling WAD File ---")
    doom_wad_path = None
    # Look for doom1.wad
    for r, d, files in os.walk(root_dir):
        for f in files:
            if f.lower() == "doom1.wad":
                doom_wad_path = os.path.join(r, f)
                break
        if doom_wad_path:
            break

    wad_data = b""
    if doom_wad_path:
        print(f"Found doom1.wad at: {doom_wad_path}")
        with open(doom_wad_path, "rb") as f:
            wad_data = f.read()
    else:
        print("[WARNING] doom1.wad not found in the workspace. Creating a dummy WAD placeholder.")
        # Create a valid minimal dummy WAD file header: "IWAD", 0 lumps, directory offset 12
        wad_data = b"IWAD\x00\x00\x00\x00\x0c\x00\x00\x00"

    print("\n--- STEP 7: Packaging bootable aetheros.img ---")
    img_path = os.path.join(root_dir, "aetheros.img")
    
    # LBA sector 1000 is 1000 * 512 = 512,000 bytes
    lba_1000_offset = 1000 * 512

    # Check if boot.bin + kernel.bin exceeds LBA sector 1000
    total_kernel_sectors = (kernel_size + 511) // 512
    if 512 + kernel_size > lba_1000_offset:
        print(f"[ERROR] Kernel is too large! Boot sector + Kernel size ({512 + kernel_size} bytes) exceeds LBA sector 1000 offset ({lba_1000_offset} bytes).")
        sys.exit(1)

    # Assemble the image
    with open(img_path, "wb") as f_out:
        # Write boot.bin (sector 0)
        f_out.write(boot_data)
        
        # Write kernel.bin (starting at sector 1)
        f_out.write(kernel_bin_data := open(kernel_bin, "rb").read())
        
        # Pad with zeros until LBA sector 1000
        bytes_written = 512 + len(kernel_bin_data)
        padding_size = lba_1000_offset - bytes_written
        f_out.write(b"\x00" * padding_size)
        
        # Write doom1.wad contents
        f_out.write(wad_data)
        if len(wad_data) < 8196 * 512:
            f_out.write(b"\x00" * (8196 * 512 - len(wad_data)))

    print(f"\n[SUCCESS] aetheros.img packaged successfully at: {img_path}")
    print(f"Total size: {os.path.getsize(img_path)} bytes.")

if __name__ == "__main__":
    main()
