#!/bin/sh
# Static integrity checker for examples/manifest.sexp.
EXAMPLE_DIR="examples"
MANIFEST="$EXAMPLE_DIR/manifest.sexp"
PRINT_SUGGESTIONS=0

while [ $# -gt 0 ]; do
  case "$1" in
    --suggest) PRINT_SUGGESTIONS=1 ;;
    --help|-h) echo "usage: $0 [--suggest]"; exit 0 ;;
    *) echo "check-example-manifest: unknown option: $1" >&2; exit 2 ;;
  esac
  shift
done

if [ ! -d "$EXAMPLE_DIR" ] || [ ! -f "$MANIFEST" ]; then
  echo "check-example-manifest: missing examples directory or manifest" >&2
  exit 2
fi

tmp_names="${TMPDIR:-/tmp}/lz_manifest_names.$$"
tmp_dups="${TMPDIR:-/tmp}/lz_manifest_dups.$$"
tmp_files="${TMPDIR:-/tmp}/lz_manifest_files.$$"
tmp_expect="${TMPDIR:-/tmp}/lz_manifest_expect.$$"
trap 'rm -f "$tmp_names" "$tmp_dups" "$tmp_files" "$tmp_expect"' EXIT HUP INT TERM

grep -E '^[[:space:]]*\(example[[:space:]]+"[^"]+\.lisp"[[:space:]]+:expect[[:space:]]+[a-z]+[[:space:]]*\)' "$MANIFEST" \
  | sed -n 's/^[[:space:]]*(example[[:space:]]*"\([^"]*\.lisp\)".*/\1/p' \
  | sort > "$tmp_names"

grep -E '^[[:space:]]*\(example[[:space:]]+"[^"]+\.lisp"[[:space:]]+:expect[[:space:]]+[a-z]+[[:space:]]*\)' "$MANIFEST" \
  | sed -n 's/.*:expect[[:space:]]*\([a-z][a-z]*\).*/\1/p' \
  | sort > "$tmp_expect"

find "$EXAMPLE_DIR" -maxdepth 1 -type f -name '*.lisp' -exec basename {} \; | sort > "$tmp_files"
failures=0; unlisted=0; stale=0; duplicates=0; bad_expect=0

uniq -d "$tmp_names" > "$tmp_dups"
while IFS= read -r name; do
  [ -n "$name" ] || continue
  echo "DUPLICATE-MANIFEST-ENTRY  $name"
  duplicates=$((duplicates + 1)); failures=$((failures + 1))
done < "$tmp_dups"

while IFS= read -r file; do
  [ -n "$file" ] || continue
  if ! grep -Fx "$file" "$tmp_names" >/dev/null 2>&1; then
    echo "MISSING-FROM-MANIFEST  $file"
    [ "$PRINT_SUGGESTIONS" -eq 1 ] && echo "  suggested: (example \"$file\" :expect experimental)"
    unlisted=$((unlisted + 1)); failures=$((failures + 1))
  fi
done < "$tmp_files"

while IFS= read -r name; do
  [ -n "$name" ] || continue
  if [ ! -f "$EXAMPLE_DIR/$name" ]; then
    echo "MANIFEST-ENTRY-NO-FILE  $name"
    stale=$((stale + 1)); failures=$((failures + 1))
  fi
done < "$tmp_names"

while IFS= read -r expect; do
  [ -n "$expect" ] || continue
  case "$expect" in pass|error|experimental) ;; *) echo "UNKNOWN-EXPECT  $expect"; bad_expect=$((bad_expect + 1)); failures=$((failures + 1));; esac
done < "$tmp_expect"

echo ""
echo "Manifest audit summary:"
echo "  duplicate entries:     $duplicates"
echo "  unlisted files:        $unlisted"
echo "  entries without files: $stale"
echo "  unknown expectations:  $bad_expect"

if [ "$failures" -ne 0 ]; then
  echo ""
  echo "CHECK FAILED: $failures manifest issue(s)."
  exit 1
fi

echo "OK — example manifest is complete and well-formed."
exit 0
