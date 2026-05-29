#!/usr/bin/env python3
"""Audit the liblizard build graph without compiling experimental modules."""
from __future__ import print_function

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAKEFILE = ROOT / "Makefile"
SRC = ROOT / "src"

EXCLUDED = set([
    "repl",
    "prims_kernel", "prims_kernel_defs", "prims_kernel_meta", "prims_kernel_proof",
    "prims_modules", "prims_bytecode", "kernel_sexp",
])
REQUIRED_IF_PRESENT = set([
    "surface_term", "expansion_context", "syntax_expander",
    "expansion_trace_report", "report_schema", "diagnostic_report",
    "syntax_expansion_report", "report_writer", "diagnostics",
    "object_model", "gc_metadata",
])

text = MAKEFILE.read_text()
if "AUTO_LIB_SRCS := $(sort $(basename" in text:
    print("FAIL: Makefile still uses aggressive src/*.c AUTO_LIB_SRCS closure")
    sys.exit(1)

m = re.search(r"(?m)^LIB_CORE_SRCS\s*:=\s*(.*)$", text)
if not m:
    print("FAIL: Makefile lacks LIB_CORE_SRCS")
    sys.exit(1)
core = set(m.group(1).split())

m = re.search(r"(?m)^LIB_OPTIONAL_SRCS\s*:=\s*(.*)$", text)
if not m:
    print("FAIL: Makefile lacks LIB_OPTIONAL_SRCS")
    sys.exit(1)
optional = set(m.group(1).split())

missing_required = []
for path in SRC.glob("*.c"):
    stem = path.stem
    if stem in REQUIRED_IF_PRESENT and stem not in core and stem not in optional:
        missing_required.append(stem)

if missing_required:
    print("FAIL: build graph omits required optional source(s):")
    for name in sorted(missing_required):
        print("  ", name)
    sys.exit(1)

bad = [name for name in EXCLUDED if name in core or name in optional]
if bad:
    print("FAIL: experimental/incomplete source listed in LIB_*_SRCS:")
    for name in sorted(bad):
        print("  ", name)
    sys.exit(1)

print("OK: build graph uses conservative allowlisted closure")
