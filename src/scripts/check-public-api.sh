#!/usr/bin/env sh
set -eu

header="${1:-include/lizard_api.h}"
failures=0

check_contains() {
  needle="$1"
  label="$2"
  if grep -F "$needle" "$header" >/dev/null 2>&1; then
    printf '  ok     %s\n' "$label"
  else
    printf '  MISSING %s\n' "$label"
    failures=$((failures + 1))
  fi
}

printf 'Checking public API report/type boundary in %s\n' "$header"
check_contains 'typedef struct lizard_expansion_trace_report lizard_expansion_trace_report_t;' 'opaque expansion trace report type'
check_contains 'typedef struct lizard_diagnostic_report lizard_diagnostic_report_t;' 'opaque diagnostic report type'
check_contains 'typedef struct lizard_syntax_expansion_report lizard_syntax_expansion_report_t;' 'opaque syntax expansion report type'
check_contains 'typedef struct lizard_expansion_trace_event {' 'expansion trace event struct'
check_contains '} lizard_expansion_trace_event_t;' 'expansion trace event typedef'
check_contains 'typedef struct lizard_diagnostic_event {' 'diagnostic event struct'
check_contains '} lizard_diagnostic_event_t;' 'diagnostic event typedef'
check_contains 'void lizard_diagnostic_clear(lizard_diagnostic_t *diagnostic);' 'diagnostic clear helper API'
check_contains 'void lizard_diagnostic_set(lizard_diagnostic_t *diagnostic,' 'diagnostic set helper API'
check_contains 'void lizard_source_span_clear(lizard_source_span_t *span);' 'source span clear helper API'
check_contains 'lizard_diagnostic_report_t *lizard_context_diagnostic_report(' 'context diagnostic report API'
check_contains 'void lizard_context_set_trace_expansion(lizard_context_t *context, int enabled);' 'context expansion trace enable API'
check_contains 'unsigned long lizard_context_expansion_trace_count(lizard_context_t *context);' 'context expansion trace count API'
check_contains 'int lizard_context_expansion_trace_event(lizard_context_t *context,' 'context expansion trace event API'
check_contains 'int lizard_context_expansion_trace_event_string(lizard_context_t *context,' 'context expansion trace event string API'
check_contains 'lizard_expansion_trace_report_t *lizard_context_expansion_trace_report(' 'context expansion trace report snapshot API'
check_contains 'lizard_syntax_expansion_report_t *lizard_context_syntax_expansion_report(' 'syntax expansion report API'

if [ "$failures" -ne 0 ]; then
  printf 'CHECK FAILED: %s public API boundary item(s) missing.\n' "$failures"
  exit 1
fi

./scripts/check-public-api-duplicates.py "$header"

printf 'OK — public API report/type boundary is complete.\n'
