#!/usr/bin/env python3
"""Phase 3D surgical recovery helper.

Run from the repository root after the Phase 3C build-graph closure patch if
strict builds start compiling experimental modules or report-schema headers lose
public prototypes.

The script is idempotent and only edits known build/API boundary files.
"""
from __future__ import print_function

import os
import re
import sys
from pathlib import Path

ROOT = Path.cwd()

CORE_LIB_SRCS = "runtime lizard env mem parser primitives tokenizer printer tt_equality tt_check gc bytecode kernel tactics"
OPTIONAL_LIB_SRCS = "diagnostics object_model gc_metadata report_writer report_schema diagnostic_report expansion_trace_report syntax_expansion_report surface_term expansion_context syntax_expander"

REPORT_SCHEMA_API = r'''

typedef struct lizard_report_schema_info {
  const char *type;
  int version;
  int supports_text;
  int supports_json;
  int stable_v1;
} lizard_report_schema_info_t;

unsigned long lizard_report_schema_count(void);
int lizard_report_schema_get(unsigned long index,
                             lizard_report_schema_info_t *out_info);
const char *lizard_report_schema_list_type(void);
int lizard_report_schema_list_version(void);
int lizard_report_schema_list_fprint(FILE *fp);
int lizard_report_schema_list_fprint_json(FILE *fp);
int lizard_report_schema_require(const char *type,
                                 int min_version,
                                 const char *format,
                                 lizard_report_schema_info_t *out_info);
'''

EXPANSION_TRACE_API = r'''

typedef struct lizard_expansion_trace_event {
  const char *stage;
  const char *detail;
  const char *origin_filename;
  int origin_line;
  int origin_column;
  int origin_phase;
  unsigned long origin_scope_summary;
} lizard_expansion_trace_event_t;

void lizard_expansion_trace_report_destroy(
    lizard_expansion_trace_report_t *report);
unsigned long lizard_expansion_trace_report_count(
    const lizard_expansion_trace_report_t *report);
int lizard_expansion_trace_report_event(
    const lizard_expansion_trace_report_t *report,
    unsigned long index,
    lizard_expansion_trace_event_t *out_event);
int lizard_expansion_trace_report_event_string(
    const lizard_expansion_trace_report_t *report,
    unsigned long index,
    char *buffer,
    size_t buffer_size);
int lizard_expansion_trace_report_fprint(
    FILE *fp,
    const lizard_expansion_trace_report_t *report);
int lizard_expansion_trace_report_fprint_json(
    FILE *fp,
    const lizard_expansion_trace_report_t *report);
'''


def read(path):
    return path.read_text()


def write_if_changed(path, text):
    old = path.read_text() if path.exists() else None
    if old != text:
        path.write_text(text)
        print("updated", path)


def remove_duplicate_struct_typedefs(text, struct_name, typedef_name):
    pattern = re.compile(
        r"\ntypedef\s+struct\s+" + re.escape(struct_name) +
        r"\s*\{\n.*?\}\s*" + re.escape(typedef_name) + r";\n",
        re.S,
    )
    matches = list(pattern.finditer(text))
    if len(matches) <= 1:
        return text
    pieces = []
    last = 0
    for i, match in enumerate(matches):
        pieces.append(text[last:match.start()])
        if i == 0:
            pieces.append(match.group(0))
        last = match.end()
    pieces.append(text[last:])
    return re.sub(r"\n{4,}", "\n\n\n", "".join(pieces))


def remove_duplicate_public_api_definitions(text):
    pairs = [
        ("lizard_expansion_trace_event", "lizard_expansion_trace_event_t"),
        ("lizard_diagnostic_event", "lizard_diagnostic_event_t"),
        ("lizard_report_schema_info", "lizard_report_schema_info_t"),
        ("lizard_source_span", "lizard_source_span_t"),
        ("lizard_diagnostic", "lizard_diagnostic_t"),
    ]
    for struct_name, typedef_name in pairs:
        text = remove_duplicate_struct_typedefs(text, struct_name, typedef_name)
    return text


def replace_makefile():
    path = ROOT / "Makefile"
    s = read(path)
    # Replace the Phase 3C aggressive wildcard closure with a conservative
    # allowlisted closure.  This prevents incomplete experimental modules such
    # as prims_kernel_*.c, prims_modules.c, kernel_sexp.c, etc. from being
    # compiled merely because they exist in src/.
    pattern = re.compile(
        r"(?ms)^LIB_SRCS\s*:=.*?\n"
        r"(?:AUTO_LIB_SRCS\s*:=.*?\n)?"
        r"(?:LIB_SRCS\s*\+=.*?\n)?"
        r"LIB_OBJS\s*:="
    )
    replacement = (
        "LIB_CORE_SRCS := %s\n" % CORE_LIB_SRCS +
        "LIB_OPTIONAL_SRCS := %s\n" % OPTIONAL_LIB_SRCS +
        "EXISTING_OPTIONAL_LIB_SRCS := $(foreach m,$(LIB_OPTIONAL_SRCS),$(if $(wildcard $(SRC_DIR)/$(m).c),$(m)))\n" +
        "LIB_SRCS := $(LIB_CORE_SRCS) $(filter-out $(LIB_CORE_SRCS),$(EXISTING_OPTIONAL_LIB_SRCS))\n" +
        "LIB_OBJS :="
    )
    if not pattern.search(s):
        raise SystemExit("could not find LIB_SRCS/LIB_OBJS block in Makefile")
    s = pattern.sub(replacement, s, count=1)
    # Ensure build-graph-audit is not wired as a hard CI gate until it knows
    # about the allowlist policy.
    write_if_changed(path, s)


def insert_before_endif(path, snippet, sentinel):
    s = read(path)
    if sentinel in s:
        return
    idx = s.rfind("#endif")
    if idx == -1:
        s = s.rstrip() + "\n" + snippet.rstrip() + "\n"
    else:
        s = s[:idx].rstrip() + "\n" + snippet.rstrip() + "\n\n" + s[idx:]
    write_if_changed(path, s)


def ensure_lizard_api():
    path = ROOT / "include" / "lizard_api.h"
    if not path.exists():
        return
    s = remove_duplicate_public_api_definitions(read(path))
    insertions = []
    if "lizard_expansion_trace_event_t" not in s:
        insertions.append(EXPANSION_TRACE_API)
    elif "lizard_expansion_trace_report_fprint_json" not in s:
        insertions.append(EXPANSION_TRACE_API)
    if "lizard_report_schema_info_t" not in s:
        insertions.append(REPORT_SCHEMA_API)
    elif "lizard_report_schema_require" not in s:
        insertions.append(REPORT_SCHEMA_API)
    if insertions:
        idx = s.find("typedef enum")
        if idx == -1:
            idx = s.find("#endif")
        if idx == -1:
            s = s.rstrip() + "\n" + "\n".join(insertions) + "\n"
        else:
            s = s[:idx] + "\n".join(insertions) + "\n" + s[idx:]
    s = remove_duplicate_public_api_definitions(s)
    write_if_changed(path, s)

def ensure_header_includes():
    for rel in ["src/report_schema.h", "src/expansion_trace_report.h", "src/surface_term.h"]:
        path = ROOT / rel
        if not path.exists():
            continue
        s = read(path)
        if '#include "lizard_api.h"' not in s:
            lines = s.splitlines()
            insert_at = 0
            for i, line in enumerate(lines):
                if line.startswith("#define ") or line.startswith("#ifndef "):
                    insert_at = i + 1
            lines.insert(insert_at, '#include "lizard_api.h"')
            write_if_changed(path, "\n".join(lines) + "\n")


def ensure_parser_tokenizer_prototypes():
    parser = ROOT / "src" / "parser.h"
    if parser.exists():
        insert_before_endif(
            parser,
            "lizard_ast_node_t *lizard_reparse_datum(lizard_ast_node_t *datum,\n                                           lizard_heap_t *heap);",
            "lizard_reparse_datum(",
        )
    tokenizer = ROOT / "src" / "tokenizer.h"
    if tokenizer.exists():
        insert_before_endif(
            tokenizer,
            "lz_list_t *lizard_tokenize_source(const char *source,\n                                  const char *filename,\n                                  lizard_diagnostic_t *diagnostic);",
            "lizard_tokenize_source(",
        )


def ensure_nonempty_prims_kernel():
    path = ROOT / "src" / "prims_kernel.c"
    if not path.exists():
        return
    s = read(path)
    stripped = re.sub(r"/\*.*?\*/", "", s, flags=re.S)
    stripped = re.sub(r"//.*", "", stripped)
    meaningful = [line.strip() for line in stripped.splitlines()
                  if line.strip() and not line.strip().startswith("#include")]
    if not meaningful:
        s = s.rstrip() + "\n\ntypedef int lizard_prims_kernel_translation_unit_guard_t;\n"
        write_if_changed(path, s)


def ensure_build_graph_audit():
    path = ROOT / "scripts" / "check-build-graph.py"
    path.parent.mkdir(exist_ok=True)
    script = r'''#!/usr/bin/env python3
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
'''
    write_if_changed(path, script)
    os.chmod(str(path), 0o755)


def main():
    replace_makefile()
    ensure_lizard_api()
    ensure_header_includes()
    ensure_parser_tokenizer_prototypes()
    ensure_nonempty_prims_kernel()
    ensure_build_graph_audit()
    print("Phase 3D recovery edits complete.")

if __name__ == "__main__":
    main()
