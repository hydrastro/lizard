#!/bin/sh
# scripts/run-examples.sh — honest example runner.
#
# Reads examples/manifest.sexp and runs every example, checking that
# reality matches the declared :expect outcome. Exits nonzero if:
#   * a :expect pass example exits nonzero or prints "Error:"
#   * a :expect error example unexpectedly succeeds
#   * an example file on disk is missing from the manifest
#   * a manifest entry has no corresponding file
#
# :expect experimental examples are run and reported but never gate CI.
#
# Usage: scripts/run-examples.sh [path-to-lizard-binary]
#   defaults to build/lizard

# NOTE: deliberately NOT using `set -e`. Expected-error examples exit
# nonzero on purpose; the runner tracks failures itself and exits at the
# end. `set -e` would abort on the first such example.

BIN="${1:-build/lizard}"
EXAMPLE_DIR="examples"
MANIFEST="$EXAMPLE_DIR/manifest.sexp"

if [ ! -x "$BIN" ]; then
  echo "run-examples: binary not found or not executable: $BIN" >&2
  exit 2
fi
if [ ! -f "$MANIFEST" ]; then
  echo "run-examples: manifest not found: $MANIFEST" >&2
  exit 2
fi

# Real interpreter error signatures:
#   * eval errors print to STDOUT as "Error at L:C (code N):" / "Error (code N):"
#   * parser/tokenizer errors + "parse failed" print to STDERR
# We deliberately do NOT match arbitrary "Error"/"ERROR"/"error" text, since
# programs legitimately print those as data (test harnesses, error-handling
# demos, the CAS, type-error demonstrations, etc.).
STDOUT_ERR_RE='Error at [0-9]|Error \(code [0-9]'
STDERR_ERR_RE='Error:|error at [0-9]|parse failed|tokenization failed|unterminated|Segmentation|Aborted|core dumped|panic'

pass_ok=0
pass_fail=0
err_ok=0
err_fail=0
exp_pass=0
exp_fail=0
missing=0

fail_list=""

# --- extract the expectation for a given filename from the manifest ---
expect_of() {
  # matches: (example "NAME" :expect WHAT)
  grep -F "\"$1\"" "$MANIFEST" 2>/dev/null \
    | sed -n 's/.*:expect[[:space:]]*\([a-z]*\).*/\1/p' \
    | head -1
}

run_one() {
  f="$1"
  base=$(basename "$f")
  expect=$(expect_of "$base")

  if [ -z "$expect" ]; then
    echo "  MISSING-FROM-MANIFEST  $base"
    missing=$((missing + 1))
    fail_list="$fail_list\n  unlisted: $base"
    return
  fi

  errfile="${TMPDIR:-/tmp}/lz_examples_err.$$"
  out=$("$BIN" < "$f" 2>"$errfile"); code=$?
  err=$(cat "$errfile" 2>/dev/null); rm -f "$errfile"

  haserr=0; why=""
  hit=$(printf '%s\n' "$out" | grep -En "$STDOUT_ERR_RE" | head -1)
  if [ -n "$hit" ]; then haserr=1; why="stdout: $hit"; fi
  if [ "$haserr" -eq 0 ]; then
    hit=$(printf '%s\n' "$err" | grep -En "$STDERR_ERR_RE" | head -1)
    if [ -n "$hit" ]; then haserr=1; why="stderr: $hit"; fi
  fi

  case "$expect" in
    pass)
      if [ "$code" -eq 0 ] && [ "$haserr" -eq 0 ]; then
        pass_ok=$((pass_ok + 1))
      else
        pass_fail=$((pass_fail + 1))
        echo "  FAIL (pass)            $base   [exit=$code err=$haserr]"
        if [ -n "$why" ]; then echo "      ↳ $why"; fi
        fail_list="$fail_list\n  pass-failed: $base (exit=$code err=$haserr) ${why}"
      fi
      ;;
    error)
      if [ "$code" -ne 0 ] || [ "$haserr" -eq 1 ]; then
        err_ok=$((err_ok + 1))
      else
        err_fail=$((err_fail + 1))
        echo "  FAIL (error)           $base   [succeeded unexpectedly]"
        fail_list="$fail_list\n  error-but-passed: $base"
      fi
      ;;
    experimental)
      if [ "$code" -eq 0 ] && [ "$haserr" -eq 0 ]; then
        exp_pass=$((exp_pass + 1))
        echo "  exp PASS               $base   (consider promoting to :expect pass)"
      else
        exp_fail=$((exp_fail + 1))
        echo "  exp incomplete         $base   [exit=$code err=$haserr]"
      fi
      ;;
    *)
      echo "  UNKNOWN-EXPECT($expect)  $base"
      fail_list="$fail_list\n  unknown-expect: $base ($expect)"
      ;;
  esac
}

echo "Running examples against manifest..."
for f in "$EXAMPLE_DIR"/*.lisp; do
  run_one "$f"
done

# --- detect manifest entries with no file ---
grep -oE '"[^"]+\.lisp"' "$MANIFEST" | tr -d '"' | while read -r name; do
  if [ ! -f "$EXAMPLE_DIR/$name" ]; then
    echo "  MANIFEST-ENTRY-NO-FILE $name"
  fi
done

echo ""
echo "Summary:"
echo "  pass:         $pass_ok ok, $pass_fail failed"
echo "  error:        $err_ok ok, $err_fail failed"
echo "  experimental: $exp_pass passing, $exp_fail incomplete (non-gating)"
echo "  missing from manifest: $missing"

if [ "$pass_fail" -ne 0 ] || [ "$err_fail" -ne 0 ] || [ "$missing" -ne 0 ]; then
  echo ""
  echo "FAILURES:"
  printf "%b\n" "$fail_list"
  exit 1
fi
echo ""
echo "OK — all gating examples behaved as declared."
exit 0
