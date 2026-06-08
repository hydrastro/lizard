# INET_ENGINE_PLAN — rewriting lizard into an interaction-net machine

Status: planning document with a working, validated stack landed. The four
agents are implemented (`src/ic.c`, `src/ic.h`), differentially validated against
an independent oracle over 500k random terms (`tests/ic_fuzz.c`, Phase 13a), and
the first lowering of the core IR — the negative + Σ fragment plus runtime
arithmetic — runs and is fuzz-validated (`src/ic_lower.{c,h}`,
`tests/ic_lower_test.c`, Phase 12). Read `docs/INET.md` first — it describes the
existing combinator runtime (`src/inet.c`) and the soundness constraints. This
document picks up where that one ends ("the first step of a longer direction:
lizard becoming an interaction-net machine") and lays out the rest of the road.

The goal is the one you set: make lizard reduce the way HVM and Bend do — by
local graph rewriting on an interaction net — and do it so that the engine is
not an accessory but *the* evaluator, with the type theory expressed in the same
substrate. Two ideas govern the design: your directional construction/observation
duality, and Higher Observational Type Theory. They are not decoration; they
each pin down a concrete piece of the machine, and this document tries to show
exactly where.

This is staged on purpose. The trusted kernel is currently sound (the kernel
audit tracks a 198/0 figure), and nothing here is allowed to regress that until
a typed net can be reified back into a kernel term and re-checked. Every phase
is additive and independently testable; the net engine earns its way to the
centre one verified layer at a time.


## 0. Programs are graphs, not trees

This is the load-bearing idea, so it is worth being concrete rather than
aspirational. After compilation a program is *not* an abstract syntax tree; it
is an abstract syntax graph — a flat arena of agent nodes joined by wires
(`src/ic.c`: `g_p1`/`g_p2` are each node's two ports, `g_tag` its kind). Three
properties make it a graph and not a tree, and all three are present today:

- **Binding is an edge, not a name.** A variable compiles to a wire; `link_ports`
  resolves the variable slot into a direct connection between the binder's port
  and the use site. No variable name survives into the running graph — the
  occurrence *is* the edge.
- **Sharing is a DAG.** A value consumed in two places is not copied; it flows
  into a `DUP` node whose two outputs fan to the two consumers, so the subgraph
  is stored once and referenced from several edges. `((λx. x + x) 5)` compiles to
  a graph containing a `DUP` that shares `x`; `((λx. x * x) (2 + 3))` shares the
  *computed* `(2+3)` through a `DUP` so it is evaluated once. This value sharing
  is optimal and `tests/ic_sharing_test.c` measures it: duplicating an argument
  that reduces to a value adds only a small constant cost per use (the expensive
  work is done once), whereas naive copying would multiply it. Full
  *Lévy*-optimality — sharing reduction *inside a duplicated λ* as well — additionally
  needs the bracket/croissant oracle of §7; the same test shows that case is
  currently sound but not yet work-optimal.
- **Computation is local graph rewriting.** Reduction rewrites active pairs in
  place until none remain; there is no tree walk.

You can see this directly: `ic_dump_net` renders the compiled graph
(`tests/ic_graph_demo.c`, captured in `docs/ic_graph_output.txt`). A node prints
as `nK:TAG[p1 p2]`; a port is a wire to another node (`nK`), a number (`#n`), an
eraser (`*`), a wire junction (`vK`), or an open wire (`~`). The `DUP` nodes in
that output are the sharing, made visible.

Two honest qualifications. First, the *front end* — the parser, and the `core`
IR that lowers to nets (`src/ic_lower.c`) — is still a tree; it is the notation a
program is written in, and the graph is constructed from it at compile time. The
claim is not that no tree exists anywhere, but that the thing which *runs* is the
graph. Second, recursion is represented as a *self-referential* definition rather
than by unfolding into a tree: a `ref D a` node (`T_REF`) whose unfolding —
produced by `build_def_body` — contains further `ref D ...` nodes, so the
definition refers back to itself. The unfolding is lazy: a `ref` fires only once
its argument has reduced to a concrete number (exactly as `OPR` waits for its
operands), and it terminates because the definition branches on that number, so
the base case introduces no further `ref`. This is the discipline a real
interaction-net runtime (HVM) uses for its top-level definitions — the branch in
`build_def_body` plays the role of pattern-matching on the constructor — and it
is what lets `fact`/`fib`/`gcd`/… run on the net (`make ic-recursion`, 92 checks
including a bignum `25!`) without the un-taken branch of a conditional ever being
built (an eager Church-`if` recursor *would* diverge by building it). A fully
general lazy-whole-net fixpoint with back-edges shared *in the graph* is the
next refinement; the definition-reference form already gives genuine,
terminating general recursion driven by data.


## 1. Where lizard computes today

There are three evaluators in the tree, and that multiplicity is the thing the
rewrite is meant to collapse:


- the Scheme core runs on an AST tree-walk plus a bytecode VM
  (`src/runtime.c`, `src/bytecode.c`);
- the trusted type theory reduces `kterm_t` in `src/kernel.c` (`kt_whnf`,
  bidirectional infer/check, the universe lattice, inductives, unification);
- the cubical layer reduces `lizard_ast_node_t` through the "six descent
  operations" in `src/tt_equality.c` (interval, paths, faces, Kan composition,
  Glue, `ua`).

`include/core_term.h` is the elaborator's output IR — the bridge from surface
syntax to the kernel. It is the natural seam to compile *from*: elaborate once,
then lower `core_term` to a net instead of to a tree.

Alongside these sits `src/inet.c`: a correct Lafont interaction-combinator
runtime (CON for λ/application, DUP for sharing, ERA, exact-GMP OPR/OP1),
reachable from Scheme via `inet-normalize` / `inet-cost` / `inet-reduce`, and
differentially cross-checked against the trusted kernel on the shared untyped
λ-fragment. It is real and it works — but it is an *oracle*, used to validate
the kernel, not to run the language. It is also missing the fourth agent, and
its readback gives up on shared compound normal forms.


## 2. The four agents (Phase 11 — landed in `src/ic.c`)

Your design names four 3-arrow agents (one principal port, two auxiliary),
in four polarity patterns: λ, application, duplication, and superposition.
`inet.c` had three of them. `ic.c` is the self-contained successor that adds the
fourth and makes the duality computational:

```
   nulls     (0 aux)   ERA            the eraser / terminal
   numbers   (0 aux)   NUM            an exact GMP integer
   3-arrow agents (1 principal + 2 aux), labelled:
             CON                      lambda / application   (the CON dual pair)
             DUP                      duplication            (the construction reading)
             SUP                      superposition          (the observation reading)
             OPR / OP1                exact binary arithmetic
```

DUP and SUP are the *same* combinator read in the two dual directions, so the
entire computational content of the duality is one line of the rule table:

```
   DUP{L} ~ SUP{L}   (same label)   -> ANNIHILATE   the superposed values flow
                                                    straight to the two consumers;
                                                    the search collapses to a pair
   DUP{L} ~ SUP{K}   (L != K)       -> COMMUTE      the alternatives cross and
                                                    multiply; the search fans out
```

Everything else is the standard interaction-combinator table: `CON~CON` is beta;
same family and same label annihilate, otherwise commute; `ERA` erases; `NUM`
meeting `OPR`/`OP1` computes; `NUM` meeting `DUP`/`SUP` copies the number into
both wires. Numbers are exact GMP integers (not 24-bit), and the reducer counts
interactions, both inherited from `inet.c`.

`ic.c` builds and tests in isolation (GMP + libc only), exactly like `inet.c`,
which matters because the full library does not currently link — `src/lizard_internal.h`
includes `<ds.h>`, a header absent from the tree. The standalone driver
(`src/ic_demo.c`) and the harness test (`tests/ic_test.c`) both pass; the
Scheme-facing primitives (`ic-normalize` / `ic-cost` / `ic-reduce`) are written
as `docs/ic_primitives.patch`, ready to apply once `ds.h` is restored.

What the tests already demonstrate, in the language of your duality:

- *Construction / sharing / P.* A λ that uses its argument many times reduces
  it once via DUP; the interaction count stays linear. `S K K 5 = 5` in ten
  interactions; `(λx. x+x+x) 7 = 21` in seven.
- *Observation / search / NP.* A computation distributes over a superposition,
  running on every alternative. `(op + 1 {2 3})` → `{3 4}`.
- *The duality itself, via labels.* Matching DUP/SUP labels collapse the search
  (`(dup :7 x y {:7 10 20} (op + x y))` → `30`); equal-label superpositions stay
  correlated — one shared choice (`(op + {10 20} {100 200})` → `{110 220}`);
  distinct-label superpositions form the outer product — independent choices
  (`(op + {:1 10 20} {:2 100 200})` → `{{110 210} {120 220}}`).

That last group is the operational heart of the whole picture: **sharing is the
construction side, search is the observation side, and the label is what decides
whether a duplication collapses or fans out.** P sitting inside NP is the
statement that a DUP (checking a shared witness) is the same machine as a SUP
(searching alternatives), distinguished only by whether its label matches.


## 3. The duality, mapped onto the machine

Your framework is a *directional* (adjoint, contravariant) duality: not a
symmetry, but a pairing in which one side implies the other —
construction ⊢ observation, syntax ⊢ semantics, checking ⊢ searching, P ⊆ NP,
static ⊢ dynamic. The net realises this with a fixed handedness, which is why it
is the right substrate for it.

- **Principal-port orientation is the handedness.** An active pair reduces in
  exactly one direction; there is no symmetric rule. Construction agents
  (introductions: λ, DUP, NUM, SUP-as-value) present a value on their principal
  port; observation agents (eliminations: application, OPR, DUP-as-consumer)
  consume on theirs. Beta, projection, and arithmetic all fire only when a
  constructor meets its matching destructor, never the reverse — the adjunction
  made physical.

- **Two lattices, paired by the turnstile.** You describe a *universe* lattice
  on the construction side (`a : A : 𝒰₀ : 𝒰₁ : …`) paired with a *co-universe*
  lattice on the observation side (variables : bindings : contexts :
  contexts-of-contexts), with `Γ ⊢ a : A` being the *pairing* of an element of
  one with an element of the other. On the net this is literal: a closed term is
  a subgraph whose free wires are its *context interface*. The construction
  lattice is the tower of agents producing values; the co-universe lattice is
  the tower of *binders and ports* consuming them. The turnstile is a wiring —
  `link_ports` joining a value's principal port to a context's free port. The
  kernel's existing universe lattice (`src/tt_lattice.c`) supplies the
  construction tower; the co-universe tower is the binder/port structure the
  compiler in §4 already has to track.

- **Substitution is contravariant, by construction.** In the categorical
  semantics, substitution acts on contexts the opposite way it acts on terms;
  that is the fixed handedness, "not a real symmetry." On the net, substitution
  *is* DUP commuting a value out along the wires of the context — and the
  direction of that commutation is fixed by which port is principal. The
  contravariance is not enforced by a side condition; it is the only way the
  wires can be joined.

- **Checking ⊢ searching is DUP ⊢ SUP.** This is §2's rule table read as a
  thesis. Verifying a shared witness (one value threaded to many consumers) is a
  DUP; exploring alternatives (many values offered to one consumer) is a SUP.
  They are one combinator. A solver built on this engine *searches* by emitting
  SUP and *checks* by emitting DUP, and a matching label is exactly the moment a
  search result is pinned to a single construction.

The interaction rules you said "naturally arise from the system" are, concretely,
the annihilate/commute decision in `ic.c`'s `interact()`: same family + same
label → annihilate (the constructor meets its dual observation and the pair
resolves), otherwise commute (the two structures pass through each other,
duplicating). There is no third case to invent; the table is forced.


## 4. Compiling `core_term` to nets (Phases 12–13)

The elaborator lowers surface syntax to `core_term`, which wraps either a
`kterm_t` (the trusted dependent-type-theory IR — `src/kernel.h`) or a runtime
AST. The net backend lowers the computational fragment of that IR to an `ic`
net, reusing the binder management `ic.c` already implements.

The first slice of this is *done*: `src/ic_lower.{c,h}` lowers a `kterm`-shaped
core IR (de Bruijn, so it runs without the elaborator while `<ds.h>` blocks the
full build) to a named `ic_term`, which the existing engine reduces. It covers
the negative + Σ core — VAR, LAM, APP, PAIR, PROJ1, PROJ2, LET — and the runtime
fragment — LIT and the eight PRIM operators (`core_term`'s
`LIZARD_CORE_RUNTIME_AST` path). De Bruijn indices become fresh named binders on
a scope stack; a binder used more than once still becomes a fan of sharing DUPs
with binder-bit labels disjoint from user SUP/DUP labels. Σ uses the standard
constructor encoding (`pair = λk. k a b`, `πᵢ = p (λa.λb. selector)`), so CON
beta does the elimination and a shared pair is duplicated by the ordinary DUP
machinery — no new agents required at this stage. `tests/ic_lower_test.c` checks
this: curated cases (Σ intro/elim, nested pairs, de Bruijn under several binders,
a pair duplicated then projected twice, erasure) plus a differential fuzz that
generates random core terms with a known integer value and confirms the lowered
net agrees (170k terms across seeds currently pass).

What remains in this phase:

1. **The cross-check against the kernel (Phase 13b — done).** Random closed terms
   are reduced both on the trusted kernel (`kt_whnf`) and on the net and asserted
   to agree. The bridge `src/kt_to_core.c` translates the shared fragment —
   λ/app, Σ pairs/projections, `let`, `Bool` via the Church encoding, the other
   finite data (coproducts `inl`/`inr`/`sum_rec` and options
   `just`/`nothing`/`maybe_rec`, each a tagged Σ pair `(tag, payload)` reusing the
   first-class `PAIR`/`FST`/`SND` agents), and the **inductive data with their
   recursors**: `Nat` (`zero`/`succ`/`nat_rec`) and `List` (`nil`/`cons`/
   `list_rec`), as Church encodings whose *primitive* recursion is recovered with
   the pair-trick so the recursor sees the predecessor/tail, not only the
   recursive result — and the propositional-equality eliminator `J` on `refl`
   (`J C d A a b (refl v) → d v`, refl being transparent) — from the kernel's
   `kterm_t` into the core IR, which lowers to the net. That is **all six of the
   kernel's eliminators** (`bool_rec`, `sum_rec`, `maybe_rec`, `nat_rec`,
   `list_rec`, `J`). Two observables are used: a `Bool` term reduces on the kernel
   to `true`/`false` and on the net (applied to `1`,`0`) to `1`/`0`; a `Nat`-valued
   term (including `list_rec` folds and `J` results) reduces on the kernel to a
   numeral and on the net to the same integer (observing the Church numeral as
   `n (λx. x+1) 0`).
   `tests/ic_kernel_diff_test.c` checks `oracle == kernel == net` (finite data) and
   `kernel == net` (Nat/List/J) over 200k+ terms (`make ic-kernel-diff`). This pins
   the net's β / Σ / sharing / **structural-recursion** reduction to the reducer
   the type theory's soundness already depends on, not only to an in-test oracle.
   The kernel allocates through `lizard_heap_alloc`; in the full build that is
   `mem.c`'s arena, and the kernel uses no GC or runtime, so the test links against
   `liblizard.a` directly. Two honest caveats: (i) this validates recursion
   *expressed as λ-encoding* — it is genuine net reduction (the numeral/list drives
   the iteration through real `CON`/`DUP` interactions, terminating because the
   data is finite), with native inductive agents and general recursion-as-cycles
   left as a possible engine optimisation; (ii) the `J` check is the *eliminator*
   computation rule only. The deeper **by-observation `Id`** (14c) — `Id` over Σ
   componentwise, over Π pointwise, over 𝒰 by equivalence — is not validated here,
   and importantly `kt_whnf` does **not** reduce `KT_ID` structurally either (the
   kernel keeps `Id` canonical and only steps `J`), so 14c has no oracle in the
   trusted kernel: it must be checked against the cubical layer `src/tt_equality.c`
   or against a written specification. That is why 14c is the genuinely
   research-grade step rather than more of the same.
2. **Optimal readback** — the genuinely hard part. Reading a shared normal form
   back into a correct tree needs the labelled-bracket bookkeeping of
   Lamping–Gonthier optimal reduction (the "oracle"/croissants-and-brackets).
   Until that lands, readback stays honest: a correct tree when the result is
   affine (and exact integers, and superpositions), a faithful net rendering
   otherwise. This is called out as not-yet-done in `docs/INET.md`.

First-class `PAIR`/`FST`/`SND` agents (rather than the constructor encoding) are
deferred to Phase 14, where the Id/transport agent needs to dispatch on the Σ
former *structurally* (Id over Σ = componentwise).


## 5. HOTT on the net (Phases 14–15)

Higher Observational Type Theory is the third-generation answer to univalence:
instead of an interval and a primitive path type, the identity type `Id_A(x,y)`
is *defined by recursion on the structure of A*. That is the same
construction/observation duality again — **a type is characterised by how it is
observed**, and equality is read off the type's structure:

- `Id` over `Σ` is componentwise equality;
- `Id` over `Π` is pointwise equality — which gives function extensionality
  *definitionally*, not as an axiom;
- `Id` over a universe `𝒰` is equivalence — which gives univalence *by
  computation*.

This maps onto the engine cleanly because "recursion on the structure of A" is
precisely what an interaction net does when an agent meets a type-former agent:

- represent `Id` / `transport` as an agent whose principal port faces a *type*;
- when that agent meets a Σ-former, it reduces to the componentwise rule (two
  smaller `Id` agents on the components);
- when it meets a Π-former, it reduces to the pointwise rule (an `Id` agent
  under a binder);
- when it meets a universe, it reduces to the equivalence/`ua` rule — and an
  equivalence is itself a pair of functions with coherence, i.e. more net.

In other words, transport is not a global operation interpreted by a separate
reducer (as in the cubical `tt_equality.c` descents); it is *driven by the
type-former agent it collides with*, one local rewrite at a time. HOTT is the
type theory that makes this possible, because it is the one whose equality is
already defined by observation rather than by an interval that would have to be
threaded through every rule. The existing cubical machinery becomes the
reference oracle to validate these rewrites against, the same way the kernel
validates the untyped fragment.

The DUP/SUP labelled pair reappears here too: definitional `Id` computation is
the *checking* direction (a constructed equality meeting its observation and
collapsing), while proof *search* for an equality is the SUP direction. The same
agent set carries both.

**Status (Phase 14c).** The rule set above is now realised, on its own, as a
small terminating reduction system — `src/id_observe.c` (see `make id-observe`,
22 checks). It computes `Id_A(x,y)` exactly by recursion on `A`: `Bool`/`Nat`
structurally (`Id Nat (succ m)(succ n) → Id Nat m n`, bottoming out at
`Unit`/`Empty`), product componentwise, function pointwise (β fires under the
resulting `Π`, so `Id (Unit→Bool) (λ.true)(λ.true) →* Π Unit. Unit`), the
universe by univalence (`Id 𝒰 A B → Equiv A B`), and `transport refl x → x`;
open terms stay neutral, since canonicity is only for closed terms. Because
`kt_whnf` does **not** reduce `KT_ID` structurally, there is no kernel oracle for
these rules, so this module *is* the executable specification — the exact rewrite
set the `Id`/type-former agents must implement in the net. What remains for 14c
proper is to express that system *as agents in the graph* (an `Id` agent whose
principal port faces a type-former agent, reducing by the local rules above)
rather than as a standalone AST reducer, plus the genuinely dependent cases
(transport threaded through the second component of a dependent `Σ`); those are
validated against `tt_equality.c`.


## 6. Making the net the engine (Phases 16–17)

Once `core_term → net` is verified on the typed fragment and HOTT equality runs
as net interaction, the net becomes the primary evaluator:

- route the Scheme core and the elaborated kernel terms through the net instead
  of the bytecode VM and `kt_whnf`, keeping the tree-walk as a reference oracle
  behind a flag;
- retire `src/bytecode.c` / `src/runtime.c` as the default path once the net
  reaches parity (correctness *and* the interaction-count budget) on the test
  suite;
- keep the kernel's reify-and-recheck gate in place: a net result is only
  trusted once it can be read back into a `kterm_t` and re-checked, so soundness
  is never taken on faith.

Parallelism is the Bend-style payoff and comes almost for free: interaction is
local, so independent active pairs reduce concurrently. It is deliberately last
— correctness and optimal readback first, throughput after.

**Status (Phase 16, started).** The seam now exists: `src/net_eval.c` provides
`kt_eval_via_net`, which runs a closed kernel term on the net and reads the
result back into a `kterm_t` (the reify side of the reify-and-recheck gate above),
and `kt_normalize_gated`, which chooses between the net and `kt_whnf` behind a
gate (`kt_net_eval_set_enabled`), falling back to the trusted reducer whenever the
net declines a term. So the third bullet — only trusting a net result once it is
a `kterm_t` again — is realised for the current fragment, and the routing bullet
has a concrete entry point to grow into. The net-evaluator is checked against
`kt_whnf` (`make net-eval`). The result kind is now inferred from the term's type
(`kt_eval_via_net_auto` calls `kt_infer`, then whnf's the inferred type), so the
caller no longer supplies it. What remains is reading back richer result types
and pointing the system's actual evaluation path at `kt_normalize_auto` rather
than calling `kt_whnf` directly — at which point flipping the gate makes the net
the default.

**Status (Phase 17, in progress).** Two pieces are in place. First, confluence:
`tests/ic_confluence_test.c` shows that reducing LIFO versus FIFO
(`ic_set_reduce_fifo`) gives the same normal form in the same *interaction count*
across recursion, arithmetic, and DUP-sharing (54 terms) — the order-independence
parallel scheduling needs. Second, a wavefront reducer (`ic_set_reduce_rounds`)
fires each generation of disjoint active pairs as one batch and reports the
parallel depth (rounds) and peak width (`tests/ic_parallel_test.c`); since the
work (interaction count) is unchanged, work/depth is the available parallelism —
branching recursion is highly parallel (`fib 18`: ~389 average, peak frontier
2266), while tail recursion stays sequential (`gcd`: 1.0). This is a faithful
single-threaded *model* of the parallel schedule. The remaining step is the
actual multithreaded reducer — atomic operations on ports and a thread-safe arena
so independent active pairs reduce on real cores — which is the Bend/HVM-style
payoff and deliberately last.


## 7. Honest list of the hard parts

- **Optimal reduction (the oracle).** The labelled fans already share *values*
  optimally — `tests/ic_sharing_test.c` measures it: duplicating an argument that
  reduces to a value costs ~3 interactions per extra use regardless of how
  expensive the argument was (the work is done once), where copying would cost
  ~one full reduction per use. What is *not* yet work-optimal is sharing
  reduction *inside a duplicated λ*: the same test shows a duplicated lambda's
  body is re-reduced per application (~25/use here), because the fan commutes
  through the abstraction and copies it. Closing that — genuine Lévy-optimality
  for the general untyped case — needs the bracket/croissant "oracle" of Lamping
  (1990) / Asperti–Guerrini, which tracks the *level* of each fan so nested
  duplicators pair up correctly. It is the classic hard problem of this field,
  and the same boundary HVM lives at (its labelled fans are optimal on the
  fragment they cover, not on every term).

  *Progress (`src/opt_core.{c,h}`, `make opt-core`).* Three pieces of the oracle
  are now built and validated independently. (a) A **reference normal-order
  β-normaliser** — ground truth for the λ-fragment, since the kernel does not
  normalise open λ-terms (it passes Church `succ`/`add`/`mul`). (b) The
  **Asperti–Guerrini interaction engine** — indexed agents λᵢ, @ᵢ, fan δᵢ,
  croissant ∩ᵢ, bracket ⊔ᵢ, eraser, with the exact annihilation and propagation
  rules; a rule-level self test checks all of them (β wiring; δᵢ duplicates a
  λⱼ when i<j; ⊔ᵢ increments and ∩ᵢ decrements the index of a higher agent; δ–δ
  same-index annihilation; eraser propagation). (c) An **encode + read-back
  pipeline** validated against the reference on a fragment that *includes
  function duplication* — `I`, `I I`, `church2`, `(λg.g g) I`, `K I I` all read
  back to the correct normal form. The remaining hard step is exactly the
  **fully general index-correct encoding**: with croissants alone, deeper nesting
  (Church numerals, reducible-body duplication chains) produces same-index /
  different-type active pairs that no rule covers — these are precisely what the
  matched **bracket** placement (the famous bookkeeping) must rule out by
  construction. `make opt-core` prints those terms as the documented gap rather
  than guessing a result. Refs: Lévy 1980; Lamping POPL 1990; Asperti–Guerrini
  CUP 1998, pp. 39–42; Salikhmetov, arXiv:1609.03644 (the rule set).
- **Optimal readback.** Affine readback is done; the general labelled-bracket
  bookkeeping is not. Until it exists, compound shared normal forms render as
  faithful net dumps, never as guessed trees.
- **Typed nets.** Reifying a *typed* net back into a kernel term — the thing
  that lets the engine replace `kt_whnf` without risking soundness — is the open
  research frontier already flagged in `docs/INET.md`.
- **HOTT equality coverage.** The Σ/Π/𝒰 rules above are the easy, definitional
  core; higher coherences and the full coherence theorem are a research effort,
  validated against the cubical layer as oracle.
- **Recursion as cycles, not unfolding.** The fragment lowered so far is acyclic.
  Fixpoints, `letrec`, and the `kterm` recursors (`NAT_REC`, `LIST_REC`,
  `BOOL_REC`, `J`) must compile to a *cyclic* wire in the net — a back-edge that
  the reducer unrolls on demand — not to an unfolded tree. Getting this right is
  what keeps the "graph, not tree" property (section 0) true once the language
  has real recursion; a naive recursor lowering would silently reintroduce trees.
- **The build dependency.** The full library needs `ds` (the data-structures
  library that supplies `<ds.h>`, included by `src/lizard_internal.h` and
  `src/lizard.c`); with `ds` on the include/link path the project builds. `ic`,
  `ic_lower`, and their tests are also built standalone (GMP + libc only) so this
  work is independently verifiable; `ic` and `ic_lower` are registered in the
  Makefile's `LIB_OPTIONAL_SRCS`, and `docs/ic_primitives.patch` wires `ic` into
  the Scheme layer. With `ds` present, the typed cross-check (13b) against
  `kt_whnf` is unblocked.


## 8. Phase summary

```
  11  the four agents                  DONE   src/ic.c, src/ic.h, tests/ic_test.c
                                              (SUP added; DUP/SUP duality; exact GMP)
  13a differential validation          DONE   tests/ic_fuzz.c -- net vs independent
                                              oracle, 500k random terms agree
  12  core_term -> net backend         DONE   src/ic_lower.{c,h}, tests/ic_lower_test.c
                                              negative + Sigma core + runtime frag;
                                              fuzzed vs oracle (pair/proj/let/deBruijn)
  13b cross-check vs the kernel        DONE   src/kt_to_core.{c,h}, tests/ic_kernel_diff_test.c:
                                              random closed terms reduced on the trusted
                                              kt_whnf AND on the net (via kt_to_core) agree with
                                              each other and an oracle (200k+); covers beta, Σ
                                              pairs/projections, bound vars, and ALL SIX kernel
                                              eliminators — bool_rec, sum_rec, maybe_rec,
                                              nat_rec, list_rec, and J on refl.  i.e. the whole
                                              computational fragment, including structural
                                              recursion, is validated.  `make ic-kernel-diff`
      sharing / optimal reduction      PART   src/ic.c (labelled DUP fans), tests/
                                              ic_sharing_test.c: VALUES are shared optimally
                                              (expensive arg reduced once, ~3 interactions per
                                              extra use), duplicated LAMBDA bodies are copied
                                              per application (~25/use) -- sound, not yet
                                              work-optimal.  Full Lévy-optimality needs the
                                              bracket/croissant oracle (Lamping/Asperti-Guerrini),
                                              the classic hard problem.  `make ic-sharing`
      optimal reduction (oracle)       PART   src/opt_core.{c,h}, tests/opt_core_test.c:
                                              reference normal-order normaliser (ground truth,
                                              Church arithmetic) + Asperti-Guerrini interaction
                                              engine (indexed lambda/@/fan/croissant/bracket;
                                              all rules self-tested) + encode/read-back validated
                                              vs the reference on a fragment INCLUDING function
                                              duplication ((\g.gg)I).  The fully general index-
                                              correct encoding (matched brackets) is the
                                              remaining hard step.  `make opt-core`
  14a first-class PAIR/FST/SND agents  DONE   src/ic.c (PAIR/FST/SND, do_proj),
                                              syntax (pair/fst/snd), readback renders
                                              (pair a b); ic_lower emits them and is
                                              cross-checked vs the Church encoding +
                                              oracle (270k terms, incl. shared pairs)
  14b Id-by-observation: transport     PART   src/ic.c (T_TRANSP, do_transp):
      structural + reflexive core             transp meets a former and reduces by it
                                              (Σ componentwise, Π pointwise, base trivial);
                                              reflexive transport = identity, fuzz-checked
                                              (transp wrap never changes a value)
  rec recursion-as-cycles              DONE   src/ic.{c,h} (T_REF, build_def_body,
                                              expand_def), tests/ic_recursion_test.c:
                                              a recursive definition is a self-referential
                                              graph whose `ref` node unfolds lazily once its
                                              argument is concrete; fact/sumto/fib/pow2/gcd
                                              run on the net and match C oracles incl. a
                                              bignum 25! (92 checks).  `make ic-recursion`.
                                              This is the HVM top-level-definition discipline;
                                              the C branch in build_def_body is the match.
  14c Id-by-observation: semantics     DONE   src/id_observe.{c,h}, tests/id_observe_test.c:
      as a reduction system            (sem.)  a self-contained terminating reduction system
                                              that computes Id_A(x,y) by recursion on A —
                                              Bool/Nat structurally, product componentwise,
                                              function pointwise (funext, beta under Pi),
                                              universe by univalence (-> Equiv), transport-
                                              refl = identity; 22 checks vs hand-computed
                                              normal forms (no kernel oracle exists).
                                              `make id-observe`.  This is the executable
                                              SPEC the net's Id + type-former agents must
                                              implement; wiring it into the net as agents
                                              (Id meets Σ/Π/𝒰 in the graph), and the dependent
                                              cases, are the remaining 14c step.
  15  transport as type-former rewrite   DONE   src/id_observe.{c,h} (transport extended),
      (semantics)                         (sem.)  tests/id_observe_test.c: transport reduces by
                                              the structure of its type family — refl and any
                                              constant family give the identity; the identity
                                              family over 𝒰 gives the univalent rule
                                              transport^(λX.X) (ua f) x = f x (e.g. negating a
                                              Bool), 30 checks total.  `make id-observe`.  The
                                              in-tree cross-check against the cubical layer
                                              src/tt_equality.c (which carries the same
                                              transport-refl + per-type-former rules) is the
                                              remaining external validation, and the dependent
                                              function/Σ family cases are the next extension.
  16  net becomes the primary engine    STARTED src/net_eval.{c,h}, tests/net_eval_test.c:
                                              kt_eval_via_net runs a closed kernel term on the
                                              net and reads the result back as a kterm (Nat /
                                              Bool); kt_eval_via_net_auto INFERS the result kind
                                              from kt_infer (type-directed dispatch -- no explicit
                                              kind); kt_normalize_gated / kt_normalize_auto select
                                              net-vs-kt_whnf behind a gate, falling back to the
                                              trusted reducer when the net declines.  Validated
                                              against kt_whnf (`make net-eval`).  Remaining: richer
                                              result types, and routing the system's main eval
                                              path through kt_normalize_auto (then the gate makes
                                              the net the default).
  17  parallel reduction              PROGRESS src/ic.c (ic_set_reduce_fifo / ic_set_reduce_rounds),
                                              tests/ic_confluence_test.c + ic_parallel_test.c.
                                              (a) Strong confluence: LIFO and FIFO reach the same
                                              normal form in the SAME interaction count (54 terms)
                                              -- the order-independence parallel scheduling needs.
                                              (b) Wavefront reduction reduces each generation of
                                              disjoint active pairs as one batch and reports parallel
                                              DEPTH (rounds) and peak WIDTH; work is unchanged, so
                                              work/depth is the available parallelism -- branching
                                              recursion is highly parallel (fib 18: ~389 avg, peak
                                              2266), tail recursion is sequential (gcd: 1.0).  Checked
                                              against sequential reduction.  `make ic-confluence`,
                                              `make ic-parallel`.  Remaining: the real multithreaded
                                              reducer (atomic port ops over the frontier; thread-safe
                                              arena) -- the Bend/HVM-style payoff.
```

The throughline: lizard already has every separate piece — a combinator runtime,
a universe lattice, a cubical equality engine, an elaborator IR. The rewrite is
not new machinery so much as *unifying* them on one substrate, where your
construction/observation duality is the principal-port handedness, the two
lattices are paired by wiring, and HOTT's observation-defined equality is what
lets the type theory run as interaction. The fourth agent was the missing piece
of the dynamics; it is now in place, tested, and waiting for the build to be
made whole so it can move from oracle to engine.
