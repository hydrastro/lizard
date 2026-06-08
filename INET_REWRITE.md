# lizard → interaction-net engine (INET_REWRITE)

This bundle is the first concrete stage of turning lizard into an HVM/Bend-style
interaction-net machine, plus the two repo-cleanup scripts. Files are laid out at
repo-relative paths, so unzipping this archive at the repository root drops each
file into the directory it belongs in. Nothing here overwrites an existing lizard
file — every path is new (this orientation file is `INET_REWRITE.md`, not
`README.md`, precisely so it leaves your `README.md` alone).

## What's here

```
src/ic.h                 the interaction-calculus core: header + agent/rule docs
src/ic.c                 the implementation — all FOUR agents (adds SUP to inet.c's three)
src/ic_demo.c            standalone driver / self-test (no project build needed)
tests/ic_test.c          the same checks through the project's test harness
docs/INET_ENGINE_PLAN.md the staged rewrite plan (the duality + HOTT, mapped onto the engine)
docs/ic_primitives.patch Scheme-facing primitives (ic-normalize/ic-cost/ic-reduce)
docs/ic_demo_output.txt  captured output of ic_demo — all 17 checks passing
scripts/tidy-structure.sh  cleanup #1: de-duplicate / un-nest the repo
scripts/tidy-artifacts.sh  cleanup #2: build & dev hygiene
```

## The rewrite (requests 1 & 2)

`docs/INET_ENGINE_PLAN.md` is the staged plan. Its short version: lizard already
has every separate piece — a Lafont combinator runtime (`src/inet.c`), a universe
lattice, a cubical equality engine, an elaborator IR (`core_term`). The rewrite
unifies them on one substrate. Your construction/observation duality becomes the
net's principal-port handedness; your universe / co-universe lattices are paired
by wiring (the turnstile); and HOTT's observation-defined equality (Id by
recursion on the type: Σ componentwise, Π pointwise, 𝒰 by equivalence) is what
lets the type theory run *as interaction*.

The plan's Phase 11 — the four agents — is done here. `inet.c` had three (λ/app,
duplication, erasure); `ic.c` adds the **superposition** agent and makes the
duality one line of the rule table:

```
   DUP{L} ~ SUP{L}   same label  -> ANNIHILATE   (the search collapses to a pair)
   DUP{L} ~ SUP{K}   L != K      -> COMMUTE       (the search fans out)
```

Build and run the self-test (only GMP is needed):

```
cc -std=c89 -O2 -Wall -Wextra -Isrc src/ic_demo.c src/ic.c -lgmp -o ic_demo && ./ic_demo
```

It exercises three things at once — sharing (the P / construction side, linear
interaction counts), search (the NP / observation side, computations distributing
over superpositions), and the label semantics that connect them. See
`docs/ic_demo_output.txt` for the expected output.

A note on scope: `ic.c` is built **standalone** on purpose. The full library does
not currently link — `src/lizard_internal.h` includes `<ds.h>`, which is missing
from the tree. So this stage avoids depending on the broken build. The `ic`
module is already registered in the Makefile's `LIB_OPTIONAL_SRCS`, and
`docs/ic_primitives.patch` wires it into the Scheme layer — both take effect once
`ds.h` is restored or vendored.

## The cleanup scripts (request 3)

Both default to a **dry run** (they print what they would do and change nothing);
pass `--apply` to act, or `--archive DIR` to move items aside instead of deleting.
Both refuse to run anywhere that doesn't carry the lizard fingerprint.

```
scripts/tidy-structure.sh        # preview the structural fixes
scripts/tidy-structure.sh --apply
scripts/tidy-artifacts.sh        # preview the hygiene sweep
scripts/tidy-artifacts.sh --apply
```

- **tidy-structure.sh** removes the duplication that crept in: the nested copy of
  the sources at `src/src/`, the whole second project tree duplicated inside
  `src/` (its own `lib/`, `tests/`, `docs/`, `Makefile`, `flake.*`, …), and the
  redundant `lizard.zip` snapshot. The canonical copies at the repo root are kept;
  `src/` is left holding only `.c`/`.h`.
- **tidy-artifacts.sh** sweeps build and dev debris: `*.o` / `*.d` under `build/`,
  `__pycache__` and `*.pyc`, coverage output, editor/OS leftovers, and empty stray
  files (e.g. an accidental `quit`). It is deliberately conservative — any
  *non-empty* stray file (like a hand-written `time2.sh`) is flagged for review,
  never deleted automatically.

Suggested order once you unzip: run both scripts in dry-run first to see exactly
what they would touch, then `--apply`. They are idempotent and safe to re-run.
