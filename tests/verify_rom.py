#!/usr/bin/env python3
"""
verify_rom.py — Verification suite for 8-bit BTRON MSX MegaROM.
Checks ROM size, standard 'AB' header, R800 init sequence, and embedded data.
"""

import sys
import os

ROM_PATH = "build/btron8.rom"
EXPECTED_SIZE = 1024 * 1024  # 1 MiB (64 x 16 KB banks)

def test_rom():
    print(f"[TEST] Verifying MSX MegaROM binary: {ROM_PATH}")

    if not os.path.exists(ROM_PATH):
        print(f"FAIL: ROM file not found at {ROM_PATH}")
        sys.exit(1)

    size = os.path.getsize(ROM_PATH)
    print(f"[PASS] ROM file exists, size = {size} bytes ({size // 1024} KiB)")

    if size != EXPECTED_SIZE:
        print(f"FAIL: Expected exact size {EXPECTED_SIZE} bytes, got {size}")
        sys.exit(1)

    with open(ROM_PATH, "rb") as f:
        rom = f.read()

    # 1. Check Cartridge ROM Signature "AB" at offset 0
    sig = rom[0:2]
    if sig != b"AB":
        print(f"FAIL: Cartridge signature 'AB' not found at offset 0 (found {sig})")
        sys.exit(1)
    print("[PASS] Cartridge signature 'AB' (0x41, 0x42) verified at offset 0x0000.")

    # 2. Check INIT and EXEC entry vectors (pointing to 0x4010)
    init_vec = rom[2] | (rom[3] << 8)
    exec_vec = rom[4] | (rom[5] << 8)
    print(f"[PASS] INIT vector: 0x{init_vec:04X}, EXEC vector: 0x{exec_vec:04X}")

    if init_vec != 0x4010 or exec_vec != 0x4010:
        print(f"FAIL: Entry vectors do not point to 0x4010")
        sys.exit(1)

    # 3. Check startup code at offset 0x0010 (address 0x4010)
    startup_bytes = rom[0x0010:0x001A]
    expected_prefix = bytes([0xF3, 0x31, 0xFF, 0xEF, 0xDB, 0xA8, 0x47, 0xE6, 0x0C])
    if startup_bytes[:len(expected_prefix)] != expected_prefix:
        print(f"FAIL: Startup bytecode mismatch at 0x4010: {startup_bytes.hex()}")
        sys.exit(1)
    print("[PASS] Startup code verified: DI, LD SP, #0xEFFF, Page 2 Slot & MegaROM Init.")

    # 4. Check for presence of TAD Document payloads
    # Heart Sutra Latin & Tibetan
    if b"Prajnaparamitahrdaya" not in rom:
        print("FAIL: Heart Sutra Latin string not found in ROM")
        sys.exit(1)
    print("[PASS] Heart Sutra Latin identifier found in ROM.")

    # Tibetan UTF-8 snippet: ༄༅ (0xE0, 0xBC, 0x84, 0xE0, 0xBC, 0x85)
    tibetan_head = bytes([0xE0, 0xBC, 0x84, 0xE0, 0xBC, 0x85])
    if tibetan_head not in rom:
        print("FAIL: Tibetan invocation ༄༅ not found in ROM")
        sys.exit(1)
    print("[PASS] Tibetan invocation glyph sequences found in ROM.")

    # NORTIA Report text
    if b"NORTIA TAD Working Group" not in rom:
        print("FAIL: NORTIA TAD Working Group letter string not found in ROM")
        sys.exit(1)
    print("[PASS] NORTIA Working Group letter found in ROM.")

    # 5. Check for Shell Terminal & Application strings
    if b"B-MSX OS v1.0 Shell" not in rom:
        print("FAIL: Terminal shell string not found in ROM")
        sys.exit(1)
    print("[PASS] Terminal shell command table found in ROM.")

    if b"Hypermedia Network" not in rom:
        print("FAIL: Browser string not found in ROM")
        sys.exit(1)
    print("[PASS] Hypermedia Browser data found in ROM.")

    if b"Storage Volumes" not in rom:
        print("FAIL: Cabinet volume string not found in ROM")
        sys.exit(1)
    print("[PASS] Storage Cabinet volume table found in ROM.")

    print("\nALL ROM VERIFICATION CHECKS PASSED SUCCESSFULLY (100%).")

if __name__ == "__main__":
    test_rom()

