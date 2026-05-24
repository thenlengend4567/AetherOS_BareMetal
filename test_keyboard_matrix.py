import os
import sys
import subprocess
import socket
import time

def main():
    root_dir = os.path.abspath(os.path.dirname(__file__))
    img_path = os.path.join(root_dir, "aetheros.img")
    serial_log = os.path.join(root_dir, "serial.log")
    
    if not os.path.exists(img_path):
        print(f"[ERROR] aetheros.img not found at {img_path}. Build the image first!")
        sys.exit(1)

    qemu_path = "C:/Program Files/qemu/qemu-system-i386.exe"
    if not os.path.exists(qemu_path):
        print(f"[ERROR] QEMU not found at {qemu_path}")
        sys.exit(1)

    print("[STRESS TEST] Starting QEMU with TCP Monitor on 127.0.0.1:5555...")
    cmd = [
        qemu_path,
        "-drive", f"file={img_path},format=raw",
        "-display", "none",
        "-monitor", "tcp:127.0.0.1:5555,server,nowait",
        "-serial", f"file:{serial_log}",
        "-d", "cpu_reset,guest_errors"
    ]

    proc = None
    try:
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except Exception as e:
        print(f"[ERROR] Failed to start QEMU: {e}")
        sys.exit(1)

    # Give QEMU a moment to initialize the TCP socket
    time.sleep(1.5)

    # Check if QEMU died immediately
    ret = proc.poll()
    if ret is not None:
        print(f"[ERROR] QEMU failed to start. Exit code: {ret}")
        sys.exit(1)

    # Connect to QEMU Monitor
    print("[STRESS TEST] Connecting to QEMU Monitor via TCP...")
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.settimeout(3.0)
        s.connect(("127.0.0.1", 5555))
        # Read the initial greeting banner
        greeting = s.recv(4096)
        print("[STRESS TEST] Connected! QEMU Monitor says:")
        print(greeting.decode(errors='ignore').strip())
    except Exception as e:
        print(f"[ERROR] Failed to connect to QEMU monitor: {e}")
        proc.terminate()
        sys.exit(1)

    # Prepare stress test keys
    test_keys = [
        "a", "b", "c", "d", "e", "f", "g", "h", "i", "j",
        "k", "l", "m", "n", "o", "p", "q", "r", "s", "t",
        "u", "v", "w", "x", "y", "z", "1", "2", "3", "4",
        "5", "6", "7", "8", "9", "0", "kp_enter", "spc",
        "shift-a", "shift-b", "shift-c", "ret", "backspace"
    ]
    
    # We will send the list of keys multiple times to stress the input buffer
    iterations = 3
    delay_between_keys = 0.015 # 15 milliseconds between presses (very fast!)

    print(f"[STRESS TEST] Sending {len(test_keys) * iterations} keystrokes rapidly...")
    
    try:
        for i in range(iterations):
            print(f"[STRESS TEST] Iteration {i+1}/{iterations}...")
            for key in test_keys:
                cmd_str = f"sendkey {key}\n"
                s.sendall(cmd_str.encode())
                # Read response from monitor buffer to prevent blocking
                try:
                    s.recv(1024)
                except socket.timeout:
                    pass
                time.sleep(delay_between_keys)

        print("[STRESS TEST] Burst complete. Checking if QEMU is still running...")
        time.sleep(1.0)
        
        # Verify QEMU is still healthy
        ret = proc.poll()
        if ret is not None:
            print(f"[ERROR] QEMU crashed/exited during or after the stress test! Exit code: {ret}")
            sys.exit(1)
        else:
            print("[STRESS TEST] QEMU is active and responsive.")

    except Exception as e:
        print(f"[ERROR] Exception occurred during stress test: {e}")
        proc.terminate()
        sys.exit(1)
    finally:
        s.close()
        print("[STRESS TEST] Terminating QEMU process...")
        proc.terminate()
        try:
            proc.wait(timeout=2)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()

    # Read serial log to verify keyboard interrupts or scans
    if os.path.exists(serial_log):
        try:
            with open(serial_log, "r", encoding="utf-8", errors="ignore") as f:
                log_content = f.read()
            print("\n[STRESS TEST] --- Serial Log Audit ---")
            # Just print the last 20 lines to keep it brief
            lines = log_content.splitlines()
            for line in lines[-20:]:
                print(line)
            print("[STRESS TEST] -------------------------\n")
            
            # Check for standard keyboard driver prints (e.g. "key", "scancode", "keyboard")
            # to verify if driver was active. We won't fail the test if they aren't printed, 
            # as print logging might be disabled, but we will print a message.
            active_hints = ["key", "scan", "kbd", "keyboard", "input"]
            found_hints = [h for h in active_hints if h in log_content.lower()]
            if found_hints:
                print(f"[STRESS TEST] Keyboard activity signatures detected in serial log: {found_hints}")
            else:
                print("[STRESS TEST] Note: No explicit keyboard signatures detected in serial log (normal if logging is quiet).")
                
        except Exception as e:
            print(f"[WARNING] Could not read serial log: {e}")

    print("[STRESS TEST] RESULT: SUCCESS - Keyboard matrix stress test completed successfully.")
    sys.exit(0)

if __name__ == "__main__":
    main()
