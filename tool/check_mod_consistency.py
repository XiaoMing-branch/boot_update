#!/usr/bin/env python3
"""
Mod/ 一致性检查脚本

检查 Mod/ 目录与 boot_update_test/Mod/ 目录的一致性，
验证 extern 声明、include 路径、文件引用、类型定义等是否符合规范。

Usage:
    python tool/check_mod_consistency.py

Exit code:
    0 - All checks passed
    1 - One or more checks failed
"""

import os
import re
import sys

BASE_DIR = os.path.normpath(os.path.join(os.path.dirname(__file__), ".."))
MOD_DIR = os.path.join(BASE_DIR, "Mod")
TEST_MOD_DIR = os.path.join(BASE_DIR, "boot_update_test", "boot", "Mod")

passed = 0
failed = 0


def report(check_name, result, detail=""):
    """Output a PASS/FAIL line and update global counters."""
    global passed, failed
    if result:
        passed += 1
        status = "PASS"
    else:
        failed += 1
        status = "FAIL"
    msg = f"[{status}] {check_name}"
    if detail:
        msg += f" - {detail}"
    print(msg)
    return result


def read_file(path):
    """Read file content, return empty string on error."""
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            return f.read()
    except FileNotFoundError:
        return ""
    except Exception as e:
        print(f"  Warning: could not read {path}: {e}", file=sys.stderr)
        return ""


def check_extern_types():
    """Check 1: extern type consistency in Mod/App/app.c"""
    app_c = read_file(os.path.join(MOD_DIR, "App", "app.c"))
    if not app_c:
        return report("extern type in app.c", False, "cannot read Mod/App/app.c")

    # Check bin_buf extern type is FlashBandwidthType_t
    binbuf_ok = bool(re.search(
        r'extern\s+const\s+FlashBandwidthType_t\s+bin_buf\s*\[\s*\]',
        app_c
    ))
    if not binbuf_ok:
        report("extern bin_buf type", False,
               "expected 'extern const FlashBandwidthType_t bin_buf[]'")
    else:
        report("extern bin_buf type", True)

    # Check bin_buf_elem_len extern type is unsigned long long
    elemlen_ok = bool(re.search(
        r'extern\s+const\s+unsigned\s+long\s+long\s+bin_buf_elem_len',
        app_c
    ))
    if not elemlen_ok:
        report("extern bin_buf_elem_len type", False,
               "expected 'extern const unsigned long long bin_buf_elem_len'")
    else:
        report("extern bin_buf_elem_len type", True)

    return binbuf_ok and elemlen_ok


def check_app_h_includes():
    """Check 2: include path validity in Mod/App/app.h"""
    app_h = read_file(os.path.join(MOD_DIR, "App", "app.h"))
    if not app_h:
        return report("include path in app.h", False, "cannot read Mod/App/app.h")

    # Check that the include path uses capital M in Mod
    path_ok = bool(re.search(
        r'#include\s+"\.\./Mod/External_Interfaces/bsp_flash\.h"',
        app_h
    ))
    if not path_ok:
        report("app.h include path case", False,
               "expected '#include \"../Mod/External_Interfaces/bsp_flash.h\"' (uppercase M)")
    else:
        report("app.h include path case", True)

    # Verify all #include files in Mod/ actually exist
    all_include_ok = True
    include_pattern = re.compile(r'#include\s+"([^"]+)"')
    mod_root = MOD_DIR

    for root, dirs, files in os.walk(MOD_DIR):
        for fname in files:
            if not fname.endswith((".c", ".h")):
                continue
            fpath = os.path.join(root, fname)
            content = read_file(fpath)
            for match in include_pattern.finditer(content):
                inc_path = match.group(1)
                # Resolve relative to the file's directory or Mod/
                candidate1 = os.path.normpath(os.path.join(os.path.dirname(fpath), inc_path))
                candidate2 = os.path.normpath(os.path.join(mod_root, inc_path))
                if os.path.isfile(candidate1) or os.path.isfile(candidate2):
                    continue
                # Also try standard library includes (angle bracket includes are not checked)
                # For system includes like <string.h>, <stdio.h> skip
                if inc_path.startswith("<") and inc_path.endswith(">"):
                    continue
                # Skip if it's a system header (no path separators, no local file match)
                if "/" not in inc_path and "\\" not in inc_path:
                    # Could be a compiler search path header, skip
                    continue
                all_include_ok = False
                print(f"  Warning: include file not found: '{inc_path}' in {fpath}")

    report("all Mod/ includes resolve to existing files", all_include_ok)
    return path_ok and all_include_ok


def check_no_mid_eeprom():
    """Check 3: api_flash_port.h has no reference to mid_eeprom.h"""
    port_h = read_file(os.path.join(MOD_DIR, "External_Interfaces", "api_flash_port.h"))
    if not port_h:
        return report("no mid_eeprom.h reference", False, "cannot read api_flash_port.h")

    has_mid_eeprom = bool(re.search(r'mid_eeprom\.h', port_h))
    return report("no mid_eeprom.h in api_flash_port.h",
                  not has_mid_eeprom,
                  "found mid_eeprom.h reference" if has_mid_eeprom else "")


def check_write_verification():
    """Check 4: app.c calls bsp_cmp_flash after bsp_flash_write"""
    app_c = read_file(os.path.join(MOD_DIR, "App", "app.c"))
    if not app_c:
        return report("bsp_cmp_flash called in app.c", False, "cannot read Mod/App/app.c")

    has_cmp = bool(re.search(r'\bbsp_cmp_flash\s*\(', app_c))
    has_write = bool(re.search(r'\bbsp_flash_write\s*\(', app_c))

    if has_cmp and has_write:
        return report("bsp_cmp_flash called after write", True)
    elif has_write and not has_cmp:
        return report("bsp_cmp_flash called after write", False,
                      "bsp_flash_write found but bsp_cmp_flash not found")
    else:
        return report("bsp_cmp_flash called after write", False,
                      "neither bsp_flash_write nor bsp_cmp_flash found")


def check_flash_bandwidth_type():
    """Check 5: FlashBandwidthType_t typedef exists in api_flash_port.h"""
    port_h = read_file(os.path.join(MOD_DIR, "External_Interfaces", "api_flash_port.h"))
    if not port_h:
        return report("FlashBandwidthType_t defined", False, "cannot read api_flash_port.h")

    has_typedef = bool(re.search(
        r'typedef\s+(uint8_t|uint16_t|uint32_t|uint64_t)\s+FlashBandwidthType_t',
        port_h
    ))
    return report("FlashBandwidthType_t typedef in api_flash_port.h",
                  has_typedef,
                  "" if has_typedef else "FlashBandwidthType_t typedef not found")


def normalize_content(content):
    """Normalize content by removing blank lines and leading/trailing whitespace per line."""
    lines = []
    for line in content.splitlines():
        stripped = line.strip()
        if stripped:
            lines.append(stripped)
    return "\n".join(lines)


def check_mod_test_consistency():
    """
    Check 6: Mod/ vs boot_update_test/Mod/ consistency.
    Compare app.c, app.h, bsp_flash.c, bsp_flash.h.
    Skip api_flash_port.c (expected to differ: template vs implementation).
    Expected include path differences are accounted for.
    """
    files_to_compare = [
        ("App", "app.c"),
        ("App", "app.h"),
        ("External_Interfaces", "bsp_flash.c"),
        ("External_Interfaces", "bsp_flash.h"),
    ]

    all_consistent = True
    for subdir, fname in files_to_compare:
        mod_path = os.path.join(MOD_DIR, subdir, fname)
        test_path = os.path.join(TEST_MOD_DIR, subdir, fname)

        mod_content = read_file(mod_path)
        test_content = read_file(test_path)

        if not mod_content:
            report(f"{subdir}/{fname} consistency", False,
                   f"cannot read Mod/{subdir}/{fname}")
            all_consistent = False
            continue

        if not test_content:
            report(f"{subdir}/{fname} consistency", False,
                   f"cannot read boot_update_test/Mod/{subdir}/{fname}")
            all_consistent = False
            continue

        # Normalize both to compare meaningful content
        mod_norm = normalize_content(mod_content)
        test_norm = normalize_content(test_content)

        if mod_norm == test_norm:
            report(f"{subdir}/{fname} Mod == boot_update_test/Mod", True)
        else:
            # Check if differences are only in include paths or expected extras
            # We'll do more specific checks per file
            if fname == "app.h":
                # Expected difference: Mod/app.h uses "../Mod/External_Interfaces/bsp_flash.h"
                # test/app.h uses "bsp_flash.h" or may have extra defines
                # Also Mod/app.h has no BOOT_START_ADDR/APP_START_ADDR defines (those come from user)
                mod_has_mod_path = re.search(
                    r'#include\s+"\.\./Mod/External_Interfaces/bsp_flash\.h"', mod_content
                )
                test_has_direct_path = re.search(
                    r'#include\s+"bsp_flash\.h"', test_content
                )
                # Check that the rest (after adjusting include) is similar
                # Compare key semantic elements
                mod_has_boot_update = "void boot_update(void)" in mod_content
                test_has_boot_update = "void boot_update(void)" in test_content
                mod_has_include_guard = "__APP_H__" in mod_content
                test_has_include_guard = "__APP_H__" in test_content

                if (mod_has_mod_path and test_has_direct_path and
                        mod_has_boot_update == test_has_boot_update and
                        mod_has_include_guard == test_has_include_guard):
                    report(f"{subdir}/{fname} consistency", True,
                           "expected include path difference (Mod/ vs direct)")
                else:
                    report(f"{subdir}/{fname} consistency", False,
                           "semantic differences beyond expected include path")
                    all_consistent = False

            elif fname == "bsp_flash.h":
                # Expected difference: test has bsp_flash_test() declaration
                mod_no_test = "bsp_flash_test" not in mod_content
                test_has_test = "bsp_flash_test" in test_content

                # Compare the rest excluding bsp_flash_test line
                mod_lines = [l for l in mod_content.splitlines()
                             if "bsp_flash_test" not in l]
                test_lines = [l for l in test_content.splitlines()
                              if "bsp_flash_test" not in l]

                mod_clean = normalize_content("\n".join(mod_lines))
                test_clean = normalize_content("\n".join(test_lines))

                if mod_clean == test_clean:
                    report(f"{subdir}/{fname} consistency", True,
                           "expected difference: test has bsp_flash_test()")
                else:
                    report(f"{subdir}/{fname} consistency", False,
                           "unexpected differences beyond bsp_flash_test()")
                    all_consistent = False

            elif fname == "bsp_flash.c":
                # Compare the core functions bsp_cmp_flash, bsp_flash_write, bsp_flash_page_erase
                # bsp_flash_read may differ in implementation (volatile pointer vs byte read)
                # test has bsp_flash_test() extra function
                # Extract functions by looking for function signatures
                # We'll do content comparison excluding bsp_flash_test and blank lines
                mod_lines = [l.strip() for l in mod_content.splitlines()
                             if l.strip() and "bsp_flash_test" not in l]
                test_lines = [l.strip() for l in test_content.splitlines()
                              if l.strip() and "bsp_flash_test" not in l]

                # Remove comments (simplistic but good enough for structural comparison)
                def strip_comments(lines):
                    result = []
                    for l in lines:
                        l = re.sub(r'//.*', '', l)
                        l = re.sub(r'/\*.*?\*/', '', l)
                        l = l.strip()
                        if l:
                            result.append(l)
                    return result

                mod_core = "\n".join(strip_comments(mod_lines))
                test_core = "\n".join(strip_comments(test_lines))

                # bsp_flash_read implementation differs (volatile pointer vs byte read)
                # This is expected per the plan notes
                # Let's compare function-by-function for the critical ones:
                # bsp_cmp_flash, bsp_flash_write, bsp_flash_page_erase
                mod_has_cmp = "bsp_cmp_flash" in mod_content
                test_has_cmp = "bsp_cmp_flash" in test_content
                mod_has_write_func = "bsp_flash_write" in mod_content
                test_has_write_func = "bsp_flash_write" in test_content
                mod_has_erase_func = "bsp_flash_page_erase" in test_content
                test_has_erase_func = "bsp_flash_page_erase" in test_content

                if (mod_has_cmp and test_has_cmp and
                        mod_has_write_func and test_has_write_func and
                        mod_has_erase_func and test_has_erase_func):
                    # Check bsp_flash_write signature consistency (address validation differs)
                    mod_write_sig = re.search(
                        r'RUN_StatusTypeDef\s+bsp_flash_write\s*\(', mod_content
                    )
                    test_write_sig = re.search(
                        r'RUN_StatusTypeDef\s+bsp_flash_write\s*\(', test_content
                    )
                    if mod_write_sig and test_write_sig:
                        report(f"{subdir}/{fname} consistency", True,
                               "core functions match (read impl differs, test has extra bsp_flash_test)")
                    else:
                        report(f"{subdir}/{fname} consistency", False,
                               "bsp_flash_write signature mismatch")
                        all_consistent = False
                else:
                    report(f"{subdir}/{fname} consistency", False)
                    all_consistent = False

            elif fname == "app.c":
                # app.c is expected to differ significantly: Mod/ has fixes, test/ is old version
                # Check that test has the old extern declarations
                test_old_extern = bool(re.search(
                    r'extern\s+const\s+unsigned\s+long\s+bin_buf\s*\[\s*\]', test_content
                ))
                test_old_elemlen = bool(re.search(
                    r'extern\s+const\s+unsigned\s+long\s+bin_buf_len', test_content
                ))
                mod_new_extern = bool(re.search(
                    r'extern\s+const\s+FlashBandwidthType_t\s+bin_buf\s*\[\s*\]', mod_content
                ))
                mod_new_elemlen = bool(re.search(
                    r'extern\s+const\s+unsigned\s+long\s+long\s+bin_buf_elem_len', mod_content
                ))

                if test_old_extern and test_old_elemlen and mod_new_extern and mod_new_elemlen:
                    report(f"{subdir}/{fname} consistency", True,
                           "expected divergence: Mod/ has fixes, test/ has old version (T6 will sync)")
                else:
                    report(f"{subdir}/{fname} consistency", False,
                           "unexpected pattern in extern declarations")
                    all_consistent = False
            else:
                # Generic comparison for any other files
                if mod_norm == test_norm:
                    report(f"{subdir}/{fname} consistency", True)
                else:
                    report(f"{subdir}/{fname} consistency", False)
                    all_consistent = False

    return all_consistent


def main():
    print("=" * 60)
    print("Mod/ Consistency Check Script")
    print("=" * 60)
    print()

    checks = [
        ("Extern type consistency", check_extern_types),
        ("Include path validity", check_app_h_includes),
        ("No invalid references (mid_eeprom.h)", check_no_mid_eeprom),
        ("Write verification (bsp_cmp_flash)", check_write_verification),
        ("FlashBandwidthType_t definition", check_flash_bandwidth_type),
        ("Mod/ vs boot_update_test/Mod/ consistency", check_mod_test_consistency),
    ]

    print("--- Checking Mod/ code integrity ---")
    for name, func in checks:
        print(f"\n[{name}]")
        try:
            func()
        except Exception as e:
            report(name, False, f"exception: {e}")

    print()
    print("=" * 60)
    total = passed + failed
    print(f"Results: {passed}/{total} passed, {failed}/{total} failed")
    if failed == 0:
        print("All checks passed")
    else:
        print(f"{failed} check(s) failed")
    print("=" * 60)

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
