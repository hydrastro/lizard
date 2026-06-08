#!/usr/bin/env bash
#
# tidy-structure.sh -- remove the structural duplication that crept into the
# lizard repository: a nested copy of the C sources at src/src/, a whole second
# project tree duplicated inside src/ (its own lib/, tests/, docs/, Makefile,
# flake.*, ...), and a redundant full-project snapshot lizard.zip at the root.
#
# The canonical layout is:
#     <root>/src/        -- C sources only (*.c, *.h)
#     <root>/lib/        -- the .lisp library
#     <root>/tests/ docs/ examples/ include/ scripts/ build/
#     <root>/Makefile README.md DESIGN.md flake.nix ...
#
# Anything inside src/ that is NOT a .c/.h file and that also exists at the repo
# root is an accidental duplicate; this script removes those copies (the live
# ones at the root are kept). It NEVER deletes anything at the repo root except
# the lizard.zip snapshot.
#
# SAFETY:
#   * dry-run by default -- prints what it would remove and changes nothing
#   * --apply            -- actually remove
#   * --archive DIR      -- move items into DIR/<timestamp>/ instead of deleting
#   * refuses to run unless the target really looks like the lizard repo
#
# Usage:
#   scripts/tidy-structure.sh                 # dry run from the repo root
#   scripts/tidy-structure.sh --apply         # do it
#   scripts/tidy-structure.sh --archive .trash --apply
#   scripts/tidy-structure.sh --root /path/to/lizard --apply

set -eu

APPLY=0
ARCHIVE=""
ROOT=""

usage() { sed -n '2,40p' "$0" | sed 's/^# \{0,1\}//'; exit "${1:-0}"; }

while [ $# -gt 0 ]; do
  case "$1" in
    --apply)      APPLY=1 ;;
    --archive)    ARCHIVE="${2:?--archive needs a directory}"; shift ;;
    --root)       ROOT="${2:?--root needs a directory}"; shift ;;
    -h|--help)    usage 0 ;;
    *) echo "tidy-structure: unknown argument: $1" >&2; usage 1 ;;
  esac
  shift
done

# --- locate the repo root ----------------------------------------------------
if [ -z "$ROOT" ]; then
  # walk up from the script's directory looking for the lizard fingerprint
  d="$(cd "$(dirname "$0")" && pwd)"
  while [ "$d" != "/" ]; do
    if [ -f "$d/Makefile" ] && [ -d "$d/src" ] && [ -f "$d/DESIGN.md" ]; then
      ROOT="$d"; break
    fi
    d="$(dirname "$d")"
  done
fi
[ -n "$ROOT" ] || { echo "tidy-structure: could not find the repo root; pass --root" >&2; exit 1; }
ROOT="$(cd "$ROOT" && pwd)"

# --- fingerprint guard: refuse to operate on a non-lizard directory ----------
fingerprint_ok=1
for marker in Makefile DESIGN.md src lib README.md; do
  [ -e "$ROOT/$marker" ] || fingerprint_ok=0
done
if [ "$fingerprint_ok" -ne 1 ]; then
  echo "tidy-structure: $ROOT does not look like the lizard repo (missing markers); refusing." >&2
  exit 1
fi

if [ -n "$ARCHIVE" ]; then
  ARCHIVE_DIR="$ARCHIVE/$(date +%Y%m%d-%H%M%S)"
fi

echo "tidy-structure: root = $ROOT"
if [ "$APPLY" -eq 1 ]; then
  if [ -n "$ARCHIVE" ]; then echo "mode: APPLY (archiving into $ARCHIVE_DIR)"
  else echo "mode: APPLY (deleting)"; fi
else
  echo "mode: DRY RUN (nothing will change; pass --apply to act)"
fi
echo

total_bytes=0
count=0

# portable byte size of a file or directory tree
size_of() {
  if [ -d "$1" ]; then du -sk -- "$1" 2>/dev/null | awk '{print $1*1024}'
  else wc -c < "$1" 2>/dev/null || echo 0; fi
}

remove_item() {  # $1 = absolute path, $2 = human reason
  path="$1"; reason="$2"
  [ -e "$path" ] || return 0
  bytes="$(size_of "$path")"
  total_bytes=$((total_bytes + bytes))
  count=$((count + 1))
  rel="${path#$ROOT/}"
  printf '  %-44s  %10s bytes   %s\n' "$rel" "$bytes" "$reason"
  [ "$APPLY" -eq 1 ] || return 0
  if [ -n "$ARCHIVE" ]; then
    dest="$ARCHIVE_DIR/$rel"
    mkdir -p "$(dirname "$dest")"
    mv -- "$path" "$dest"
  else
    rm -rf -- "$path"
  fi
}

# 1) nested copy of the C sources: src/src/
if [ -d "$ROOT/src/src" ]; then
  remove_item "$ROOT/src/src" "nested duplicate of the source dir"
fi

# 2) duplicated project tree inside src/ : remove each entry that also exists at
#    the repo root (the canonical copy stays).  Only .c/.h belong in src/.
for entry in "$ROOT"/src/*; do
  [ -e "$entry" ] || continue
  base="$(basename "$entry")"
  case "$base" in
    *.c|*.h) continue ;;            # genuine sources -- keep
    src)     continue ;;            # the nested source dir -- handled above
  esac
  # keep our own standalone demo driver binary out of it: it's handled by
  # tidy-artifacts.sh, not here.
  if [ -e "$ROOT/$base" ]; then
    remove_item "$entry" "duplicate of repo-root /$base"
  fi
done

# 3) the redundant full-project snapshot at the root
if [ -f "$ROOT/lizard.zip" ]; then
  remove_item "$ROOT/lizard.zip" "redundant full-project snapshot"
fi

echo
if [ "$count" -eq 0 ]; then
  echo "tidy-structure: nothing to do -- the tree is already clean."
else
  human="$(awk -v b="$total_bytes" 'BEGIN{printf "%.1f MB", b/1048576}')"
  if [ "$APPLY" -eq 1 ]; then
    echo "tidy-structure: removed $count item(s), reclaimed ~$human."
  else
    echo "tidy-structure: $count item(s) would be removed, ~$human. Re-run with --apply."
  fi
fi
