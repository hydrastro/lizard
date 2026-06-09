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
the full λ-calculus. Identity types now **run as interaction-net agents** —
Id-by-observation computes by graph rewriting on the inductive + universe fragment,
matching the spec across a 200k-term fuzz. Parallelism is demonstrated (strong
confluence + a wavefront reducer on both the core engine and Δ-Nets). The two big
open fronts are (1) full λ-K *sharing* in Δ-Nets — turning refusals into correct
answers, which we've now shown needs the full phase-2 canonicalization (no
duplication shortcut) — and (2) the real multithreaded/GPU reducer.

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
✅  Id as interaction-net AGENTS (src/idnet.c): an ID agent's principal meets a
    type-former's principal and fires that former's structural Id-rule —
    ID·Unit→Unit, ID·𝒰(A,B)→Equiv A B (univalence), ID·(A×B)→componentwise (with
    UNPAIR projecting values), ID·Bool→case analysis, ID·Nat→RECURSIVE (Id_Nat of
    successors re-spawns Id_Nat of predecessors). Validated against id_nf (the
    spec): 19 worked cases + a 200k-term differential fuzz, 0 wrong, 0 refused.
    Id computes by observation AS GRAPH REWRITING — the duality thesis, literally.
✅  The function case in the net (src/idnet.c): Id_(A→B) f g = Πz:A. Id B (f z)(g z),
    funext, now reduces ON THE NET. The key move needs no β-engine: applying f,g to
    the fresh Π-binder z is, in de Bruijn, just their bodies (the λ's var 0 and the
    new Π's var 0 coincide), so the bodies are reused directly with var 0 = z. Each
    observation agent gained a NEUTRAL rule (observing a variable can't fire, so it
    emits a neutral Id node), and read-back became binder-aware (Π/var/neutral-Id).
    Covers constant, identity, structural (e.g. λ.S#0), into-product, and dependent
    Π codomains. Validated against the spec: 7 worked cases + a 100k funext fuzz
    (random A→B with constant & identity bodies), 0 wrong, 0 refused. (A body that
    applies something now reduces too — see β below — so the only sound limit left
    here is that the codomain must be closed; open codomains would need net-level
    shifting, not yet done.)
✅  TRANSPORT as interaction-net AGENTS (src/idnet.c): transport^(λi. T i) p x now
    reduces ON THE NET, dual to Id — a TR agent faces the family's BODY and recurses
    on its structure: a closed (constant) body → identity; a product body →
    componentwise (HoTT 2.6.4, duplicating the path); the path variable itself
    (identity family λX.X) → consult the path. Transport along refl is the identity
    for any family. The genuinely univalent case works by graph rewriting:
    transport^(λX.X)(ua f) x = f x, e.g. transport^(λX.X)(ua not) true = false and
    transport^(λX.X×X)(ua not)(true,false) = (false,true). This needed a minimal
    EVALUATOR on the net too — application + β (substitution via a generic net copy)
    and Bool elimination (`if`) — so the net actually computes `f x`. Validated
    against the spec: 6 worked cases + a 60k transport fuzz (constant/product/
    univalence), 0 wrong, 0 refused.
✅  Univalence in a FUNCTION family (semantics AND net) — the inverse path (HoTT
    2.9.4), previously the one deferred transport case. A univalence path now carries
    a genuine equivalence: a forward map f AND an inverse g (uae f g; the old ua f is
    the involutive case g = f). Transport recurses on the family with VARIANCE — the
    forward map at covariant occurrences of the type variable, the inverse at
    contravariant (function-domain) ones: transport^(λX. D[X]->C[X])(uae f g) h =
    λz. [f if C=X]( h ( [g if D=X] z ) ); so transport^(λX.X->X)(uae f g) h conjugates,
    λz. f(h(g z)). On the net this builds the result lambda and reduces it innermost-
    first. Doing so surfaced (and fixed) a latent bug in the net's call-by-name β and
    its copy/substitution: an identity-like body applied to a NON-value argument (a
    redex) was wired to the body variable's port instead of the argument's own output
    port; both now use up_port(result), which is unchanged for ordinary bodies but
    correct when the body collapses to a redex argument. Validated against the spec by
    a 60k fuzz (across X->X / D->X / X->D variance, involutive Bool equivalences AND a
    genuine non-involutive 3-cycle on Bool+Unit) plus worked cases, 0 wrong, 0 refused.
    The 3-cycle is the decisive test: cyc ≠ cyc_inv, so transport of the identity is
    the identity only if the inverse is routed to the contravariant slot (a wrong
    forward there would send inl t to cyc² inl t = inr *). A function family along a
    NON-univalence path is still soundly refused (no inverse to route), as is a nested
    occurrence of the variable (e.g. X*X in the domain).
🟡  Cross-check the semantics against the in-tree cubical layer (tt_equality.c)
✅  Id over a dependent Σ (semantics AND net): Id (Σx:A. B x)((a,b),(a',b')) for an
    INDUCTIVE first component A. The first-component path Id_A a a' is decided by
    observation — an Empty anywhere (a ≠ a') makes the whole Σ Empty; an all-Unit
    (a = a') means the path is trivial, transport is the identity, and the type
    contracts to Id (B a) b b'. The second-component type genuinely depends on the
    (equal) first value (e.g. B = λx. if x then Bool else Nat). On the net this is a
    decision agent (N_SIGD) facing the first-component Id result: Unit → the
    second-component Id, Empty → Empty, and a PRODUCT path collapses by chaining
    decisions (contractible iff every component is). The family is instantiated at a
    by the generic copy/substitution. Validated against the spec: worked cases +
    a 60k Σ fuzz (random inductive A incl. products, constant family B), 0 wrong,
    0 refused.
✅  Id over a Σ with a NON-INDUCTIVE first component (semantics AND net) — the case
    that the inverse-path transport just unblocked. For A = U the first-component path
    Id_U a a' = Equiv a a' is a genuine type, so the Σ-Id does not collapse: it is a
    genuine Σ of an equivalence path and a transported second-component equality,
    Σ(p: Id_A a a'). Id (B a')(transport^(λZ. B Z) p b) b'. The transport is along the
    bound path p, so it is the identity when B is constant and stays neutral otherwise.
    The SPEC computes this in full (including the dependent case, where the result keeps
    a neutral transport along the bound path — e.g. Id (Σx:U. x)((Bool,t),(Nat,0)) =
    Σ(Equiv Bool Nat). Id Nat (transport (λZ.Z) p t) 0; note this is non-trivial even
    when the two first-component types coincide, since Equiv A A has more than one
    element). The NET handles the case where the second component is CONSTANT (transport
    is the identity, and the second-component Id — already built and closed w.r.t. the
    new binder — is the body; N_SIGD facing an Equiv builds the Σ), and soundly REFUSES
    a genuinely dependent second component, which would need the body transported under
    the fresh path binder (a shift the net does not perform). Validated against the spec:
    worked cases + a 60k non-inductive-Σ fuzz (random first-component types, constant
    second component), 0 wrong, 0 refused; the dependent case is checked to refuse.
✅  Nat recursor / eliminator (semantics AND net): rec z s n — the elimination dual
    to the zero/succ constructors — computes by recursion on the scrutinee. rec z s 0
    → z; rec z s (succ m) → s m (rec z s m). The step s is λn.λr. … (n = predecessor,
    r = recursive result), so double/add/pred (value steps) and even (an `if` IN the
    step body) all run. On the net it is two agents: N_REC, plus a call-by-value
    step-forcer N_RSTEP that forces the recursive result to a value BEFORE the step is
    applied — this keeps duplication on the value fragment (the only fragment idnet's
    structural copy is sound for) and lets the eliminator branch correctly. The fix
    that made `if`-in-step work: a value-eliminator (if / rec / its forcer) whose
    principal meets a bound variable WAITS for β to substitute a real value instead of
    firing on the variable (Id/transport/Σ neutral-variable rules are unaffected and
    still fire). Validated against the spec: worked cases (incl. nested rec and
    recursor-result-into-Id) + a 60k recursor fuzz, 0 wrong, 0 refused; a neutral
    (free-variable) scrutinee is soundly refused. Limit: only VALUE arguments to the
    step's β are sound — general nested β still needs the fan duplication that Δ-Nets
    has and idnet does not; the CBV forcer is exactly the workaround for that.
✅  Lists — a parameterized, data-carrying inductive type (semantics AND net). Three
    pieces, mirroring the inductive-type pattern: construction (nil / cons),
    observation (Id over List by structural recursion on the cons spine), and
    elimination (foldr). Id (List E) xs ys observes the spines together: [] = []
    gives Unit, mismatched lengths give Empty, and cons h:t = cons h':t' gives the
    PRODUCT (Id E h h') * (Id (List E) t t') — so for equal lists it is a nested
    product of Units, and an Empty anywhere marks the difference (e.g. an element
    type of U yields genuine Equiv components, Equiv Bool Nat * Unit). On the net
    this is the two-agent observer dance (N_IDL faces xs; N_NSZ / N_CCS face ys
    carrying the element type and the held head/tail) plus an extra port (PORTS 4→5)
    so the cons case can hold y, output, element type, head and tail at once. foldr
    reuses the recursor machinery wholesale: N_LREC + the same call-by-value
    step-forcer (N_RSTEP), where the step sees the head and the recursive result, so
    length / count / `all` (an `if` on the head) / map-succ (which builds a list) all
    run. Validated against the spec: worked cases + a 60k Id-over-List fuzz (random
    element type, random lists, biased equal/unequal) + a 60k foldr fuzz, 0 wrong,
    0 refused; a neutral (free-variable) list spine is soundly refused. Same inherited
    limit as the Nat recursor: only value arguments to the step's beta are sound.
✅  Coproducts A + B (semantics AND net) — the categorical dual of the product, and
    the missing core type former (alongside ×, Σ, →, Π, List). Construction is inl /
    inr; observation is Id over the sum (inl a = inl a' observes Id A a a'; inr b =
    inr b' observes Id B b b'; a tag mismatch is Empty); elimination is case
    (case (inl x) f g = f x, case (inr y) f g = g y). On the net Id-over-sum is a
    two-agent observer (N_IDS faces x carrying both component types; N_INLS / N_INRS
    face y on the matching side), and case is a single agent that, on inl/inr, builds
    the application f x / g y and discards the other branch — the application then
    reduces by the ordinary beta, so no new evaluation machinery is needed and `if`s
    in the branch bodies fire after substitution (the value-eliminator-waits rule).
    Validated against the spec: worked cases (incl. U components -> genuine Equiv,
    product components, case feeding Id, and a case that *builds* a sum) + a 60k
    Id-over-sum fuzz + a 60k case fuzz, 0 wrong, 0 refused; a neutral (free-variable)
    sum is soundly refused.
🟡  Dependent families (semantics, id_observe.c): the remaining open cases are
    transport THROUGH a dependent Σ family, Id over a Σ whose first component is
    non-inductive (needs transport along a non-trivial first-component path), and
    transport through a dependent Σ family. (Transport of functions -- the inverse
    path -- is now DONE on both spec and net.) Dependent Π funext, product
    transport, and Id-over-Σ for inductive first components are done (above).
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
    Ruled out as a shortcut: unsharing-by-duplication at read-back would LOOP on
    ~all refused terms (measured 130/130, 147/148, 149/149 across depths 7–9) —
    the β-normal Δ-nets for sharing are genuinely cyclic, exactly as the paper
    says, so only the level-delta-guided phase-2 unfolding resolves them.
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
✅  GPU-dispatch model (Δ-Nets, dn_gpu): a faithful single-core model of one
    lock-free kernel dispatch — fires only a CONFLICT-FREE subset per round (no
    overlapping rewrites), so it reports the parallelism a GPU actually achieves
    without per-port atomics. Honest finding: on tight arithmetic chains the
    idealized wavefront width (5–10) collapses to ~2–3 realizable per dispatch
    (most redexes are adjacent), with the peak live working set measured. Result
    validated identical to sequential. The realistic blueprint for the kernel.
✅  Flat, pointer-free representation already (agents are array indices) — the
    shape an OpenCL kernel needs
✅  Real multithreaded CPU reducer (on the HoTT net, idnet.c — `idnet_id_nf_par`,
    `reduce_par`): SOUND lock-free parallel firing. The obstacle is locality — our
    substitution `cs` copies a whole subtree in one rule (a non-local macro-rule with
    an unbounded write-set), which breaks naive neighbourhood locking. So we fire only
    the LOCAL rules in parallel (whitelisted: Id-observation against Bool/Nat/Unit/
    Empty/U and the value testers — O(1) write-set, no cs, no keep_d), each worker
    CAS-claiming the redex's closed neighbourhood {a,b}∪neighbours so concurrent
    firings are provably disjoint and re-validating the pair under the claim; the
    subtree-copying rules (β, recursor/fold, Σ-construction, transport) run in a serial
    fallback sweep. Atomic node allocation. Correctness rests on interaction-net strong
    confluence + disjoint firing; VALIDATED differentially: 36k mixed reductions ×
    {2,4,8} threads with par == seq == spec, 0 divergences, plus a determinism stress
    (one term reduced 1500× at 8 threads, 0 variance — a race would show). Honest
    perf: at this scale/granularity it is overhead-bound, NOT yet faster (depth-13
    balanced product, ~49k interactions: ~4.3 ms seq vs ~4.7 ms at 2–8 threads) — the
    parallel fraction is small (structural decomposition + cs stay serial), the reducer
    re-scans the arena each round (a redex work-queue would fix that), and per-net work
    is tiny vs barrier sync. The hard part — provably race-free concurrent reduction +
    the claim-based disjoint-firing mechanism — is the GPU backend's foundation. See
    docs/ARCHITECTURE.md §5.
⬜  **OpenCL backend**: a redex-bag kernel — (1) scan the net for active pairs,
    (2) apply the whole batch in parallel with atomic CAS on ports, (3) compact /
    allocate new nodes from a per-workgroup arena, (4) repeat. The Bend/HVM payoff:
    HVM2 hits ~74,000 MIPS on an RTX 4090 vs ~400 single-thread. Next perf levers
    (from the CPU reducer above): a redex work-queue instead of full re-scans, and
    localizing the structural rules (incremental duplication, the Δ-Nets direction) so
    far more of the work fires in parallel.
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
3. 🟡  **Finish Id/transport in the net** — Id, the function case (funext, incl.
   dependent Π), transport (incl. computational univalence, via a minimal on-net
   evaluator: application/β + `if`), Id over a dependent Σ for inductive first
   components (a decision agent on the first-component path), AND the Nat recursor
   (rec, with a call-by-value step-forcer so `if`/value steps run), AND Lists (a
   parameterized inductive type: Id over List by observation + the foldr recursor,
   reusing the step-forcer), AND coproducts A+B (Id over the sum + the case
   eliminator), AND univalence in a FUNCTION family (transport along a genuine
   equivalence -- forward + inverse, the inverse path, validated by a non-involutive
   3-cycle), AND Id over a Σ with a non-inductive (universe) first component (a genuine
   Σ of an equivalence path and a transported second equality; constant second component
   on the net, the dependent case in the spec) now all run as agents in src/idnet.c.
   Remaining: transport through a dependent Σ family, and the cross-check against the
   cubical layer (blocked — needs the in-tree ds.h / tt_equality.c headers).
4. ⬜  **OpenCL backend** — the redex-bag kernel above. The massive-parallelism payoff.

## Validation discipline (applies to everything)
Every engine change is checked by **differential testing against an independent
reference** (a normal-order normaliser / the trusted kernel) over hundreds of
thousands of random terms, and the build is kept strict (`-std=c89 -Werror`
-pedantic + the full warning set). The rule is simple: **never ship a wrong result** —
when the engine cannot be sure, it refuses rather than guesses.
