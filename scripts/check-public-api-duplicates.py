#!/usr/bin/env python3
"""Reject duplicate public typedef/struct definitions in lizard_api.h.

This catches botched patch/recovery merges that append a public API block more
than once.  Include guards do not protect against duplicate definitions inside
the same header, so this needs an explicit audit.
"""
from __future__ import print_function

import re
import sys
from pathlib import Path

HEADER = Path(sys.argv[1]) if len(sys.argv) > 1 else Path("include/lizard_api.h")
TEXT = HEADER.read_text()

CHECKS = [
    ("lizard_expansion_trace_event", r"typedef\s+struct\s+lizard_expansion_trace_event\s*\{"),
    ("lizard_diagnostic_event", r"typedef\s+struct\s+lizard_diagnostic_event\s*\{"),
    ("lizard_report_schema_info", r"typedef\s+struct\s+lizard_report_schema_info\s*\{"),
    ("lizard_source_span", r"typedef\s+struct\s+lizard_source_span\s*\{"),
    ("lizard_diagnostic", r"typedef\s+struct\s+lizard_diagnostic\s*\{"),
    ("lizard_status_t", r"typedef\s+enum\s*\{[^}]*LIZARD_STATUS_OK"),
    ("lizard_diagnostic_severity_t", r"typedef\s+enum\s*\{[^}]*LIZARD_DIAGNOSTIC_SEVERITY_INFO"),
    ("lizard_diagnostic_category_t", r"typedef\s+enum\s*\{[^}]*LIZARD_DIAGNOSTIC_CATEGORY_UNKNOWN"),
]

failures = 0
for name, pattern in CHECKS:
    count = len(re.findall(pattern, TEXT, flags=re.S))
    if count == 1:
        print("  ok     %s defined once" % name)
    elif count == 0:
        print("  MISSING %s" % name)
        failures += 1
    else:
        print("  DUPLICATE %s appears %d times" % (name, count))
        failures += 1

if failures:
    print("CHECK FAILED: %d duplicate/missing public API definition issue(s)." % failures)
    sys.exit(1)

print("OK — public API definitions are unique.")
