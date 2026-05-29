#!/usr/bin/env python3
"""Audit Lizard header include layering.

This is intentionally lightweight and conservative. It does not try to replace a
compiler; it checks architectural invariants that the compiler will not catch:

- public API headers must not include implementation headers;
- implementation headers must not include include/lizard.h;
- implementation-header quoted include graph must be acyclic;
- lizard_api.h must remain the root public API surface.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path
from typing import Dict, Iterable, List, Set, Tuple

ROOT = Path(__file__).resolve().parents[1]
INCLUDE_DIR = ROOT / "include"
SRC_DIR = ROOT / "src"

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+([<"])([^>"]+)[>"]')

PUBLIC_HEADERS = {
    "include/lizard_api.h",
    "include/lizard.h",
}

ALLOWED_PUBLIC_QUOTED = {
    "include/lizard.h": {"lizard_api.h"},
    "include/lizard_api.h": set(),
}

# Headers that are intentionally low-level and should not depend on the large
# interpreter internals. This prevents tooling/report helper headers from
# accidentally pulling in lizard_internal.h and its external dependencies.
LEAF_INTERNAL_HEADERS = {
    "src/report_writer.h",
    "src/diagnostics.h",
    "src/diagnostic_report.h",
    "src/syntax_expansion_report.h",
}

LEAF_FORBIDDEN_QUOTED = {
    "lizard_internal.h",
    "runtime.h",
    "parser.h",
    "tokenizer.h",
    "primitives.h",
    "kernel.h",
    "bytecode.h",
    "gc.h",
}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def iter_headers() -> Iterable[Path]:
    yield from sorted(INCLUDE_DIR.glob("*.h"))
    yield from sorted(SRC_DIR.glob("*.h"))


def includes(path: Path) -> List[Tuple[str, str, int]]:
    out: List[Tuple[str, str, int]] = []
    for lineno, line in enumerate(path.read_text().splitlines(), 1):
        match = INCLUDE_RE.match(line)
        if match:
            out.append((match.group(1), match.group(2), lineno))
    return out


def fail(errors: List[str], message: str) -> None:
    errors.append(message)


def audit_public_headers(errors: List[str]) -> None:
    for rel_header in sorted(PUBLIC_HEADERS):
        path = ROOT / rel_header
        if not path.exists():
            fail(errors, f"MISSING public header: {rel_header}")
            continue
        allowed = ALLOWED_PUBLIC_QUOTED[rel_header]
        for style, target, lineno in includes(path):
            if style == '"' and target not in allowed:
                fail(
                    errors,
                    f"{rel_header}:{lineno}: public header may not include \"{target}\"",
                )
            if style == '"' and (target.startswith("../") or target.startswith("src/")):
                fail(
                    errors,
                    f"{rel_header}:{lineno}: public header leaks implementation include {target}",
                )


def audit_src_header_edges(errors: List[str]) -> Dict[str, Set[str]]:
    graph: Dict[str, Set[str]] = {}
    known_src_headers = {path.name: rel(path) for path in SRC_DIR.glob("*.h")}

    for path in sorted(SRC_DIR.glob("*.h")):
        rel_header = rel(path)
        graph.setdefault(rel_header, set())
        for style, target, lineno in includes(path):
            if style != '"':
                continue
            if target == "lizard.h" or target.startswith("include/"):
                fail(
                    errors,
                    f"{rel_header}:{lineno}: implementation header must not include public wrapper {target}",
                )
            if target.startswith("../"):
                fail(errors, f"{rel_header}:{lineno}: parent-relative include is forbidden: {target}")
            if rel_header in LEAF_INTERNAL_HEADERS and target in LEAF_FORBIDDEN_QUOTED:
                fail(
                    errors,
                    f"{rel_header}:{lineno}: leaf tooling/report header must not include {target}",
                )
            if target in known_src_headers:
                graph[rel_header].add(known_src_headers[target])
    return graph


def audit_cycles(graph: Dict[str, Set[str]], errors: List[str]) -> None:
    visiting: Set[str] = set()
    visited: Set[str] = set()
    stack: List[str] = []

    def dfs(node: str) -> None:
        if node in visited:
            return
        if node in visiting:
            try:
                start = stack.index(node)
                cycle = stack[start:] + [node]
            except ValueError:
                cycle = stack + [node]
            fail(errors, "header include cycle: " + " -> ".join(cycle))
            return
        visiting.add(node)
        stack.append(node)
        for nxt in sorted(graph.get(node, ())):
            dfs(nxt)
        stack.pop()
        visiting.remove(node)
        visited.add(node)

    for node in sorted(graph):
        dfs(node)


def main() -> int:
    errors: List[str] = []
    print("Checking include-layer boundaries")
    audit_public_headers(errors)
    graph = audit_src_header_edges(errors)
    audit_cycles(graph, errors)

    if errors:
        for item in errors:
            print("  FAIL  " + item)
        print(f"CHECK FAILED: {len(errors)} include-layer issue(s).")
        return 1

    print("OK — include layers are acyclic and public/internal boundaries are clean.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
