# INET — the interaction-net engine, and its bridge to the trusted kernel

Status: an interaction-combinator runtime exists in the kernel (`src/inet.c`,
`src/inet.h`), reachable from the surface, and it has been **differentially
cross-checked against the trusted tree-walking kernel** on the shared untyped
lambda fragment.  This document records what the engine is, what is verified,
how the two engines are bridged, and — honestly — what is *not* yet done.

This is the first step of a longer direction: lizard becoming an
interaction-net machine.  It is built **alongside** the trusted type-checking
kernel as an additive module; it does not replace `kt_whnf` and does not touch
the cubical layer.  Protecting the kernel's soundness (currently 198/0) is the
governing constraint, and reifying *typed* nets is the open research frontier
(see "What is not done").

## The model

Computation is local graph rewriting on an interaction net (Lafont's
interaction combinators; Lamping–Gonthier optimal reduction; the HVM /
HigherOrderCO runtime by Taelin).  An *active pair* is two agents whose
principal ports are wired together; reducing it is a constant-size local
rewrite.  Sharing (via the duplicator) makes reduction Lévy-optimal: a
duplicated function is reduced once, not once per use.

Agents in the runtime (`src/inet.c`):

- **CON** — the constructor, used for both λ (lambda) and application; the
  principal-port orientation distinguishes intro from elim.  CON~CON is **beta**
  (annihilation).
- **DUP** — the duplicator (labelled).  DUP commuting past another agent is how
  a value is *shared*/copied.  DUP~DUP with the same label is wire-merge
  (annihilation); with different labels it commutes.
- **ERA** — the eraser/null: erases an agent and propagates to its neighbours.
  This is how an unused binder (e.g. the discarded argument of `K`) is collected.
- **OPR / OP1** — numeric agents carrying an exact **GMP** integer and a binary
  operator (`+ - * / % = < >`); OP1 is the partially-applied form holding one
  operand.  Arithmetic is therefore *exact* and unbounded (an improvement over
  HVM2's 24-bit machine integers).

`link_ports` performs wire-chasing substitution and pushes a redex when it joins
two principal ports; `interact` applies one rewrite; `reduce` runs to normal
form.  The reducer counts interactions (`inet-cost`) so that sharing can be
demonstrated empirically rather than merely asserted.

### Relationship to the four polarised 3-arrow agents

The intended design (from the project's interaction-net vision) names four
3-arrow agents — λ, application, duplication, superposition — each with one
principal port, in the four polarity patterns
`(in; out out)`, `(out; in in)`, `(in; out in)`, `(out; in out)`.
The current runtime realises the *dynamics* of this system (λ/application as the
CON dual pair under the principal-port mechanism; duplication/superposition as
DUP and its commutation), which is what makes beta + sharing + erasure correct.
The explicit in/out **polarity labelling** of each port is orientation metadata
layered on top of the dynamics; the exact assignment of the four patterns to the
four agents is a convention to be pinned down (it does not change the reduction
rules, which depend on principal-port matching, not on the in/out tags).  This
is the one piece of the surface vision deliberately left open here, to be fixed
deliberately rather than guessed.

## Surface interface

- `(inet-normalize t)` — reduce `t` to normal form and read back an **integer**,
  or `#f` if the normal form is not an integer (e.g. it is a lambda).
- `(inet-cost t)` — the **interaction count** for reducing `t` (the empirical
  sharing measure).
- `(inet-reduce t)` — reduce `t` and read the normal form back as a **de Bruijn
  lambda term** string (`#k` bound var, `(f a)` application, `(lam b)`
  abstraction), or `#f` if the normal form is not representable.

`t` is a surface lambda/arithmetic term: `(lambda (x) body)`, `(f a ...)`
(left-folded application), integer literals, and `(op l r)` for the operators
above.

## The bridge: differential agreement with the trusted kernel

The decisive question is whether this net dynamics **agrees** with the audited
tree-walking kernel on the fragment both speak.  The test (made permanent in
sangaku `examples/451-interaction-net-bridge.lisp`) is, for each Church numeral
N:

- the **trusted kernel** beta-reduces `church_N s z` to `s` applied N times to z
  (`kernel-reduce` / `kernel-equal?` — the audited evaluator); and
- the **interaction net** reduces `church_N succ 0` to the integer N
  (`inet-normalize` — the graph machine).

These two completely different engines — substitution-based tree walking vs.
local graph rewriting with optimal sharing — produce the same answer for
N = 0..7, and the net independently computes exact arithmetic
(`(λx. x*x) 8 = 64`, `(6*3)+(10-1) = 27`), reads lambda normal forms back
(identity → `(lam #0)`, Church 2 → `(lam (lam (#1 (#1 #0))))`), and exhibits
observable sharing via `inet-cost`.  The C-level core test (`tests/inet_test.c`,
25 assertions) additionally covers S K K → I, Church addition PLUS 2 3 → 5,
big-integer arithmetic (10^24), and an **honest readback boundary**: when a bare
normal form still carries residual compound sharing, readback *refuses* (returns
no term) rather than emit a wrong one — while the same computation still yields
the correct integer on demand.

## What is verified

- The net core builds clean under the kernel's strict flags
  (`-std=c89 -Wall -Wextra -Werror -pedantic` and the full warning set).
- `tests/inet_test.c`: 25/25 assertions (beta, sharing, erasure, exact GMP
  arithmetic, Church numerals 0..20, S K K, Church addition, readback, and the
  readback refusal boundary).
- The differential bridge agrees with the trusted kernel on Church 0..7.
- Adding all of this changed **nothing** in the trusted kernel: kernel soundness
  stays 198/0 and kernel_test 21/0.

## What is NOT done (the open frontier)

- **Typed nets.**  The shared ground here is the *untyped* lambda calculus.
  Carrying dependent — let alone cubical/observational — *typing* on the
  interaction-net substrate is unproven and is the genuine research problem.
  The realistic near-term shape is two-layer: the net machine as evaluator, the
  audited kernel as checker.  Whether the net dynamics can carry dependent
  typing *directly* is the decisive experiment that determines whether
  "the substrate is the type theory" is achievable or whether the two-layer
  architecture is the answer.
- **Superposition as a first-class surface agent** (with labels exposed) and the
  precise in/out polarity assignment of the four agents.
- **Replacing `kt_whnf`.**  Not attempted and not on the near path: the trusted
  tree-walker remains the evaluator of record for the typed kernel until the
  net engine is shown to agree on a much larger fragment (and to carry typing).
