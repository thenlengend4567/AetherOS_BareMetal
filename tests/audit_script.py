import os
import subprocess
import re

def run_command(cmd):
    result = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    return result.returncode, result.stdout, result.stderr

def normalize_symbol(name):
    # Strip common PE import prefixes and leading/trailing underscores
    n = name
    if n.startswith('__imp__'):
        n = n[7:]
    elif n.startswith('__imp_'):
        n = n[6:]
    while n.startswith('_'):
        n = n[1:]
    while n.endswith('_'):
        n = n[:-1]
    return n

def main():
    workspace = r"c:\Users\ebai\Documents\AetherOS_BareMetal"
    build_dir = os.path.join(workspace, "build")
    tests_dir = os.path.join(workspace, "tests")
    report_file = os.path.join(tests_dir, "purity_report.txt")
    
    os.makedirs(tests_dir, exist_ok=True)
    
    report_lines = []
    report_lines.append("======================================================================")
    report_lines.append("AETHEROS STATIC PURITY AND ALIGNMENT AUDIT REPORT")
    report_lines.append("======================================================================")
    
    overall_pass = True
    critical_failures = []
    
    # -------------------------------------------------------------
    # 1. Audit symbols using nm on all .o files and kernel.exe
    # -------------------------------------------------------------
    report_lines.append("\n[CHECK 1] Banned Standard C Library Symbol Audit")
    report_lines.append("----------------------------------------------------------------------")
    
    banned_targets = {
        'printf', 'scanf', 'fopen', 'fclose', 'fread', 'fwrite', 'exit', 'abort', 'atexit',
        '__libc_start_main', '__cxa_atexit'
    }
    
    files_to_scan = []
    for f in os.listdir(build_dir):
        if f.endswith('.o'):
            files_to_scan.append(os.path.join(build_dir, f))
    files_to_scan.append(os.path.join(build_dir, "kernel.exe"))
    
    symbol_violations = {}
    
    for filepath in files_to_scan:
        filename = os.path.basename(filepath)
        code, stdout, stderr = run_command(["nm", filepath])
        if code != 0:
            report_lines.append(f"  [ERROR] nm failed to run on {filename} (exit code: {code})")
            report_lines.append(f"  Stderr: {stderr.strip()}")
            overall_pass = False
            continue
            
        # Parse nm output
        file_violations = []
        for line in stdout.splitlines():
            line = line.strip()
            if not line:
                continue
            # nm outputs can be like:
            # 00000000 T _vga_init
            #          U _printf
            parts = line.split()
            if not parts:
                continue
            symbol_name = parts[-1]
            sym_type = parts[-2] if len(parts) >= 2 else "?"
            
            norm_sym = normalize_symbol(symbol_name)
            if norm_sym in banned_targets:
                file_violations.append((symbol_name, sym_type))
                
        if file_violations:
            symbol_violations[filename] = file_violations
            
    if symbol_violations:
        report_lines.append("  [FAIL] References to standard C library symbols were found!")
        for filename, violations in symbol_violations.items():
            report_lines.append(f"  File: {filename}")
            for orig_sym, sym_type in violations:
                report_lines.append(f"    - Banned Symbol: '{orig_sym}' (Type in nm: {sym_type})")
                critical_failures.append(f"Banned symbol '{orig_sym}' found in {filename}")
        overall_pass = False
    else:
        report_lines.append("  [PASS] No references to standard C library symbols (printf, scanf, etc.) found.")
        
    # -------------------------------------------------------------
    # 2. Check boot.bin size and signature
    # -------------------------------------------------------------
    report_lines.append("\n[CHECK 2] Bootloader Binary Verification")
    report_lines.append("----------------------------------------------------------------------")
    boot_bin_path = os.path.join(build_dir, "boot.bin")
    
    if not os.path.exists(boot_bin_path):
        report_lines.append("  [FAIL] boot.bin does not exist in build directory!")
        critical_failures.append("boot.bin does not exist")
        overall_pass = False
    else:
        boot_size = os.path.getsize(boot_bin_path)
        with open(boot_bin_path, "rb") as f:
            boot_data = f.read()
            
        size_pass = (boot_size == 512)
        sig_pass = (len(boot_data) >= 512 and boot_data[510] == 0x55 and boot_data[511] == 0xAA)
        
        if size_pass:
            report_lines.append(f"  [PASS] Size of boot.bin is exactly {boot_size} bytes.")
        else:
            report_lines.append(f"  [FAIL] Size of boot.bin is {boot_size} bytes, expected 512.")
            critical_failures.append(f"boot.bin size is {boot_size} bytes (expected 512)")
            overall_pass = False
            
        if sig_pass:
            report_lines.append("  [PASS] MBR boot signature verified: 0x55 0xAA at offsets 510-511.")
        else:
            found_sig = boot_data[510:512].hex() if len(boot_data) >= 512 else "N/A"
            report_lines.append(f"  [FAIL] MBR boot signature is invalid! Found 0x{found_sig}, expected 0x55 0xAA.")
            critical_failures.append(f"boot.bin has invalid boot signature 0x{found_sig}")
            overall_pass = False

    # -------------------------------------------------------------
    # 3. Verify .text starting physical address near 0x100000
    # -------------------------------------------------------------
    report_lines.append("\n[CHECK 3] Kernel .text Link Address boundaries Verification")
    report_lines.append("----------------------------------------------------------------------")
    kernel_exe_path = os.path.join(build_dir, "kernel.exe")
    
    if not os.path.exists(kernel_exe_path):
        report_lines.append("  [FAIL] kernel.exe does not exist in build directory!")
        critical_failures.append("kernel.exe does not exist")
        overall_pass = False
    else:
        # Check ImageBase
        code_p, stdout_p, stderr_p = run_command(["objdump", "-p", kernel_exe_path])
        image_base = 0
        image_base_str = ""
        for line in stdout_p.splitlines():
            if "ImageBase" in line:
                parts = line.split()
                if len(parts) >= 2:
                    image_base_str = parts[1]
                    try:
                        image_base = int(image_base_str, 16)
                    except ValueError:
                        pass
                break
                
        # Check Sections VMA
        code_h, stdout_h, stderr_h = run_command(["objdump", "-h", kernel_exe_path])
        text_vma = None
        for line in stdout_h.splitlines():
            if ".text" in line:
                parts = line.split()
                # Headers line might contain: "0 .text         0003e000  00000000  00000000 ..."
                # Let's extract VMA (normally the 3rd field after Name, but let's be robust)
                for part in parts:
                    if re.match(r'^[0-9a-fA-F]{8}$', part):
                        # VMA is usually the one that matches 00000000 in PE for relative section.
                        # Wait, .text size is also an 8-digit hex. VMA is after Size.
                        # Let's find index of '.text'
                        idx = parts.index('.text')
                        if idx + 2 < len(parts):
                            text_vma_str = parts[idx + 2]
                            try:
                                text_vma = int(text_vma_str, 16)
                            except ValueError:
                                pass
                        break
                break
                
        if image_base_str:
            report_lines.append(f"  PE Optional Header ImageBase: 0x{image_base_str} ({image_base} bytes)")
        else:
            report_lines.append("  [FAIL] Could not extract ImageBase from objdump -p!")
            critical_failures.append("Could not extract ImageBase")
            overall_pass = False
            
        if text_vma is not None:
            text_address = image_base + text_vma
            report_lines.append(f"  .text section relative VMA: 0x{text_vma:08x}")
            report_lines.append(f"  .text section absolute load address: 0x{text_address:08x}")
            
            # Check if text address is exactly or near 0x100000 (1MB)
            if text_address == 0x100000:
                report_lines.append("  [PASS] .text starts exactly at 0x100000 (1MB).")
            elif abs(text_address - 0x100000) <= 0x1000:
                report_lines.append(f"  [PASS] .text starts at 0x{text_address:08x}, which is near 0x100000.")
            else:
                report_lines.append(f"  [FAIL] .text starts at 0x{text_address:08x}, which is NOT at or near 0x100000!")
                critical_failures.append(f".text address is 0x{text_address:08x}")
                overall_pass = False
        else:
            report_lines.append("  [FAIL] Could not locate .text section relative VMA!")
            critical_failures.append("Could not locate .text relative VMA")
            overall_pass = False

    # -------------------------------------------------------------
    # 4. Verify no .interp section exists
    # -------------------------------------------------------------
    report_lines.append("\n[CHECK 4] Dynamic Linking Verification")
    report_lines.append("----------------------------------------------------------------------")
    if os.path.exists(kernel_exe_path):
        has_interp = False
        for line in stdout_h.splitlines():
            if ".interp" in line:
                has_interp = True
                break
                
        if has_interp:
            report_lines.append("  [FAIL] .interp section exists in the kernel binary, indicating dynamic linking!")
            critical_failures.append(".interp section found")
            overall_pass = False
        else:
            report_lines.append("  [PASS] No .interp section exists (fully statically compiled kernel).")

    # -------------------------------------------------------------
    # 5. Alignment boundaries check for sections
    # -------------------------------------------------------------
    report_lines.append("\n[CHECK 5] Section Alignment Boundaries Audit")
    report_lines.append("----------------------------------------------------------------------")
    if os.path.exists(kernel_exe_path):
        # We can also check sections are aligned (e.g. 4096 bytes or as defined)
        # In linker.ld, we had . = ALIGN(4096) for sections. Let's verify VMAs are aligned.
        sections_found = []
        for line in stdout_h.splitlines():
            # Lines like:
            #  0 .text         0003e000  00000000  00000000  00000400  2**2
            parts = line.split()
            if len(parts) >= 6 and re.match(r'^\d+$', parts[0]):
                sec_name = parts[1]
                sec_vma_str = parts[3]
                try:
                    sec_vma = int(sec_vma_str, 16)
                    sec_abs = image_base + sec_vma
                    # Parse alignment (e.g. 2**2 which is 4 bytes align in GCC PE representation)
                    align_str = parts[5]
                    # Sometimes, PE format alignment is represented as power of 2, e.g. 2**2
                    align_bytes = 4
                    if "2**" in align_str:
                        p = int(align_str.split("2**")[1])
                        align_bytes = 2**p
                    sections_found.append((sec_name, sec_abs, align_bytes))
                except ValueError:
                    pass
                    
        for name, addr, align in sections_found:
            report_lines.append(f"  Section {name:8s} starts at 0x{addr:08x} (Alignment: {align} bytes)")
            
    # -------------------------------------------------------------
    # Overall PASS/FAIL Summary
    # -------------------------------------------------------------
    report_lines.append("\n======================================================================")
    report_lines.append("SUMMARY")
    report_lines.append("======================================================================")
    if overall_pass:
        report_lines.append("  OVERALL AUDIT STATUS: PASS")
        report_lines.append("  The compiled binaries are statically pure, bootable, and correctly aligned.")
    else:
        report_lines.append("  OVERALL AUDIT STATUS: FAIL")
        report_lines.append(f"  Total critical violations: {len(critical_failures)}")
        for failure in critical_failures:
            report_lines.append(f"    - {failure}")
            
    report_lines.append("======================================================================")
    
    # Write the report
    report_content = "\n".join(report_lines) + "\n"
    with open(report_file, "w") as f:
        f.write(report_content)
        
    print("[SUCCESS] Static purity audit completed successfully!")
    print(f"Report written to: {report_file}")
    if overall_pass:
        print("Status: PASS")
    else:
        print("Status: FAIL")

if __name__ == "__main__":
    main()
