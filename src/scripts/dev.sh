#!/usr/bin/env bash
#
# scripts/dev.sh — developer workflow wrapper.
#
# Usage:
#   ./scripts/dev.sh build            # build only
#   ./scripts/dev.sh test             # build + run C/Lisp tests
#   ./scripts/dev.sh examples         # run every example, report failures
#   ./scripts/dev.sh check            # test + examples + benchmark
#   ./scripts/dev.sh bench            # run the benchmark example with timing
#   ./scripts/dev.sh stats            # rule and test counts
#   ./scripts/dev.sh doctor           # diagnose build / runtime / env issues
#   ./scripts/dev.sh clean            # delegate to clean.sh
#   ./scripts/dev.sh bundle [path]    # create zip; default /tmp/lizard-pro.zip
#   ./scripts/dev.sh release          # clean + check + bundle in one shot
#   ./scripts/dev.sh                  # this help
#
# Honest about what it does — no hidden state, no surprise side effects.
# Safe to run from anywhere — resolves project root from script location.

set -euo pipefail

# --- locate project root --------------------------------------------------
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
root=$(cd -- "$script_dir/.." &>/dev/null && pwd)
cd "$root"

# --- env (find lizard's deps from common locations) -----------------------
# Lizard depends on a 'ds' header library (from the lizard project's parent
# repo) and on GMP for bignums. We probe a few common install paths and
# prepend their include / lib to CPPFLAGS / LDFLAGS if found. The Makefile
# picks them up. If neither probe finds anything the user can still set
# CPPFLAGS / LDFLAGS explicitly before invoking this script.
# Always include lizard's own include/ and src/ so the build is robust to
# being invoked from anywhere.
export CPPFLAGS="-Iinclude -Isrc ${CPPFLAGS:-}"
DEP_CANDIDATES=(
  "/home/claude/ds-install"
  "/usr/local"
  "/opt/homebrew"
)
for c in "${DEP_CANDIDATES[@]}"; do
  if [[ -d "$c/include" ]]; then
    if [[ -f "$c/include/ds.h" || -f "$c/include/gmp.h" ]]; then
      export CPPFLAGS="-I$c/include ${CPPFLAGS}"
      export LDFLAGS="-L$c/lib -Wl,-rpath,$c/lib ${LDFLAGS:-}"
    fi
  fi
done

# --- helpers --------------------------------------------------------------
say() { printf '\033[1;34m▸ %s\033[0m\n' "$*"; }
ok()  { printf '\033[1;32m✓ %s\033[0m\n' "$*"; }
err() { printf '\033[1;31m✗ %s\033[0m\n' "$*" >&2; }

cmd_build() {
  say "Building lizard"
  if ! make >/dev/null 2>build.log; then
    err "Build failed. See build.log:"
    tail -40 build.log >&2
    return 1
  fi
  rm -f build.log
  ok "Build succeeded"
}

cmd_test() {
  cmd_build
  say "Running tests"
  if ! make test 2>&1 | tail -40; then
    err "Tests failed"
    return 1
  fi
  ok "Tests passed"
}

cmd_examples() {
  cmd_build
  say "Running examples"
  local DIM=$'\033[2m'
  local OFF=$'\033[0m'
  local failures=0 total=0
  local first_failure_log=""
  for ex in examples/*.lisp; do
    total=$((total + 1))
    local log
    log=$(mktemp)
    if ! ./build/lizard < "$ex" > "$log" 2>&1; then
      local rc=$?
      err "FAIL: $ex (exit $rc)"
      failures=$((failures + 1))
      if [[ -z "$first_failure_log" ]]; then
        first_failure_log=$log
        echo "${DIM}  --- first failure output ---${OFF}"
        head -20 "$log" | sed 's/^/    /'
        if [[ $(wc -l < "$log") -gt 20 ]]; then
          echo "${DIM}    (output truncated; full log: $log)${OFF}"
        else
          rm -f "$log"
          first_failure_log=""
        fi
        echo "${DIM}  --- end of first failure output ---${OFF}"
      else
        rm -f "$log"
      fi
    else
      rm -f "$log"
    fi
  done
  if [[ $failures -gt 0 ]]; then
    err "$failures of $total examples failed"
    return 1
  fi
  ok "All $total examples ran cleanly"
}

cmd_bench() {
  cmd_build
  say "Running benchmark"
  if [[ ! -f examples/22-benchmarks.lisp ]]; then
    err "Benchmark example not found"
    return 1
  fi
  time ./build/lizard < examples/22-benchmarks.lisp > /dev/null
  ok "Benchmark complete"
}

cmd_check() {
  cmd_test
  cmd_examples
  cmd_bench
  ok "Full check passed"
}

cmd_stats() {
  cmd_build
  say "Lizard at-a-glance"
  # Source size
  local c_lines h_lines t_lines lsp_lines
  c_lines=$(wc -l src/*.c 2>/dev/null | tail -1 | awk '{print $1}' || echo 0)
  h_lines=$(wc -l include/*.h src/*.h 2>/dev/null | tail -1 | awk '{print $1}' || echo 0)
  t_lines=$(wc -l tests/*.c 2>/dev/null | tail -1 | awk '{print $1}' || echo 0)
  lsp_lines=$(wc -l examples/*.lisp 2>/dev/null | tail -1 | awk '{print $1}' || echo 0)
  echo "  C source:        ${c_lines} lines"
  echo "  Headers:         ${h_lines} lines"
  echo "  C tests:         ${t_lines} lines"
  echo "  Lisp examples:   ${lsp_lines} lines"
  # Tests
  local c_tests lisp_tests
  c_tests=$(ls tests/*_test.c 2>/dev/null | wc -l)
  lisp_tests=$(ls tests/*.lisp 2>/dev/null | wc -l || echo 0)
  echo "  C test programs: ${c_tests}"
  echo "  Lisp tests:      ${lisp_tests}"
  echo "  Example files:   $(ls examples/*.lisp 2>/dev/null | wc -l)"
  # Rules — grep for flag_find_or_create lines
  local rules
  rules=$(grep -c '^[[:space:]]*flag_find_or_create' src/tt_equality.c || echo 0)
  echo "  Reduction rules: ${rules} (flag count)"
  # Typing rules — grep for case labels in tt_check.c infer/check paths
  local infer_rules
  infer_rules=$(grep -c '^[[:space:]]*case AST_TT_' src/tt_check.c || echo 0)
  echo "  Typing cases:    ${infer_rules}"
}

cmd_clean() {
  exec "$script_dir/clean.sh" "$@"
}

cmd_doctor() {
  say "Diagnosing lizard environment"
  echo

  local SECT=$'\033[1;34m'
  local OFF=$'\033[0m'
  local DIM=$'\033[2m'
  local WARN=$'\033[1;33m'
  local FAIL=$'\033[1;31m'

  # 1. Working directory + tooling
  echo "${SECT}[environment]${OFF}"
  echo "  pwd:    $root"
  echo "  shell:  ${BASH_VERSION:-unknown bash}"
  echo "  uname:  $(uname -srm 2>/dev/null || echo unknown)"
  if command -v cc >/dev/null; then
    echo "  cc:     $(command -v cc) ($(cc --version 2>/dev/null | head -1))"
  else
    echo "  ${FAIL}cc not found${OFF}"
  fi
  if command -v make >/dev/null; then
    echo "  make:   $(command -v make)"
  else
    echo "  ${FAIL}make not found${OFF}"
  fi
  if command -v zip >/dev/null; then
    echo "  zip:    $(command -v zip)"
  else
    echo "  ${WARN}zip not found (bundle won't work)${OFF}"
  fi
  echo

  # 2. CPPFLAGS/LDFLAGS as resolved
  echo "${SECT}[build flags]${OFF}"
  echo "  CPPFLAGS=${CPPFLAGS:-<empty>}"
  echo "  LDFLAGS=${LDFLAGS:-<empty>}"
  echo

  # 3. Source tree sanity
  echo "${SECT}[source tree]${OFF}"
  for d in src include tests examples scripts; do
    if [[ -d "$d" ]]; then
      echo "  $d/: $(find "$d" -maxdepth 1 -type f | wc -l) files"
    else
      echo "  ${FAIL}$d/ MISSING${OFF}"
    fi
  done
  for f in Makefile README.md examples/prelude.lisp; do
    if [[ -f "$f" ]]; then
      echo "  $f: present"
    else
      echo "  ${FAIL}$f MISSING${OFF}"
    fi
  done
  echo

  # 4. Try to build
  echo "${SECT}[build]${OFF}"
  if make >/tmp/lizard-doctor.log 2>&1; then
    ok "build succeeded"
    rm -f /tmp/lizard-doctor.log
  else
    err "build failed"
    echo "  last lines of build output:"
    tail -10 /tmp/lizard-doctor.log | sed 's/^/    /'
    echo "  full log at /tmp/lizard-doctor.log"
    return 1
  fi
  echo

  # 5. Binary diagnostics
  echo "${SECT}[binary]${OFF}"
  if [[ -x build/lizard ]]; then
    local sz
    if sz=$(stat -c %s build/lizard 2>/dev/null); then :; else sz=$(stat -f %z build/lizard 2>/dev/null || echo unknown); fi
    echo "  build/lizard: present, $sz bytes"
    if command -v ldd >/dev/null; then
      echo "  dynamic libraries:"
      ldd build/lizard 2>&1 | sed 's/^/    /' | head -10
      if ldd build/lizard 2>&1 | grep -q "not found"; then
        echo "  ${FAIL}WARNING: missing shared library — this causes runtime failure${OFF}"
      fi
    elif command -v otool >/dev/null; then
      echo "  dynamic libraries (otool -L):"
      otool -L build/lizard 2>&1 | sed 's/^/    /' | head -10
    fi
  else
    err "build/lizard is missing or not executable"
    return 1
  fi
  echo

  # 6. Smoke test — does the binary actually run?
  echo "${SECT}[smoke test]${OFF}"
  local out exit_code
  out=$(echo '(+ 1 2)' | ./build/lizard 2>&1) || true
  exit_code=$?
  echo "  '(+ 1 2)' → '$out' (exit $exit_code)"
  if [[ $exit_code -ne 0 ]]; then
    err "binary exits non-zero on trivial input"
    echo "  This explains why dev.sh examples reports all failures."
    return 1
  fi

  # Try one full example
  if [[ -f examples/01-basics.lisp ]]; then
    out=$(./build/lizard < examples/01-basics.lisp 2>&1 | head -5)
    exit_code=$?
    echo "  examples/01-basics.lisp first 5 lines:"
    echo "$out" | sed 's/^/    /'
    echo "  exit code: $exit_code"
    if [[ $exit_code -ne 0 ]]; then
      err "example 01 exits non-zero"
      return 1
    fi
  fi
  echo
  ok "Doctor: lizard looks healthy"
}

cmd_bundle() {
  local target=${1:-/tmp/lizard-pro.zip}
  cmd_build
  "$script_dir/clean.sh" >/dev/null
  say "Bundling to $target"
  rm -f "$target"
  if ! zip -rq "$target" . -x "build/*" "*.log"; then
    err "Bundling failed"
    return 1
  fi
  local size
  size=$(ls -lh "$target" | awk '{print $5}')
  ok "Bundle: $target ($size)"
}

cmd_release() {
  say "Release workflow: clean + check + bundle"
  "$script_dir/clean.sh"
  cmd_check
  cmd_bundle "${1:-/tmp/lizard-pro.zip}"
  ok "Release complete"
}

cmd_help() {
  sed -n '2,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

# --- dispatch -------------------------------------------------------------
sub=${1:-help}
shift || true
case "$sub" in
  build)    cmd_build "$@" ;;
  test)     cmd_test "$@" ;;
  examples) cmd_examples "$@" ;;
  check)    cmd_check "$@" ;;
  bench)    cmd_bench "$@" ;;
  stats)    cmd_stats "$@" ;;
  doctor)   cmd_doctor "$@" ;;
  clean)    cmd_clean "$@" ;;
  bundle)   cmd_bundle "$@" ;;
  release)  cmd_release "$@" ;;
  help|-h|--help) cmd_help ;;
  *)
    err "Unknown subcommand: $sub"
    cmd_help
    exit 2
    ;;
esac
