# Architecture: terms, the net, and the `><` rules

This document explains how the engine is layered, why the *source* you write looks
S-expression-like rather than a hand-drawn interaction net, where the `α >< β` interaction
rules actually live, and what is and is not parallel. It is meant to be the orientation
document; `INET_ENGINE_PLAN.md` has the per-feature detail and `ROADMAP.md` the status.

## 1. Two layers, and a reference oracle beside them

There are **two representations of a program** and an independent **reference implementation**:

1. **Surface terms.** What you write — `id_idty(...)`, `id_transp(...)`, `id_sigma(...)`,
   `id_app(id_lam(...), ...)`. These are *constructors of a type-theory term language* (a
   small dependent λ-calculus: variables, λ, application, the type formers ×/+/Σ/Π/→/List/U,
   and the witnesses refl/transport/rec/case). It looks Lisp-y for one boring reason: it is
   constructor application, `f(g(x), h(y))`, written in C. It is **not** the interaction net.

2. **The interaction net.** The runtime representation. `enc(term)` *compiles* a surface term
   into a graph of **agents** (nodes) wired through **ports**, held in flat arrays
   (`k_[]` = kind, `pt_[]/pp_[]` = port target node/port). Reduction happens here.

3. **The spec (`id_observe.c`).** A self-contained, obviously-terminating **tree-rewriting**
   system that computes the same thing (`Id_A(x,y)` by recursion on `A`, transport by recursion
   on the family, …). It is the oracle. The net engine (`idnet.c`) is checked against it by
   differential fuzzing — hundreds of thousands of random terms, requiring
   `idnet_id_nf(t) == id_nf(t)` or an explicit refusal, never a wrong answer.

So "do we need a different syntax for graphs?" — no. The graph is the *internal* form; the
surface stays a term language, and a separate tree-rewriter keeps the net honest.

## 2. What an agent, a port, and `><` are here

* An **agent** is a node with a **kind** (`N_LAM`, `N_APP`, `N_ID`, `TY_SIGMA`, `VAL_TRUE`, …)
  and up to `PORTS` ports.
* **Port 0 is the principal port** (the convention `up_port(kind)` encodes which port is the
  "output"/principal for each kind). The other ports are auxiliary.
* An **active pair** — what other interaction-net languages draw as two triangles meeting,
  `α >< β` — is simply **two principal ports wired to each other**: `a.port0 <-> b.port0`.
  That is the redex. The reducer scans for exactly this condition.

You never type `><`. You build a term; `enc` lays out the agents and wires; active pairs *arise*
where a principal port meets a principal port.

### A real net dump

Encoding `Id Bool ((λx.x) true) true` produces this net (principal port = port 0):

```
agent #0 = N_ROOT   : port0(principal)-> #1.port1
agent #1 = N_ID     : port0(principal)-> #2.port0   port1-> #0.port0   port2-> #3.port1   port3-> #7.port0
agent #2 = TY_BOOL  : port0(principal)-> #1.port0
agent #3 = N_APP    : port0(principal)-> #4.port0   port1-> #1.port2   port2-> #6.port0
agent #4 = N_LAM    : port0(principal)-> #3.port0   port1-> #5.port0
agent #5 = N_VAR    : port0(principal)-> #4.port1
agent #6 = VAL_TRUE : port0(principal)-> #3.port2
agent #7 = VAL_TRUE : port0(principal)-> #1.port3

active pairs (two principal ports connected -- the >< redexes):
    #1 N_ID  ><  #2 TY_BOOL
    #3 N_APP ><  #4 N_LAM
```

Read it off: the `N_ID` agent's principal port meets `TY_BOOL`'s principal port (that is
`Id` *observing* the type `Bool`), and the `N_APP` agent's principal meets the `N_LAM`'s
principal (that is a β-redex, `(λx.x) true`). Both `><` pairs are present at once; reduction
fires them and the net settles to `Unit` (since `true` observed against `true` is contractible).

## 3. The interaction rules *are* the C `rule()` cases

In a textbook interaction-net language you write rules as `α >< β => <new net>`. Here those
rules are the branches of `rule(a, d)` in `idnet.c`. When the reducer finds an active pair, it
calls `rule(agent, data)`; the branch matching the two kinds rewrites the local net with
`mk` (allocate an agent), `link_` (wire two ports), and `splice` (reconnect a neighbour). That
right-hand-side net **is** the rule's RHS.

For example, `Id` meeting the type former `U` is the univalence rule "a path in the universe is
an equivalence", `Id_U a b => Equiv a b`. In code it is literally:

```c
if (ka == N_ID) {
  ...
  if (kd == TY_U) {                 /* ID  ><  TY_U   =>   Equiv a b   (univalence) */
    int e = mk(N_EQUIV);
    link_(e, 1, /* a */ ...);
    link_(e, 2, /* b */ ...);
    splice(a, 1, e, 0);             /* the Id-output wire now carries the Equiv */
    return;
  }
}
```

The catalogue of `α >< β` rules is therefore the set of `if (kd == …)` arms under each agent in
`rule()`: `N_ID` against each type former (the Id-by-observation rules), the value testers
(`N_IDB >< VAL_TRUE`, `N_IDN >< VAL_SUCC`, …), `N_APP >< N_LAM` (β), the recursor and fold
steps, `N_TR` against each family (transport), `N_SIGD` deciding a Σ-path, and so on. Adding a
new type former = adding its agent kind, its `enc` case, and its `rule()` arms — then proving
the arms agree with the spec by fuzzing.

A textual net dumper (as shown in §2) is a few lines over the same arrays; exposing a `><`
*surface* syntax would also be possible. We simply have not needed one: the surface is the type
theory, and the net is the compilation target.

## 4. The reduce loop

`serial_sweep_once()` scans the arena once: for each agent whose principal meets another
principal, orient it as (active agent, data) and call `rule()`. (One guard: a *value*
eliminator — `if`, the Nat recursor, its step-forcer — meeting a bare variable WAITS, because
it is sitting in an un-applied function body; β must substitute a real value first.) The
sequential reducer loops the sweep to a fixpoint. Read-back (`rb`) walks the settled net from
the root back into a surface term, **refusing** (returning NULL) if it meets an agent that
should have been consumed — that is how "unsupported" becomes an honest refusal rather than a
wrong type.

One consequence shapes how *neutral* results are represented. After `rule()` returns, the loop
**erases** the active pair unconditionally — so a rule that decides "this is neutral" cannot
simply leave the stuck agent in place (it would be erased and read-back would refuse). Instead a
genuinely neutral result is emitted as a **fresh, non-active node** that read-back knows how to
render: `N_NID` for a neutral `Id A x y` (built when an observation meets a variable), and
`N_NTR` for a neutral transport `transport^(λX. body) p x` (built when a non-constant family is
transported along a *variable* path — constant families having already reduced to the identity).
Because these nodes are not active agents, the reducer never touches them; because the dependent
Σ-Id body is built as an *active* `Id` over an *active* transport, a product second component
still decomposes componentwise (the transport splits, its base legs becoming `N_NTR`), matching
the spec rather than freezing as one opaque blob.

## 5. Parallelism: what is, what isn't, and why

Interaction nets are famously parallel-friendly: disjoint redexes commute, and the system is
**strongly confluent** (the normal form is independent of the order of firing). So in principle
every `><` pair can fire concurrently. The catch is **locality**.

* A **local** rule rewrites only its two agents plus their immediate neighbours and allocates a
  constant number of fresh agents. Its write-set is `O(1)`. The Id-observation and value-tester
  rules are local.
* Our **substitution copies a subtree in one step**. β is `rule`: `cs(body, …)` duplicates the
  whole λ-body graph at once. That is a *non-local macro-rule* — its read/write-set is the size
  of the body, unbounded. The recursor/fold steps, Σ-construction, and transport are non-local
  for the same reason (they `cs`).

Naive neighbourhood locking is therefore unsound for the non-local rules: while one thread copies
a subtree, another could be rewriting inside it. This is the crux, and it is exactly why
fine-grained interaction-net parallelism (HVM, GPU evaluators) is built on **local** rules with
incremental duplication (δ/fan agents copying one node per step). Our affine/`λK` engine
(`deltanets.c`) already has that local, fan-based duplication; the HoTT engine (`idnet.c`) does
not — it deliberately uses subtree-copy for a clear sequential reference.

### What the multithreaded reducer does

`reduce_par(nthreads)` is **sound by construction**:

1. **Fire only the local rules in parallel.** A whitelist (`rule_par_safe`) admits exactly the
   audited-local collisions: `N_ID` against `TY_BOOL/TY_NAT/TY_UNIT/TY_EMPTY/TY_U`, and the
   testers `N_IDB/N_IDN/N_EQT/N_EQF/N_ISZ/N_SCS` against values. None of these call `cs`, none
   set `keep_d`.
2. **Claim disjoint neighbourhoods.** Before firing a redex `(a,b)`, a worker atomically
   (CAS) claims `{a, b} ∪ neighbours(a) ∪ neighbours(b)`, in index order; if any claim fails it
   backs off and skips. Two firings that share any node can never both proceed, so concurrent
   firings are disjoint. The active-pair condition is re-validated under the claim.
3. **Serial fallback for everything else.** β, the recursor/fold steps, Σ-construction, and
   transport run in a sequential sweep. This also guarantees termination identical to the
   sequential reducer.

`mk` allocates with an atomic fetch-and-add; the interaction counter is atomic. Confluence then
guarantees the parallel result equals the sequential one. This is checked, not assumed: the test
suite runs tens of thousands of mixed reductions across 2/4/8 threads requiring
`par == seq == spec`, plus a determinism stress (the same term reduced 1500× at 8 threads with
zero variance — a race would show up as a varied result).

### Honest performance

At present the multithreaded reducer is **correctness-first, not yet faster**. On the workloads
here it is overhead-bound (e.g. a depth-13 balanced product, ~49k interactions: ~4.3 ms
sequential vs ~4.7 ms at 2–8 threads). Three reasons, all real:

* The **parallel fraction is small** — only the leaf observation rules fire concurrently, while
  the structural decomposition and all subtree-copy rules are serial (Amdahl).
* The reducer **re-scans the whole arena** each round to find redexes; a redex **work-queue**
  would remove most of that cost.
* Per-net work here is **tiny** relative to thread-barrier synchronisation.

The value delivered is the hard part — *provably race-free* concurrent interaction-net
reduction and the claim-based disjoint-firing mechanism. The route to real speedup is clear and
is the stated GPU endgame: (a) a redex work-queue instead of full re-scans, (b) make the
structural rules local so far more fires in parallel — i.e. incremental duplication, the
`deltanets.c` direction — and (c) move the local-rule firing onto a massively-parallel executor
(OpenCL/GPU), where thousands of disjoint `><` pairs fire per step.

## 6. The discipline that keeps it honest

Every change: compile every unit under strict C89 (`-std=c89 -pedantic-errors -Werror
-Wconversion …`, plus `-pthread` for the net), run the spec checks and the full fuzz, and — for
the net — require it to match the spec or refuse, **never** mis-compute. The parallel reducer
adds its own invariant on top: it must produce the *same* normal form as the sequential engine,
for every term, at every thread count.
