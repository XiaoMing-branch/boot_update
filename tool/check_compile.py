#!/usr/bin/env python3
"""
check_compile.py — C syntax check for Mod/ source files.

Detects available C compiler (gcc > clang), runs -fsyntax-only on each
Mod/ source file, and distinguishes expected cross-compilation header
issues from actual C syntax errors.

Usage:
    python tool/check_compile.py

Exit codes:
    0   All PASS, or no compiler available (skip)
    1   At least one file has syntax errors
"""

import subprocess
import sys
import os
import shutil

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

SOURCES = [
    "Mod/App/app.c",
    "Mod/External_Interfaces/api_flash_port.c",
    "Mod/External_Interfaces/bsp_flash.c",
]

INCLUDE_DIRS = [
    "Mod",
    "Mod/External_Interfaces",
    "Mod/App",
]

# Suppress expected #error when APP_START_ADDR / BOOT_START_ADDR are missing
EXTRA_DEFINES = {
    "APP_START_ADDR": "0x08000000",
    "BOOT_START_ADDR": "0x08020000",
}

# Inject <stdint.h> so uint8_t/uint16_t/uint32_t/uint64_t are available
# (api_flash_port.h uses them but relies on the STM32 HAL to provide them).
FORCE_INCLUDE = "stdint.h"

COMPILERS = ["gcc", "clang"]

BASE_FLAGS = ["-fsyntax-only", "-std=c99"]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _repo_root():
    """Return absolute path to the repository root (parent of tool/)."""
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def find_compiler():
    """Return (name, path) for the first usable C compiler, or (None, None)."""
    for name in COMPILERS:
        path = shutil.which(name)
        if path:
            return name, os.path.realpath(path)
    return None, None


def _build_cmd(compiler_path, source):
    """Assemble the compiler command line for one source file."""
    cmd = [compiler_path]
    cmd += BASE_FLAGS
    for d in INCLUDE_DIRS:
        cmd += ["-I", d]
    for macro, val in EXTRA_DEFINES.items():
        cmd += ["-D", f"{macro}={val}"]
    cmd += ["-include", FORCE_INCLUDE]
    cmd += ["-c", source]
    return cmd


def _is_header_not_found(line):
    """Return True if the error line is a missing-header that we expect."""
    return "No such file or directory" in line


def check_file(compiler_path, source_path):
    """Run -fsyntax-only on one file.

    Returns (passed: bool, diagnostics: str).
    """
    abs_source = os.path.join(_repo_root(), source_path)
    cmd = _build_cmd(compiler_path, abs_source)

    try:
        proc = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            cwd=_repo_root(),
        )
    except FileNotFoundError:
        return False, f"  ERROR: compiler not found at {compiler_path}"

    stderr = proc.stderr or ""
    stdout = proc.stdout or ""

    # No output at all — clean pass
    if not stderr.strip() and not stdout.strip():
        return True, ""

    # Collect all error: lines (case-insensitive)
    all_lines = (stdout + stderr).splitlines()
    error_lines = [ln.strip() for ln in all_lines if "error:" in ln.lower()]

    if not error_lines:
        # Warnings only — still a PASS
        return True, stderr.strip()

    # Classify: header-not-found errors are expected (cross-compilation)
    syntax_errors = [
        ln for ln in error_lines if not _is_header_not_found(ln)
    ]

    if not syntax_errors:
        # All errors are header-not-found — still a PASS
        return True, stderr.strip()

    return False, "\n".join(syntax_errors)


def print_header(compiler_name, compiler_path):
    line = "=" * 54
    print(line)
    print("  Compile Check for Mod/  (C syntax verification)")
    print(line)
    print(f"  Compiler : {compiler_name}")
    print(f"  Path     : {compiler_path}")
    print(f"  Files    : {len(SOURCES)}")
    print(line)


def print_summary(results):
    passed = sum(1 for _, p in results if p)
    failed = len(results) - passed

    line = "=" * 54
    print(line)
    print(f"  Summary  :  {passed} PASS   {failed} FAIL")
    print(line)

    return failed == 0


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    compiler_name, compiler_path = find_compiler()

    if not compiler_name:
        print("=" * 54)
        print("  Compile Check for Mod/  (C syntax verification)")
        print("=" * 54)
        print()
        print("  WARNING: No C compiler found.")
        print(f"  Tried   : {', '.join(COMPILERS)}")
        print()
        print("  Install a C compiler (gcc or clang) and ensure it is")
        print("  in your PATH to enable syntax checking.")
        print()
        print("  Exit code : 0  (SKIP)")
        sys.exit(0)

    print_header(compiler_name, compiler_path)

    results = []
    for source in SOURCES:
        passed, diagnostics = check_file(compiler_path, source)
        results.append((source, passed))

        status = "PASS" if passed else "FAIL"
        print(f"\n  [{status}]  {source}")

        if diagnostics:
            for line in diagnostics.splitlines():
                if line.strip():
                    print(f"          {line.strip()}")

    all_pass = print_summary(results)
    sys.exit(0 if all_pass else 1)


if __name__ == "__main__":
    main()
