#!/usr/bin/env sh
set -eu

failures=0

# Public API types that implementation headers are allowed to expose only when
# they include lizard_api.h themselves. This catches include-order regressions
# such as surface_term.h publishing lizard_expansion_trace_event_t without
# making that type visible first.
public_types='lizard_expansion_trace_event_t lizard_diagnostic_event_t lizard_report_schema_info_t lizard_source_span_t lizard_diagnostic_t lizard_status_t lizard_diagnostic_severity_t lizard_diagnostic_category_t'

printf 'Checking src/*.h public API type include boundaries\n'

for header in src/*.h; do
  [ -f "$header" ] || continue
  needs_api=0
  for type in $public_types; do
    if grep -E "(^|[^A-Za-z0-9_])${type}([^A-Za-z0-9_]|$)" "$header" >/dev/null 2>&1; then
      needs_api=1
    fi
  done

  if [ "$needs_api" -eq 1 ]; then
    if grep -E '^#include "lizard_api\.h"' "$header" >/dev/null 2>&1; then
      printf '  ok     %s includes lizard_api.h\n' "$header"
    else
      printf '  MISSING %s uses public API types but does not include lizard_api.h\n' "$header"
      failures=$((failures + 1))
    fi
  fi
done

if [ "$failures" -ne 0 ]; then
  printf 'CHECK FAILED: %s header boundary issue(s).\n' "$failures"
  exit 1
fi

printf 'OK — implementation headers expose public API types with explicit lizard_api.h includes.\n'
