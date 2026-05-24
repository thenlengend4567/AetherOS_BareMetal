import os
import sys
import subprocess
import time

def main():
    test_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(test_dir)
    build_script = os.path.join(root_dir, "build.py")
    img_path = os.path.join(root_dir, "aetheros.img")
    
    print("[QEMU AUDIT] === AetherOS QEMU Boot Validation Audit ===")
    
    # 1. Build the image by calling build.py
    print(f"[QEMU AUDIT] Rebuilding operating system image using: {build_script}")
    try:
        result = subprocess.run(
            [sys.executable, build_script],
            cwd=root_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True
        )
        print("[QEMU AUDIT] Build completed successfully.")
    except subprocess.CalledProcessError as e:
        print("[QEMU AUDIT] RESULT: FAIL - build.py compilation failed!")
        print("[BUILD STDOUT]")
        print(e.stdout)
        print("[BUILD STDERR]")
        print(e.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"[QEMU AUDIT] RESULT: FAIL - Unexpected exception during compilation: {e}")
        sys.exit(1)

    # 2. Verify image exists
    if not os.path.exists(img_path):
        print(f"[QEMU AUDIT] RESULT: FAIL - aetheros.img was not generated at {img_path}!")
        sys.exit(1)
        
    # 3. Launch QEMU
    qemu_path = "C:/Program Files/qemu/qemu-system-i386.exe"
    if not os.path.exists(qemu_path):
        print(f"[QEMU AUDIT] RESULT: FAIL - QEMU executable not found at {qemu_path}!")
        sys.exit(1)
        
    cmd = [
        qemu_path,
        "-drive", "file=aetheros.img,format=raw,index=0,media=disk",
        "-display", "none",
        "-serial", "stdio",
        "-no-reboot",
        "-no-shutdown"
    ]
    
    print(f"[QEMU AUDIT] Launching QEMU headless target process...")
    print(f"[QEMU AUDIT] Command: {' '.join(cmd)}")
    
    try:
        proc = subprocess.Popen(
            cmd,
            cwd=root_dir,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
    except Exception as e:
        print(f"[QEMU AUDIT] RESULT: FAIL - Failed to launch QEMU process: {e}")
        sys.exit(1)
        
    # 4. Capture serial output for 10 seconds
    print("[QEMU AUDIT] Monitoring boot status and capturing serial output for 10 seconds...")
    start_time = time.time()
    time_limit = 10.0
    crashed = False
    
    while time.time() - start_time < time_limit:
        ret = proc.poll()
        if ret is not None:
            print(f"[QEMU AUDIT] QEMU terminated prematurely with exit code {ret}!")
            crashed = True
            break
        time.sleep(0.5)
        
    # Terminate QEMU process cleanly
    if proc.poll() is None:
        print("[QEMU AUDIT] Terminating QEMU boot process...")
        proc.terminate()
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            print("[QEMU AUDIT] Force-killing QEMU boot process...")
            proc.kill()
            proc.wait()
            
    # Retrieve captured output
    stdout_bytes, stderr_bytes = proc.communicate()
    stdout = stdout_bytes.decode('utf-8', errors='ignore')
    stderr = stderr_bytes.decode('utf-8', errors='ignore')
    
    print("\n[QEMU AUDIT] === Captured Serial Console Output ===")
    if stdout.strip():
        print(stdout)
    else:
        print("<No serial console output captured>")
    print("[QEMU AUDIT] ======================================\n")
    
    if stderr.strip():
        print("[QEMU AUDIT] === Captured QEMU Standard Error ===")
        print(stderr)
        print("[QEMU AUDIT] ====================================\n")
        
    # 5. Check for known panic strings
    panic_strings = ["PANIC", "GPF", "PAGE FAULT", "TRIPLE FAULT"]
    found_panics = []
    
    for ps in panic_strings:
        if ps in stdout.upper():
            found_panics.append(ps)
            
    # 6. Report PASS/FAIL with status
    if crashed:
        print("[QEMU AUDIT] RESULT: FAIL - QEMU exited prematurely during kernel boot.")
        sys.exit(1)
    elif found_panics:
        print(f"[QEMU AUDIT] RESULT: FAIL - Known panic string(s) detected in serial output: {', '.join(found_panics)}")
        sys.exit(1)
    else:
        print("[QEMU AUDIT] RESULT: PASS - Boot completed successfully with no detected panic strings!")
        sys.exit(0)

if __name__ == "__main__":
    main()
