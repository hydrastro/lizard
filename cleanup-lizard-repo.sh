#!/usr/bin/env bash
# cleanup-lizard-repo.sh — clean generated files from the Lizard repository.
#
# Default mode is a dry run. Use --apply to actually remove files.
# This script is intentionally conservative: it removes build/profiler/editor
# artefacts, and only prunes dynsys/TPCAS leftovers when --prune-dynsys is set.
#
# Usage:
#   ./cleanup-lizard-repo.sh                 # dry run
#   ./cleanup-lizard-repo.sh --apply         # remove safe generated artefacts
#   ./cleanup-lizard-repo.sh --apply --prune-dynsys
#   ./cleanup-lizard-repo.sh --check         # fail if suspicious dirty files exist

set -euo pipefail

apply=0
check=0
prune_dynsys=0

usage() {
  sed -n '1,18p' "$0" | sed 's/^# \{0,1\}//'
}

for arg in "$@"; do
  case "$arg" in
    --apply) apply=1 ;;
    --check) check=1 ;;
    --prune-dynsys) prune_dynsys=1 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "unknown option: $arg" >&2; usage >&2; exit 2 ;;
  esac
done

# Resolve repository root. Prefer git when available, otherwise use the
# current working directory. This lets you keep the script outside the repo
# and run it as: /path/to/cleanup-lizard-repo.sh --apply.
if root=$(git rev-parse --show-toplevel 2>/dev/null); then
  :
else
  root=$(pwd)
fi
cd "$root"

found=0
removed=0
warned=0

say() { printf '%s\n' "$*"; }
warn() { warned=$((warned + 1)); printf 'WARN: %s\n' "$*" >&2; }

remove_path() {
  local p=$1
  if [ -e "$p" ] || [ -L "$p" ]; then
    found=$((found + 1))
    if [ "$apply" -eq 1 ]; then
      rm -rf -- "$p"
      removed=$((removed + 1))
      printf 'removed: %s\n' "$p"
    else
      printf 'would remove: %s\n' "$p"
    fi
  fi
}

remove_glob() {
  # Expands globs safely even when no file matches.
  local pattern=$1
  local oldnullglob
  oldnullglob=$(shopt -p nullglob || true)
  shopt -s nullglob
  local p
  for p in $pattern; do
    remove_path "$p"
  done
  eval "$oldnullglob" 2>/dev/null || true
}

say "Repository: $root"
if [ "$apply" -eq 0 ]; then
  say "Mode: dry run; pass --apply to remove."
else
  say "Mode: applying removals."
fi

# Safe generated files and directories.
remove_path build
remove_path build.log
remove_path lizard
remove_path liblizard.a
remove_path liblizard.so
remove_path result
remove_glob 'result-*'
remove_glob '*.zip'
remove_glob '*.tar'
remove_glob '*.tar.gz'
remove_glob '*.tgz'
remove_glob '*.o'
remove_glob '*.a'
remove_glob '*.so'
remove_glob '*.d'
remove_glob 'src/*.o'
remove_glob 'src/*.a'
remove_glob 'src/*.so'
remove_glob 'tests/*.o'
remove_glob 'test/*.o'

# Profilers, sanitizers, debuggers, coverage.
remove_path gmon.out
remove_path perf.data
remove_path perf.data.old
remove_glob 'callgrind.out.*'
remove_glob 'cachegrind.out.*'
remove_glob 'massif.out.*'
remove_glob 'vgcore.*'
remove_glob 'core'
remove_glob 'core.*'
remove_glob '*.gcda'
remove_glob '*.gcno'
remove_glob '*.gcov'

# Editor/OS noise.
remove_path .DS_Store
remove_path imgui.ini
remove_glob '*~'
remove_glob '#*#'
remove_glob '.#*'
remove_glob '*.swp'
remove_glob '*.swo'
remove_glob 'src/*~'
remove_glob 'include/*~'
remove_glob 'tests/*~'
remove_glob 'examples/*~'

# Nix symlinks/caches. Keep flake.lock: it should usually be committed.
remove_path result
remove_glob 'result-*'
remove_path .direnv
remove_path .envrc.local

# Detect wrong-project leftovers. These are not removed unless explicitly asked.
dynsys_paths=(
  'src/dynsys.cpp'
  'src/expr_ir.cpp'
  'src/expr_ir.h'
  'test/ir_smoke.cpp'
  'vendor/tpcas'
  'examples/damped_pendulum.dyn'
  'examples/four_dimensional_demo.dyn'
  'examples/henon.dyn'
  'examples/lorenz.dyn'
  'examples/lotka_volterra.dyn'
  'examples/rossler.dyn'
  'examples/saddle_separatrix.dyn'
  'examples/thomas.dyn'
  'examples/van_der_pol.dyn'
)

present_dynsys=0
for p in "${dynsys_paths[@]}"; do
  if [ -e "$p" ] || [ -L "$p" ]; then
    present_dynsys=$((present_dynsys + 1))
  fi
done

if [ "$present_dynsys" -gt 0 ]; then
  warn "found $present_dynsys dynsys/TPCAS-looking path(s). These look unrelated to Lizard."
  if [ "$prune_dynsys" -eq 1 ]; then
    for p in "${dynsys_paths[@]}"; do
      remove_path "$p"
    done
  else
    say "Run with --prune-dynsys if these are accidental leftovers."
  fi
fi

# Detect overwritten build metadata.
if [ -f Makefile ] && grep -qiE 'dynsys|Dear ImGui|GLFW|TPCAS' Makefile; then
  warn "top-level Makefile appears to be for dynsys, not Lizard. Restore the Lizard Makefile before pushing."
fi
if [ -f flake.nix ] && grep -qiE 'dynsys|Dear ImGui|GLFW|imgui|glew|cglm' flake.nix; then
  warn "flake.nix appears to be for dynsys/ImGui, not Lizard. Restore the Lizard flake before pushing."
fi
if [ -f CHANGES.md ] && grep -qiE 'dynsys|MatCont|Lorenz|RK4|Dear ImGui' CHANGES.md; then
  warn "CHANGES.md appears to describe dynsys, not Lizard. Remove or rewrite it."
fi
if [ -f COMPARISON.md ] && grep -qiE 'dynsys|pplane|Dear ImGui|TPCAS' COMPARISON.md; then
  warn "COMPARISON.md appears to describe dynsys, not Lizard. Remove or rewrite it."
fi

# Git status hint.
if command -v git >/dev/null 2>&1 && git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  say ""
  say "Git status after cleanup/dry-run:"
  git status --short
fi

if [ "$check" -eq 1 ]; then
  if [ "$found" -gt 0 ] || [ "$warned" -gt 0 ]; then
    echo "cleanup check failed: $found removable path(s), $warned warning(s)." >&2
    exit 1
  fi
fi

say ""
say "Summary: candidates=$found removed=$removed warnings=$warned"
