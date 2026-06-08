# Roadmap — lizard → interaction-net machine

A map of where this project is and where it's going. The plan is to rewrite the
lizard runtime onto a single **interaction-net** substrate — the model behind
HVM/Bend — so that evaluation, the dependent type theory (HOTT), and the
construction/observation duality all run as graph rewriting on the same agents.
The far goal is **massively parallel execution on the GPU (OpenCL)**, which the
interaction-net model is uniquely suited to.

Legend:  ✅ done & validated   🟡 partial   ⬜ open / future   ⭐ you-are-here

---

## Where we are now  ⭐

The four-agent core engine is built, validated against the trusted kernel, and
runs structural recursion. The optimal-reduction engine (Δ-Nets) is **airtight**:
provably correct on the affine fragment and never-wrong (correct-or-refused) on
the full λ-calculus. Parallelism is demonstrated (strong confluence + a wavefront
reducer on both the core engine and Δ-Nets). The two big open fronts are (1) full
λ-K *sharing* in Δ-Nets — turning refusals into correct answers — and (2) the real
multithreaded/GPU reducer.

---

## Track A — Core interaction engine  (`src/ic.c`)

```
✅  Four agents: λ / application / duplication / superposition (DUP·SUP duality)
✅  Exact arithmetic on the net (GMP), readback to terms
✅  Differential validation: net vs independent oracle, 500k random terms agree
✅  core_term → net backend (src/ic_lower.c), fuzzed vs oracle
✅  Cross-check vs the trusted dependent-type kernel: 200k+ closed terms reduced
    on kt_whnf AND on the net agree — β, Σ pairs/projections, all six eliminators
    (bool/sum/maybe/nat/list rec, J on refl): the whole computational fragment
✅  First-class PAIR / FST / SND agents, cross-checked (270k, incl. shared pairs)
✅  Recursion-as-cycles: a definition is a self-referential graph; ref unfolds
    lazily — fact / sumto / fib / pow2 / gcd, incl. bignum 25! (92 checks)
```

## Track B — Type theory on the net  (HOTT)

```
✅  Id-by-observation (semantics): a terminating reduction system computing
    Id_A(x,y) by recursion on A — Bool/Nat structurally, product componentwise,
    function pointwise (funext), universe by univalence (→ Equiv) (id_observe.c)
✅  Transport as type-former rewrite (semantics): refl/constant family = identity;
    identity family over 𝒰 gives transport^(λX.X)(ua f) x = f x (30 checks)
🟡  Wire Id / transport into the net as agents (Id meets Σ/Π/𝒰 in the graph)
🟡  Cross-check the semantics against the in-tree cubical layer (tt_equality.c)
⬜  Dependent function / Σ family cases for transport
```

## Track C — Optimal reduction  (Δ-Nets, `src/deltanets.c`)

The newest model (Salvadori 2025): one n-ary **replicator** replaces all the
brackets/croissants of Lamping/Asperti–Guerrini, so delimiters never accumulate.

```
✅  Core interaction system (fan / eraser / replicator) + λ↔net translation + readback
✅  SOUND on the AFFINE fragment (linear + erasure): 700k random terms across
    depths 6..16 match the reference — zero mismatch, zero refusal
✅  AIRTIGHT on sharing (λ-K): read-back refuses a sharing fan-out rather than
    mis-reading it → zero wrong across ~2M random λ-K terms (was ~1 in 100k wrong)
✅  dn_affine / dn_linear decide the guaranteed fragment; Church succ/add verified
✅  Wavefront reducer + parallelism profile (dn_parallel): work/depth measured,
    wavefront result == sequential == reference
🟡  opt_core: Asperti–Guerrini engine + reference; full matched-bracket encoding
    is the remaining hard step (Δ-Nets supersedes it)
⬜  **Full λ-K capability** — turn refusals into correct answers (so `mul 2 3`
    succeeds). Needs the paper's canonicalization: leftmost-outermost order +
    unpaired-replicator merging/decay + phase-2 aux-fan replication + erasure GC.
    THIS IS THE MAIN OPEN RESEARCH ITEM.  (see docs/INET_ENGINE_PLAN.md §7)
```

## Track D — Parallelism → GPU / OpenCL  (the endgame)

Interaction nets are *intrinsically* parallel: every active pair is an independent
local rewrite. That is exactly what a GPU wants. The groundwork is in place.

```
✅  Strong confluence: LIFO and FIFO reach the same normal form in the SAME
    interaction count (54 terms) — the order-independence parallel scheduling needs
✅  Wavefront reduction (core engine): reduce each generation of disjoint redexes
    as one batch; work unchanged; reports parallel DEPTH and peak WIDTH. Branching
    recursion is highly parallel (fib 18: ~389 avg, peak 2266); tail recursion is
    sequential (gcd: 1.0) — checked vs sequential
✅  Wavefront reduction (Δ-Nets): same, on the optimal engine — e.g. mul 3 3
    work=20 depth=5 width=8 (~4× here; grows with term size)
✅  Flat, pointer-free representation already (agents are array indices) — the
    shape an OpenCL kernel needs
⬜  Real multithreaded CPU reducer: atomic port operations over the frontier,
    thread-safe arena. The first concurrency milestone before the GPU.
⬜  **OpenCL backend**: a redex-bag kernel — (1) scan the net for active pairs,
    (2) apply the whole batch in parallel with atomic CAS on ports, (3) compact /
    allocate new nodes from a per-workgroup arena, (4) repeat. The Bend/HVM payoff:
    HVM2 hits ~74,000 MIPS on an RTX 4090 vs ~400 single-thread.
```

### What already de-risks the OpenCL plan
- **Order-independence** is proven (strong confluence), so a parallel schedule
  cannot change the result — the wavefront tests confirm batch == sequential.
- **The reduction step is local**: an interaction only touches its two agents and
  their ports, so disjoint redexes have no data races (the basis for lock-free apply).
- **State is already flat arrays** (`g_kind[]`, port tables) — directly portable to
  GPU buffers; no pointer-chasing to unwind.
- **The frontier is explicit**: the wavefront reducer already extracts the batch of
  independent redexes each round — that batch IS the parallel kernel's work item set.

### The likely OpenCL shape (when we get there)
```
   kernel reduce_round(net):
     redexes = parallel_scan(net for principal-principal pairs)   // global frontier
     parallel_for r in redexes:                                   // one work-item per redex
        atomically claim r's two agents (CAS)                     // avoid double-apply
        apply interaction rule -> rewire ports, alloc new nodes   // local, race-free
     compact_dead_nodes(net)                                      // stream compaction
   repeat until no redexes
```

---

## Critical path (next steps, in order)

1. 🟡→✅  **Δ-Nets canonicalization (the hard one)** — implement merging + decay +
   phase-2 + erasure GC so λ-K refusals become correct answers. Validate each step
   against the reference oracle + fuzzer already in place. Unlocks general optimal λ.
2. ⬜  **Multithreaded CPU reducer** — atomic frontier reduction on `ic.c`; the
   correctness groundwork (confluence, locality) is done. The concurrency rehearsal
   for the GPU.
3. 🟡→✅  **Wire Id/transport into the net as agents** — make the type theory *run*
   on the engine, cross-checked against the cubical layer.
4. ⬜  **OpenCL backend** — the redex-bag kernel above. The massive-parallelism payoff.

## Validation discipline (applies to everything)
Every engine change is checked by **differential testing against an independent
reference** (a normal-order normaliser / the trusted kernel) over hundreds of
thousands of random terms, and the build is kept strict (`-std=c89 -Werror`
-pedantic + the full warning set). The rule is simple: **never ship a wrong result** —
when the engine cannot be sure, it refuses rather than guesses.
