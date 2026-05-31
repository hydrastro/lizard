#!/bin/sh
# scripts/run-compiler-tests.sh
#
# Differential test harness for the Phase 9 native compiler.
#
# For each tests/compiler/*.scm program it:
#   1. evaluates it with the interpreter (build/lizard),
#   2. compiles it to C with lib/compile/to-c.lisp, then cc + links liblizard.a,
#      and runs the resulting native binary,
#   3. asserts the two outputs are byte-for-byte identical.
#
# The compiled program reuses lizard's value representation, primitives and
# numeric tower, so equality is the strongest possible correctness signal:
# the native build must observably match the interpreter.
#
# Exit status is 0 only if every program matches.

set -u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT" || exit 2

LIZARD=build/lizard
DS=${DS_DIR:-/home/claude/ds}
export C_INCLUDE_PATH="$DS" LIBRARY_PATH="$DS" LD_LIBRARY_PATH="$DS"

if [ ! -x "$LIZARD" ]; then
  echo "error: $LIZARD not built (run make first)" >&2
  exit 2
fi
if [ ! -f build/liblizard.a ]; then
  echo "error: build/liblizard.a missing (run make first)" >&2
  exit 2
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

filter() { grep -vE "^Assumed|^=> $" || true; }

pass=0
fail=0
for scm in tests/compiler/*.scm; do
  name=$(basename "$scm" .scm)

  # (1) interpreted output
  "$LIZARD" "$scm" 2>/dev/null | filter > "$WORK/$name.interp"

  # (2) emit C: wrap the program as a quoted list of forms, run the compiler
  {
    printf '(load "lib/compile/to-c.lisp")\n(display (program->c (quote (\n'
    cat "$scm"
    printf '\n))))\n'
  } > "$WORK/$name.drv"
  "$LIZARD" "$WORK/$name.drv" 2>/dev/null | grep -vE "^Assumed|^=> $" > "$WORK/$name.c"

  # (3) compile + link + run the native binary
  if ! cc -std=c89 -I include -I src "$WORK/$name.c" build/liblizard.a \
        -lds -lgmp -lm -o "$WORK/$name.bin" 2> "$WORK/$name.cc"; then
    echo "FAIL $name (compilation error)"
    sed 's/^/    /' "$WORK/$name.cc" | head -8
    fail=$((fail + 1))
    continue
  fi
  "$WORK/$name.bin" 2>/dev/null > "$WORK/$name.compiled"

  # (4) compare
  if diff -u "$WORK/$name.interp" "$WORK/$name.compiled" > "$WORK/$name.diff" 2>&1; then
    echo "ok   $name"
    pass=$((pass + 1))
  else
    echo "FAIL $name (output mismatch)"
    sed 's/^/    /' "$WORK/$name.diff" | head -12
    fail=$((fail + 1))
  fi
done

echo "-----------------------------------------"
echo "compiler differential tests: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
