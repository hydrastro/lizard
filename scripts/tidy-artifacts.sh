#!/usr/bin/env bash
#
# tidy-artifacts.sh -- sweep the build and development debris out of the lizard
# repository: compiled objects and dependency files under build/, Python
# bytecode caches (__pycache__, *.pyc), coverage output (*.gcda/*.gcno/*.gcov),
# editor leftovers (*~, *.swp, .DS_Store), and obviously-empty stray files.
#
# It is deliberately conservative: it only matches well-known throwaway
# patterns, and any stray file that is NOT empty is reported for manual review
# rather than deleted (so a hand-written helper like time2.sh is never removed
# behind your back). Source files, the .lisp library, and the contents of docs/
# are never touched.
#
# SAFETY:
#   * dry-run by default -- prints what it would remove and changes nothing
#   * --apply            -- actually remove
#   * --archive DIR      -- move items into DIR/<timestamp>/ instead of deleting
#   * refuses to run unless the target really looks like the lizard repo
#
# Usage:
#   scripts/tidy-artifacts.sh                 # dry run from the repo root
#   scripts/tidy-artifacts.sh --apply         # do it
#   scripts/tidy-artifacts.sh --root /path/to/lizard --apply

set -eu

APPLY=0
ARCHIVE=""
ROOT=""

usage() { sed -n '2,40p' "$0" | sed 's/^# \{0,1\}//'; exit "${1:-0}"; }

while [ $# -gt 0 ]; do
  case "$1" in
    --apply)    APPLY=1 ;;
    --archive)  ARCHIVE="${2:?--archive needs a directory}"; shift ;;
    --root)     ROOT="${2:?--root needs a directory}"; shift ;;
    -h|--help)  usage 0 ;;
    *) echo "tidy-artifacts: unknown argument: $1" >&2; usage 1 ;;
  esac
  shift
done

# --- locate + verify the repo root (same fingerprint as tidy-structure.sh) ---
if [ -z "$ROOT" ]; then
  d="$(cd "$(dirname "$0")" && pwd)"
  while [ "$d" != "/" ]; do
    if [ -f "$d/Makefile" ] && [ -d "$d/src" ] && [ -f "$d/DESIGN.md" ]; then
      ROOT="$d"; break
    fi
    d="$(dirname "$d")"
  done
fi
[ -n "$ROOT" ] || { echo "tidy-artifacts: could not find the repo root; pass --root" >&2; exit 1; }
ROOT="$(cd "$ROOT" && pwd)"

fingerprint_ok=1
for marker in Makefile DESIGN.md src lib README.md; do
  [ -e "$ROOT/$marker" ] || fingerprint_ok=0
done
if [ "$fingerprint_ok" -ne 1 ]; then
  echo "tidy-artifacts: $ROOT does not look like the lizard repo (missing markers); refusing." >&2
  exit 1
fi

[ -n "$ARCHIVE" ] && ARCHIVE_DIR="$ARCHIVE/$(date +%Y%m%d-%H%M%S)"

echo "tidy-artifacts: root = $ROOT"
if [ "$APPLY" -eq 1 ]; then
  if [ -n "$ARCHIVE" ]; then echo "mode: APPLY (archiving into $ARCHIVE_DIR)"
  else echo "mode: APPLY (deleting)"; fi
else
  echo "mode: DRY RUN (nothing will change; pass --apply to act)"
fi
echo

total_bytes=0
count=0
review=0
HITS="$(mktemp "${TMPDIR:-/tmp}/tidy_hits.XXXXXX")"
trap 'rm -f "$HITS"' EXIT

size_of() {
  if [ -d "$1" ]; then du -sk -- "$1" 2>/dev/null | awk '{print $1*1024}'
  else wc -c < "$1" 2>/dev/null || echo 0; fi
}

remove_item() {  # $1 = absolute path, $2 = reason
  path="$1"; reason="$2"
  [ -e "$path" ] || return 0
  bytes="$(size_of "$path")"
  total_bytes=$((total_bytes + bytes))
  count=$((count + 1))
  rel="${path#$ROOT/}"
  printf '  %-50s  %9s b   %s\n' "$rel" "$bytes" "$reason"
  [ "$APPLY" -eq 1 ] || return 0
  if [ -n "$ARCHIVE" ]; then
    dest="$ARCHIVE_DIR/$rel"; mkdir -p "$(dirname "$dest")"; mv -- "$path" "$dest"
  else
    rm -rf -- "$path"
  fi
}

# run a find expression (passed as args) and feed each hit to remove_item.
sweep() {  # $1 = reason, rest = find predicates
  reason="$1"; shift
  # -prune version-control and our own archive dir out of the walk, NUL-safe.
  find "$ROOT" \
       \( -name .git -o -name .hg -o -name .tidy-trash \) -prune -o \
       "$@" -print0 > "$HITS" 2>/dev/null || true
  while IFS= read -r -d '' hit; do
    remove_item "$hit" "$reason"
  done < "$HITS"
}

# NOTE: the counters above are updated inside remove_item, but sweep() runs the
# loop in the current shell (we read from a temp file, not a pipe) so the totals
# persist. Patterns:

# 1) compiled objects and dependency files
sweep "compiled object"       -type f -name '*.o'
sweep "dependency file"       -type f -name '*.d'
sweep "static archive"        -type f -name 'liblizard.a'

# 2) Python bytecode caches
sweep "python bytecode cache" -type d -name '__pycache__'
sweep "python bytecode"       -type f -name '*.pyc'

# 3) coverage output
sweep "coverage data"         -type f \( -name '*.gcda' -o -name '*.gcno' -o -name '*.gcov' \)

# 4) editor / OS leftovers
sweep "editor backup"         -type f \( -name '*~' -o -name '*.swp' -o -name '*.swo' \)
sweep "OS metadata"           -type f -name '.DS_Store'

# 5) obviously-empty stray files at the repo root (e.g. an accidental `quit`).
#    Non-empty strays are listed for manual review, never auto-removed.
for f in "$ROOT"/*; do
  [ -f "$f" ] || continue
  base="$(basename "$f")"
  case "$base" in
    *.md|Makefile|flake.*|*.lock|*.nix|.gitignore|LICENSE*) continue ;;
  esac
  if [ ! -s "$f" ]; then
    remove_item "$f" "empty stray file"
  else
    # a non-empty top-level file that isn't an obvious project file
    case "$base" in
      *.sh|*.txt|*.log|*.tmp|*.bak)
        rel="${f#$ROOT/}"
        printf '  %-50s  %9s b   REVIEW (non-empty; left in place)\n' \
               "$rel" "$(wc -c < "$f")"
        review=$((review + 1)) ;;
    esac
  fi
done

echo
if [ "$count" -eq 0 ] && [ "$review" -eq 0 ]; then
  echo "tidy-artifacts: nothing to do -- no build debris found."
else
  human="$(awk -v b="$total_bytes" 'BEGIN{printf "%.1f MB", b/1048576}')"
  if [ "$APPLY" -eq 1 ]; then
    echo "tidy-artifacts: removed $count item(s), reclaimed ~$human."
  else
    echo "tidy-artifacts: $count item(s) would be removed, ~$human. Re-run with --apply."
  fi
  [ "$review" -gt 0 ] && echo "tidy-artifacts: $review file(s) flagged REVIEW were left untouched."
fi
