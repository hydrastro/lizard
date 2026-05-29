#!/usr/bin/env python3
"""Audit runtime ownership conventions for Phase 3A.

This audit is intentionally conservative.  It does not prove memory safety, but
it catches the high-value ownership mistakes that make object-level non-moving
GC harder later:

- every direct AST-node heap allocation must initialize node->type nearby;
- mem.c constructors must be explicit lizard_make_* helpers;
- direct C allocation/free is only allowed in modules documented as C-owned;
- report objects must use malloc/free ownership, not the Lizard heap.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import List

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

AST_ALLOC_RE = re.compile(
    r"lizard_heap_alloc\s*\(\s*sizeof\s*\(\s*lizard_ast_node_t\s*\)\s*\)"
)
TYPE_WRITE_RE = re.compile(r"(?:->|\.)type\s*=")
C_ALLOC_RE = re.compile(r"\b(?:malloc|calloc|realloc|free)\s*\(")

C_OWNED_MODULES = {
    "bytecode.c",
    "diagnostic_report.c",
    "gc.c",
    "gc_metadata.c",
    "mem.c",
    "primitives.c",
    "repl.c",
    "runtime.c",
    "syntax_expansion_report.c",
    "tt_equality.c",
}

REPORT_MODULES = {
    "diagnostic_report.c",
    "syntax_expansion_report.c",
}


def fail(errors: List[str], message: str) -> None:
    errors.append(message)


def audit_ast_allocations(errors: List[str]) -> None:
    for path in sorted(SRC.glob("*.c")):
        lines = path.read_text().splitlines()
        for idx, line in enumerate(lines):
            if not AST_ALLOC_RE.search(line):
                continue
            window = "\n".join(lines[idx : idx + 80])
            if not TYPE_WRITE_RE.search(window):
                fail(
                    errors,
                    f"{path.relative_to(ROOT)}:{idx + 1}: direct AST heap allocation "
                    "does not assign node->type within the next 80 lines",
                )


def audit_c_owned_modules(errors: List[str]) -> None:
    for path in sorted(SRC.glob("*.c")):
        if path.name in C_OWNED_MODULES:
            continue
        for lineno, line in enumerate(path.read_text().splitlines(), 1):
            if C_ALLOC_RE.search(line) and "/* ownership-audit: allow */" not in line:
                fail(
                    errors,
                    f"{path.relative_to(ROOT)}:{lineno}: direct malloc/calloc/realloc/free "
                    "outside documented C-owned module",
                )


def audit_report_modules(errors: List[str]) -> None:
    for name in sorted(REPORT_MODULES):
        path = SRC / name
        if not path.exists():
            fail(errors, f"missing report ownership module: src/{name}")
            continue
        text = path.read_text()
        if "malloc" not in text or "free" not in text:
            fail(errors, f"src/{name}: report module should own/free its snapshots explicitly")
        if "lizard_heap_alloc" in text:
            fail(errors, f"src/{name}: report snapshots must not allocate from the Lizard heap")


def audit_object_model(errors: List[str]) -> None:
    header = SRC / "object_model.h"
    source = SRC / "object_model.c"
    if not header.exists():
        fail(errors, "missing src/object_model.h")
    if not source.exists():
        fail(errors, "missing src/object_model.c")
    if header.exists():
        text = header.read_text()
        for needle in (
            "lizard_object_owner_t",
            "lizard_object_trace_policy_t",
            "lizard_object_model_info_t",
        ):
            if needle not in text:
                fail(errors, f"src/object_model.h: missing {needle}")


def audit_gc_metadata(errors: List[str]) -> None:
    header = SRC / "gc_metadata.h"
    source = SRC / "gc_metadata.c"
    if not header.exists():
        fail(errors, "missing src/gc_metadata.h")
    if not source.exists():
        fail(errors, "missing src/gc_metadata.c")
    if header.exists():
        text = header.read_text()
        for needle in (
            "lizard_gc_metadata_table_t",
            "lizard_gc_metadata_stats_t",
            "lizard_gc_metadata_register",
            "lizard_gc_metadata_collect_stats",
        ):
            if needle not in text:
                fail(errors, f"src/gc_metadata.h: missing {needle}")
    if source.exists() and "lizard_heap_alloc" in source.read_text():
        fail(errors, "src/gc_metadata.c: metadata side table must not allocate from the Lizard heap")


def audit_explicit_gc_classification(errors: List[str]) -> None:
    mem = SRC / "mem.c"
    header = SRC / "mem.h"
    if not mem.exists():
        fail(errors, "missing src/mem.c")
        return
    if not header.exists():
        fail(errors, "missing src/mem.h")
        return
    mem_text = mem.read_text()
    header_text = header.read_text()
    if "lizard_heap_alloc_tagged" not in header_text:
        fail(errors, "src/mem.h: missing lizard_heap_alloc_tagged prototype")
    for needle in (
        "LIZARD_GC_OBJECT_AST_NODE",
        "LIZARD_GC_OBJECT_AST_LIST_NODE",
        "LIZARD_OBJECT_TRACE_AST",
        "LIZARD_OBJECT_TRACE_LIST",
    ):
        if needle not in mem_text:
            fail(errors, f"src/mem.c: missing explicit GC classification {needle}")


def main() -> int:
    errors: List[str] = []
    print("Checking runtime ownership conventions")
    audit_ast_allocations(errors)
    audit_c_owned_modules(errors)
    audit_report_modules(errors)
    audit_object_model(errors)
    audit_gc_metadata(errors)
    audit_explicit_gc_classification(errors)

    if errors:
        for message in errors:
            print("  FAIL   " + message)
        print(f"CHECK FAILED: {len(errors)} ownership issue(s).")
        return 1

    print("OK — runtime ownership conventions are auditable.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
