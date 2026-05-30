#!/usr/bin/env python3
"""Restore context-level expansion-trace public API declarations.

This recovery helper is intentionally narrow.  It repairs mixed trees where the
trace report/event types exist in include/lizard_api.h but the context-level
trace controls were dropped, causing strict tests to see implicit declarations.
"""
from __future__ import print_function

from pathlib import Path

HEADER = Path("include/lizard_api.h")
TEXT = HEADER.read_text()

BLOCK = '''
/* Context-level expansion trace controls and borrowed/latest trace access.
 * These are required by syntax-object tooling tests and embedders that want
 * trace snapshots after normal evaluation.  The owned snapshot API remains
 * lizard_context_expansion_trace_report(). */
void lizard_context_set_trace_expansion(lizard_context_t *context, int enabled);
unsigned long lizard_context_expansion_trace_count(lizard_context_t *context);
int lizard_context_expansion_trace_event(lizard_context_t *context,
                                         unsigned long index,
                                         lizard_expansion_trace_event_t *out_event);
int lizard_context_expansion_trace_event_string(lizard_context_t *context,
                                                unsigned long index,
                                                char *buffer,
                                                size_t buffer_size);
lizard_expansion_trace_report_t *lizard_context_expansion_trace_report(
    lizard_context_t *context);
'''

MARKER = '''const lizard_diagnostic_t *lizard_context_last_diagnostic(
    lizard_context_t *context);
'''

changed = False
if "lizard_context_set_trace_expansion" not in TEXT:
    if MARKER not in TEXT:
        raise SystemExit("cannot find lizard_context_last_diagnostic marker in include/lizard_api.h")
    TEXT = TEXT.replace(MARKER, MARKER + BLOCK, 1)
    changed = True

# Avoid accidental duplicate insertion if recovery scripts are rerun.
for needle in [
    "void lizard_context_set_trace_expansion",
    "unsigned long lizard_context_expansion_trace_count",
    "int lizard_context_expansion_trace_event(",
    "int lizard_context_expansion_trace_event_string(",
    "lizard_expansion_trace_report_t *lizard_context_expansion_trace_report(",
]:
    count = TEXT.count(needle)
    if count != 1:
        raise SystemExit("public trace API declaration count for %s is %d, expected 1" % (needle, count))

if changed:
    HEADER.write_text(TEXT)
    print("updated include/lizard_api.h with context expansion trace declarations")
else:
    print("include/lizard_api.h already has context expansion trace declarations")
