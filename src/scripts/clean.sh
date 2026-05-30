#!/usr/bin/env bash
#
# scripts/clean.sh — remove build artefacts and stray junk from the
# lizard project. Handles the common kinds of dirt that accumulate in
# a working directory: build outputs, editor backups, debugger leftovers,
# misplaced archives, and (with --suspicious) files at top level that
# look like typos or accidents.
#
# Usage:
#   ./scripts/clean.sh                 # standard cleanup (safe)
#   ./scripts/clean.sh --dry           # print what would be removed
#   ./scripts/clean.sh --deep          # also wipe Nix result/.direnv/cache
#   ./scripts/clean.sh --nuke-lock     # with --deep, also remove flake.lock
#   ./scripts/clean.sh --suspicious    # also list files at top level
#                                      # that look out of place (NOT
#                                      # removed automatically — only
#                                      # reported, so you can review)
#   ./scripts/clean.sh --inspect       # report on dirt without removing
#                                      # anything (alias for --dry --suspicious)
#   ./scripts/clean.sh --check         # exit nonzero if any dirt found
#                                      # (CI-friendly; removes nothing,
#                                      # implies --dry --suspicious)
#   ./scripts/clean.sh --help          # this help
#
# Multiple flags can be combined. The --dry flag overrides removal
# everywhere; everything else just becomes a report.
#
# Safe to run from anywhere — resolves the project root from the
# script's own location.

set -euo pipefail

# --- locate project root --------------------------------------------------
script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
root=$(cd -- "$script_dir/.." &>/dev/null && pwd)

# --- options --------------------------------------------------------------
dry=0
deep=0
nuke_lock=0
suspicious=0
check=0
suspicious_count=0
for arg in "$@"; do
  case "$arg" in
    --dry|-n)       dry=1 ;;
    --deep)         deep=1 ;;
    --nuke-lock)    nuke_lock=1 ;;
    --suspicious)   suspicious=1 ;;
    --inspect)      dry=1; suspicious=1 ;;
    --check)        dry=1; suspicious=1; check=1 ;;
    -h|--help)
      sed -n '2,/^$/p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *)
      echo "unknown option: $arg" >&2
      exit 2
      ;;
  esac
done

# --- color helpers --------------------------------------------------------
if [[ -t 1 ]]; then
  c_section=$'\033[1;34m'
  c_remove=$'\033[1;31m'
  c_keep=$'\033[1;33m'
  c_ok=$'\033[1;32m'
  c_dim=$'\033[2m'
  c_off=$'\033[0m'
else
  c_section= c_remove= c_keep= c_ok= c_dim= c_off=
fi

# --- removal helper -------------------------------------------------------
removed_count=0
would_remove_count=0
nuke() {
  local target=$1
  if compgen -G "$target" >/dev/null 2>&1; then
    if [[ $dry -eq 1 ]]; then
      printf '  %swould remove:%s %s\n' "$c_keep" "$c_off" "$target"
      would_remove_count=$((would_remove_count + 1))
    else
      printf '  %sremoving:%s     %s\n' "$c_remove" "$c_off" "$target"
      # shellcheck disable=SC2086
      rm -rf -- $target
      removed_count=$((removed_count + 1))
    fi
  fi
}

section() {
  printf '%s[%s]%s\n' "$c_section" "$1" "$c_off"
}

echo "Cleaning ${root}"
if [[ $dry -eq 1 ]]; then
  echo "${c_dim}(dry run — nothing will be removed)${c_off}"
fi
cd "$root"

# --- build artefacts ------------------------------------------------------
section "build artefacts"
nuke "build"
nuke "src/*.o"
nuke "src/*.a"
nuke "src/*.so"
nuke "src/*.gch"
nuke "src/*.gcda"
nuke "src/*.gcno"
nuke "src/*.lo"
nuke "tests/*.o"
nuke "tests/*.gch"
nuke "build/*.d"
nuke "lizard"
nuke "liblizard.a"
nuke "liblizard.so"
nuke "*.o"
nuke "a.out"
nuke "*.exe"
nuke "*.obj"

# --- profiler / debugger leftovers ----------------------------------------
section "profiler / debugger"
nuke "gmon.out"
nuke "perf.data"
nuke "perf.data.old"
nuke "callgrind.out.*"
nuke "cachegrind.out.*"
nuke "massif.out.*"
nuke "vgcore.*"
nuke "core"
nuke "core.*"
nuke "*.dSYM"
nuke "src/*.dSYM"

# --- editor / OS junk -----------------------------------------------------
section "editor / OS"
nuke "*~"
nuke "src/*~"
nuke "include/*~"
nuke "tests/*~"
nuke "examples/*~"
nuke "scripts/*~"
nuke "*.swp"
nuke "src/*.swp"
nuke "include/*.swp"
nuke "tests/*.swp"
nuke ".#*"
nuke "src/.#*"
nuke "#*#"
nuke "src/#*#"
nuke ".DS_Store"
nuke "src/.DS_Store"
nuke "include/.DS_Store"
nuke "tests/.DS_Store"
nuke "examples/.DS_Store"
nuke "Thumbs.db"

# --- logs / crash dumps ---------------------------------------------------
section "logs / crash dumps"
nuke "*.log"
nuke "build.log"
nuke "compile_errors.log"
nuke "test.log"
nuke "crash.txt"
nuke "trace.txt"

# --- misplaced archives ---------------------------------------------------
section "misplaced archives"
nuke "lizard-pro.zip"
nuke "lizard.zip"
nuke "lizard.tar.gz"
nuke "lizard.tar.bz2"
nuke "src.zip"
nuke "examples/*.zip"
nuke "examples/*.tar.gz"
nuke "src.tar.gz"
nuke "*.zip.bak"
nuke "*.tar.gz.bak"

# --- tool backups ---------------------------------------------------------
section "tool backups"
nuke "*.bak"
nuke "src/*.bak"
nuke "include/*.bak"
nuke "tests/*.bak"
nuke "examples/*.bak"
nuke "scripts/*.bak"
nuke "*.orig"
nuke "src/*.orig"
nuke "*.rej"
nuke "src/*.rej"
nuke "examples/*.patch"

# --- deep cleanup (opt-in) ------------------------------------------------
if [[ $deep -eq 1 ]]; then
  section "deep cleanup"
  nuke "result"
  nuke "result-*"
  if [[ $nuke_lock -eq 1 ]]; then
    nuke "flake.lock"
  fi
  nuke ".direnv"
  nuke ".envrc.cache"
  nuke "compile_commands.json"
  nuke ".cache"
fi

# --- suspicious files (REPORT ONLY) ---------------------------------------
if [[ $suspicious -eq 1 ]]; then
  section "suspicious files (review manually)"
  echo "${c_dim}  These are NOT removed automatically. They're listed so${c_off}"
  echo "${c_dim}  you can review and decide.${c_off}"

  expected_re='^(README\.md|CHANGELOG\.md|DESIGN\.md|LIMITATIONS\.md|Makefile|flake\.nix|flake\.lock|\.gitignore|\.gitattributes|build|src|include|tests|examples|scripts|docs|lib|prelude\.lisp|LICENSE.*)$'

  found_any=0
  # Iterate over normal entries first, then dotfiles (safely).
  shopt -s nullglob
  for entry in * .[!.]*; do
    [[ -e "$entry" ]] || continue
    case "$entry" in
      \.|\.\.) continue ;;
    esac
    if ! echo "$entry" | grep -Eq "$expected_re"; then
      printf '  %ssuspicious:%s   %s\n' "$c_keep" "$c_off" "$entry"
      found_any=1
      suspicious_count=$((suspicious_count + 1))
    fi
  done
  shopt -u nullglob

  # Typo directories: digits appended to expected dir names.
  for d in src include tests examples scripts; do
    for variant in "${d}2" "${d}3" "${d}_old" "${d}-old" "${d}.bak" "${d}.old"; do
      if [[ -e "$variant" ]]; then
        printf '  %slikely typo of %s:%s %s\n' "$c_keep" "$d" "$c_off" "$variant"
        found_any=1
        suspicious_count=$((suspicious_count + 1))
      fi
    done
  done

  # Source files at root (should be in src/ or include/).
  for f in *.c *.h; do
    [[ -e "$f" ]] || continue
    printf '  %ssource file at root:%s %s %s(expected in src/ or include/)%s\n' \
      "$c_keep" "$c_off" "$f" "$c_dim" "$c_off"
    found_any=1
    suspicious_count=$((suspicious_count + 1))
  done

  # Known wrong-project leftovers that should never live under examples/.
  for f in examples/Makefile examples/flake.nix examples/flake.lock examples/dynsys-*; do
    if [[ -e "$f" ]]; then
      printf '  %swrong-project leftover:%s %s\n' "$c_keep" "$c_off" "$f"
      found_any=1
      suspicious_count=$((suspicious_count + 1))
    fi
  done

  if [[ $found_any -eq 0 ]]; then
    echo "${c_dim}  (nothing suspicious found)${c_off}"
  fi
fi

# --- summary --------------------------------------------------------------
echo
if [[ $check -eq 1 ]]; then
  # CI mode: fail if any dirt detected.
  if [[ $would_remove_count -gt 0 || $suspicious_count -gt 0 ]]; then
    echo "${c_remove}CHECK FAILED${c_off}: ${would_remove_count} removable pattern(s), ${suspicious_count} suspicious entrie(s)."
    echo "${c_dim}Run './scripts/clean.sh' to remove generated dirt; review suspicious files manually.${c_off}"
    exit 1
  else
    echo "${c_ok}CHECK PASSED${c_off}: tree is clean."
    exit 0
  fi
fi
if [[ $dry -eq 1 ]]; then
  echo "${c_ok}Dry run complete${c_off} — would have removed ${would_remove_count} pattern(s)."
  echo "${c_dim}Run without --dry to actually remove.${c_off}"
else
  echo "${c_ok}Done${c_off} — removed ${removed_count} pattern(s)."
fi
