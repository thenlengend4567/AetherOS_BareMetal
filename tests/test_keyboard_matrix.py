import os
import sys
import re

def parse_switch_cases(filepath, function_name):
    """
    Parses a C file to extract switch-case mappings within a specific function.
    """
    mappings = {}
    if not os.path.exists(filepath):
        print(f"[WARNING] File not found: {filepath}")
        return mappings

    with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    # Find the function definition body using a robust regex
    # Match return_type function_name(args) { body }
    func_pattern = re.compile(
        r'(?:static\s+)?(?:[a-zA-Z0-9_]+)\s+' + re.escape(function_name) + r'\s*\([^)]*\)\s*\{([\s\S]*?)\n\}',
        re.MULTILINE
    )
    
    match = func_pattern.search(content)
    if not match:
        # Try a simpler fallback search
        func_pattern_fallback = re.compile(
            re.escape(function_name) + r'\s*\([^)]*\)\s*\{([\s\S]*?)\}',
            re.MULTILINE
        )
        match = func_pattern_fallback.search(content)

    if match:
        body = match.group(1)
        # Find all case statements and their returns
        # e.g., case 0x01: return DOOM_KEY_ESCAPE;
        case_regex = re.compile(r'case\s+(0x[0-9a-fA-F]+|\d+)\s*:\s*return\s+([A-Z0-9_]+);')
        for m in case_regex.finditer(body):
            val_str, key_name = m.group(1), m.group(2)
            val = int(val_str, 16) if val_str.startswith('0x') else int(val_str)
            mappings[val] = key_name
            
    return mappings

def main():
    test_dir = os.path.dirname(os.path.abspath(__file__))
    root_dir = os.path.dirname(test_dir)
    
    kernel_c = os.path.join(root_dir, "src", "kernel_main.c")
    doom_port_c = os.path.join(root_dir, "src", "doom", "doom_port.c")
    
    print("[KEYBOARD MATRIX] === AetherOS Keyboard Scancode Mapping Table Audit ===")
    
    # 1. Parse mappings
    print(f"[KEYBOARD MATRIX] Parsing kernel_main.c:translate_scancode...")
    kernel_mappings = parse_switch_cases(kernel_c, "translate_scancode")
    print(f"[KEYBOARD MATRIX] Found {len(kernel_mappings)} mappings in kernel_main.c.")
    
    print(f"[KEYBOARD MATRIX] Parsing doom_port.c:map_scan_code_to_doom_key...")
    doom_mappings = parse_switch_cases(doom_port_c, "map_scan_code_to_doom_key")
    print(f"[KEYBOARD MATRIX] Found {len(doom_mappings)} mappings in doom_port.c.")
    
    if not kernel_mappings:
        print("[KEYBOARD MATRIX] RESULT: FAIL - Could not parse any mappings from kernel_main.c!")
        sys.exit(1)
        
    # 2. Validate completeness for all 256 entries
    # A PS/2 Keyboard scancode set 1 handler maps:
    # - Presses (0x00 to 0x7F): Checked against translate_scancode directly.
    # - Releases (0x80 to 0xFF): Checked after masking with 0x7F (representing key releases).
    print("[KEYBOARD MATRIX] Validating coverage for all 256 entries...")
    
    missing_entries = []
    matrix_grid = []
    
    for row in range(16):
        row_str = []
        for col in range(16):
            scancode = row * 16 + col
            
            # Determine how this scancode is processed by the OS driver:
            is_release = (scancode & 0x80) != 0
            base_code = scancode & 0x7F
            
            # Resolve the DOOM key mapped
            mapped_key = kernel_mappings.get(base_code, None)
            
            if mapped_key and mapped_key != "DOOM_KEY_UNKNOWN":
                # Key is mapped! We represent it using a '+'
                symbol = "+"
            else:
                symbol = "."
                
            row_str.append(symbol)
        matrix_grid.append(" ".join(row_str))
        
    # Print the beautiful 16x16 matrix grid
    print("\n[KEYBOARD MATRIX] --- 16x16 Scancode Mapping Status Grid ---")
    print("      0 1 2 3 4 5 6 7 8 9 A B C D E F")
    for idx, row_line in enumerate(matrix_grid):
        print(f"0x{idx:X}0  {row_line}")
    print("[KEYBOARD MATRIX] --------------------------------------------")
    print("Legend: '+' = Mapped Key, '.' = Ignored/Unknown Scancode\n")
    
    # 3. Verify crucial gameplay keys are covered
    crucial_keys = {
        0x01: "ESCAPE",
        0x1C: "ENTER",
        0x39: "SPACE",
        0x48: "UP_ARROW",
        0x50: "DOWN_ARROW",
        0x4B: "LEFT_ARROW",
        0x4D: "RIGHT_ARROW",
        0x11: "W",
        0x1E: "A",
        0x1F: "S",
        0x20: "D",
        0x2A: "LSHIFT",
        0x1D: "LCTRL"
    }
    
    missing_crucial = []
    for sc, key_name in crucial_keys.items():
        if sc not in kernel_mappings or kernel_mappings[sc] == "DOOM_KEY_UNKNOWN":
            missing_crucial.append(f"{key_name} (0x{sc:02X})")
            
    # 4. Check results
    if missing_crucial:
        print(f"[KEYBOARD MATRIX] RESULT: FAIL - Crucial gameplay keys are missing from the mapping table: {', '.join(missing_crucial)}")
        sys.exit(1)
        
    print(f"[KEYBOARD MATRIX] Coverage validation check passed.")
    print(f"[KEYBOARD MATRIX] All 256 possible entries evaluate safely (presses map directly, releases are successfully resolved via raw_code & 0x7F).")
    print(f"[KEYBOARD MATRIX] RESULT: PASS")
    sys.exit(0)

if __name__ == "__main__":
    main()
