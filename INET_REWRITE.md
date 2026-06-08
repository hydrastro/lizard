# lizard → interaction-net engine (INET_REWRITE)

This bundle is the first concrete stage of turning lizard into an HVM/Bend-style
interaction-net machine, plus the two repo-cleanup scripts. Files are laid out at
repo-relative paths, so unzipping this archive at the repository root drops each
file into the directory it belongs in. The only file that replaces an existing
one is `Makefile` — and that copy is your Makefile plus a single addition (`ic`
and `ic_lower` in `LIB_OPTIONAL_SRCS`); every other path is new. This orientation
file is `INET_REWRITE.md`, not `README.md`, so it leaves your `README.md` alone.

## What's here

```
src/ic.h                 the interaction-calculus core: header + agent/rule docs
src/ic.c                 the implementation — all FOUR agents (adds SUP to inet.c's three)
src/ic_demo.c            standalone driver / self-test (no project build needed)
src/ic_lower.h           core_term-shaped IR + lowering to nets (Phase 12)
src/ic_lower.c           the lowering implementation (Sigma + runtime fragment)
tests/ic_test.c          the four-agent checks through the project's test harness
tests/ic_fuzz.c          differential validator: net vs an independent integer oracle
tests/ic_lower_test.c    lowering checks + a core-term fuzz vs the oracle
tests/ic_graph_demo.c    prints a program as an abstract syntax GRAPH (shows sharing/DUP)
src/kt_to_core.{c,h}     bridge: trusted-kernel terms (kterm_t) -> core IR -> net
tests/ic_kernel_diff_test.c  Phase 13b: kt_whnf vs the net agree on random Bool terms
                         (run with `make ic-kernel-diff`)
Makefile                 your Makefile, with `ic` and `ic_lower` added to LIB_OPTIONAL_SRCS
docs/INET_ENGINE_PLAN.md the staged rewrite plan (the duality + HOTT, mapped onto the engine)
docs/IC_SYNTAX.md        full syntax of the primitives (textual + Scheme surfaces)
docs/ic_primitives.patch Scheme-facing primitives (ic-normalize/ic-cost/ic-reduce)
docs/ic_graph_output.txt captured ASG dumps — variables as wires, sharing as DUP nodes
docs/ic_demo_output.txt  captured output of ic_demo — all 17 checks passing
docs/ic_fuzz_output.txt  captured output of ic_fuzz — random terms agreeing with the oracle
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

And the differential validator (this is roadmap item 13a — see the plan), which
builds random terms with a known integer value and checks the net agrees:

```
cc -std=c89 -O2 -Wall -Wextra -Isrc tests/ic_fuzz.c src/ic.c -lgmp -o ic_fuzz
./ic_fuzz 100000          # optional: ./ic_fuzz <iterations> <seed>
```

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
