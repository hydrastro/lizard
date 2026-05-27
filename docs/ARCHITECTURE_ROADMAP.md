# Lizard — Deep Architecture Roadmap

This document maps out the full feature gap between lizard and
production systems (Racket, Clojure, Lean/Coq, Cubical Agda),
identifies shared prerequisites, and defines the implementation
order.

## Principle: shared foundations first

Many features across the four paradigms share architectural
prerequisites. Building these foundations unlocks multiple
downstream features simultaneously.

```
                    SHARED FOUNDATIONS
                    ==================
    Syntax Objects ──────────────────────────┐
    │                                        │
    ├── syntax-rules + ellipsis              │
    ├── syntax-case                          │
    ├── macro phases                    Trusted Kernel
    ├── macro stepper                   │
    ├── #lang declarations              ├── elaborator
    ├── module namespaces               ├── holes/metavariables
    └── source-aware errors             ├── unification
                                        ├── inductive families
    Persistent Data ────────────┐       ├── termination checker
    │                           │       ├── proof state
    ├── persistent vector trie  │       ├── tactics
    ├── persistent HAMT         │       └── proof artifacts
    ├── transients              │
    └── parallel sequences      │   Cubical Core
                                │   │
    Concurrency ────────────┐   │   ├── full dependent comp
    │                       │   │   ├── face entailment
    ├── atoms               │   │   ├── general Glue/ua
    ├── refs / STM          │   │   ├── interval alpha-eq
    ├── agents              │   │   ├── HIT recursors
    ├── futures/promises    │   │   ├── higher inductive families
    └── thread-safe runtime │   │   └── canonicity theorem
```

---

## Track R: Racket-style macro system

### R.1 Syntax objects (FOUNDATION)

A syntax object wraps a datum with metadata:

```c
typedef struct lizard_syntax {
    lizard_ast_node_t *datum;       /* the actual value */
    lizard_env_t *lexical_context;  /* scope where it was defined */
    lizard_source_span_t span;      /* source location */
    unsigned int phase;             /* expansion phase (0 = runtime) */
    lizard_scope_set_t *scopes;     /* set of scopes for hygiene */
} lizard_syntax_t;
```

Key operations:
- `(datum->syntax ctx datum)` — wrap datum with context
- `(syntax->datum stx)` — unwrap
- `(syntax-e stx)` — extract inner datum
- `(syntax-source stx)` — get source location
- `(syntax-property stx key)` — extensible properties

Implementation: new AST node type `AST_SYNTAX` with the struct above.
The parser produces syntax objects (not bare AST nodes) when in
syntax-aware mode.

### R.2 Scope sets (hygiene mechanism)

Each syntax object carries a set of "scopes" — unique identifiers
added by the expander at each macro expansion step. Two identifiers
are `free-identifier=?` iff they have the same name AND the same
scope set.

```c
typedef struct lizard_scope_set {
    unsigned int *scopes;
    int count;
} lizard_scope_set_t;
```

This is the "sets of scopes" model from Matthew Flatt's 2016 paper,
which is simpler and more correct than the marks/substitutions model.

### R.3 syntax-rules with ellipsis

The existing `syntax-rules` is incomplete. Full implementation needs:
- Pattern matching with `...` (zero-or-more)
- Template instantiation with `...`
- Nested ellipsis (`(a ...) ...`)
- Literals list
- `_` wildcard

Pattern compilation: compile patterns to a matching automaton that
produces bindings, then instantiate templates from those bindings.

### R.4 syntax-case / procedural macros

```scheme
(define-syntax my-macro
  (lambda (stx)
    (syntax-case stx ()
      [(_ x ...) #'(list x ...)])))
```

Requires: syntax objects (R.1), scope sets (R.2), and a compile-time
evaluation environment (R.5).

### R.5 Macro phases

Racket separates compile-time (phase 1) from runtime (phase 0).
Macros execute at phase 1; the code they produce runs at phase 0.
Each phase has its own environment.

Implementation: add a `phase` field to the expansion context.
`(begin-for-syntax ...)` evaluates at the current phase + 1.
`(require (for-syntax ...))` imports into phase 1.

### R.6 Module namespaces and exports

```scheme
(module my-lib racket
  (provide square cube)
  (define (square x) (* x x))
  (define (cube x) (* x x x)))
```

Requires: syntax objects for proper hygiene across module boundaries.
The current `import` loads into the caller's environment; real modules
need isolated namespaces with controlled exports.

### R.7 Source-aware macro errors

With syntax objects, every piece of macro-produced code carries its
origin. Error messages can say "in expansion of `my-macro` at file.rkt:5:3".

### R.8 Macro stepper

A debugging tool that shows each expansion step:
```
Step 1: (my-macro 1 2 3)
     → (list 1 2 3)
Step 2: (list 1 2 3)
     → (cons 1 (cons 2 (cons 3 '())))
```

Requires: syntax objects with origin tracking (R.1 + R.7).

### R.9 #lang declarations

```scheme
#lang my-language
...code in my-language syntax...
```

The reader dispatches on the `#lang` line to load a language module
that provides the reader and expander. Requires: module system (R.6).

---

## Track C: Clojure-style persistent data & concurrency

### C.1 Persistent vector (trie)

A 32-way branching trie giving O(log32 n) ≈ O(1) access and update.
Each "mutation" produces a new root sharing structure with the old.

```c
typedef struct lizard_pvec_node {
    lizard_ast_node_t *entries[32];  /* leaf or child nodes */
    int shift;                        /* depth * 5 */
} lizard_pvec_node_t;

typedef struct lizard_pvec {
    int count;
    int shift;
    lizard_pvec_node_t *root;
    lizard_ast_node_t *tail[32];     /* right-edge optimization */
    int tail_count;
} lizard_pvec_t;
```

Operations: `pvec-ref`, `pvec-set`, `pvec-push`, `pvec-pop`,
`pvec-count`, `pvec->list`, `list->pvec`.

### C.2 Persistent HAMT (hash array mapped trie)

For maps and sets. Same 32-way branching, keyed by hash.

```c
typedef struct lizard_hamt_node {
    unsigned int bitmap;              /* which slots are present */
    void **entries;                   /* compressed array */
} lizard_hamt_node_t;
```

Operations: `hash-map`, `hash-get`, `hash-assoc`, `hash-dissoc`,
`hash-keys`, `hash-vals`, `hash-count`.

### C.3 Transients

Mutable versions of persistent structures for batch operations.
`(transient pvec)` → mutable; `(persistent! tvec)` → immutable.
Transients use an ownership token to ensure single-threaded mutation.

### C.4 Atoms

```scheme
(define counter (atom 0))
(swap! counter + 1)    ;; CAS-based atomic update
(deref counter)        ;; => 1
```

Implementation: `pthread_mutex_t` around the value pointer. `swap!`
retries on contention (spin loop with the update function).

### C.5 Refs / STM

Software transactional memory: multiple refs updated atomically.

```scheme
(dosync
  (alter account1 - 100)
  (alter account2 + 100))
```

This is the hardest concurrency feature. Requires: MVCC or
timestamp-based conflict detection, retry on conflict.

### C.6 Agents

Asynchronous actors: `(send agent f args)` queues an update.
Each agent has a single-threaded update queue.

### C.7 Futures and promises

```scheme
(def f (future (expensive-computation)))
(deref f)  ;; blocks until ready
```

Implementation: `pthread_create` + `pthread_join` wrapper.
Requires: thread-safe runtime (C.8).

### C.8 Thread-safe runtime

The runtime refactor (Phase 0) moved globals into `lizard_runtime_t`,
but the heap allocator is still not thread-safe. Needs:
- Per-thread heaps or a thread-safe allocator
- Lock-free or locked GC safe points
- Thread-local evaluation contexts

---

## Track K: Lean/Coq-style kernel & elaboration

### K.1 Trusted kernel boundary (FOUNDATION)

Separate the kernel (the ~500 lines of code that MUST be correct)
from everything else. The kernel:
- Checks that a term has a given type
- Reduces terms to normal form
- Compares terms for definitional equality

Everything else (inference, tactics, elaboration) produces terms
that the kernel re-checks. If the kernel is correct, the system is
sound regardless of bugs in the elaborator.

```c
/* The kernel API — these functions are the trusted core. */
typedef enum {
    KERNEL_OK,
    KERNEL_TYPE_ERROR,
    KERNEL_UNIVERSE_ERROR,
    KERNEL_REDUCTION_STUCK
} kernel_result_t;

kernel_result_t kernel_check(kernel_ctx_t *ctx,
                              kernel_term_t *term,
                              kernel_term_t *type);
kernel_term_t *kernel_reduce(kernel_ctx_t *ctx,
                              kernel_term_t *term);
int kernel_equal(kernel_ctx_t *ctx,
                 kernel_term_t *a,
                 kernel_term_t *b);
```

### K.2 Kernel term representation

The kernel uses its own term representation, distinct from the
AST used by the parser/expander. This is smaller and more regular:

```c
typedef enum {
    KTERM_VAR, KTERM_SORT, KTERM_PI, KTERM_LAM, KTERM_APP,
    KTERM_SIGMA, KTERM_PAIR, KTERM_PROJ1, KTERM_PROJ2,
    KTERM_ID, KTERM_REFL, KTERM_J,
    KTERM_PATH, KTERM_PATH_ABS, KTERM_PATH_APP,
    KTERM_COMP, KTERM_GLUE, KTERM_UNGLUE,
    KTERM_INDUCTIVE, KTERM_CONSTRUCTOR, KTERM_RECURSOR
} kterm_tag_t;

typedef struct kterm {
    kterm_tag_t tag;
    /* ... union of term data ... */
} kterm_t;
```

### K.3 Elaborator

Translates user-facing syntax into kernel terms:
- Inserts implicit arguments
- Resolves overloaded names
- Fills holes with metavariables
- Runs unification to solve metavariables

### K.4 Metavariables and holes

```
?0 : Nat → Nat    -- a hole in the proof
```

The elaborator creates metavariables for unknown terms and solves
them by unification during type checking.

### K.5 Unification

Higher-order pattern unification (Miller's algorithm) for solving
metavariables. Falls back to first-order unification when pattern
conditions aren't met.

### K.6 Inductive families

Generalize the current HIT declarations to full inductive families:

```
data Vec (A : Type) : Nat → Type where
  nil : Vec A 0
  cons : (n : Nat) → A → Vec A n → Vec A (succ n)
```

Requires: kernel term representation (K.2), dependent pattern matching.

### K.7 Termination checker

Ensure that all recursive definitions are structurally decreasing.
Without this, the type system is inconsistent (any type is inhabited).

Approach: size-change principle or foetus termination checker.

### K.8 Proof state and tactics

A proof state is a list of goals (metavariables with their contexts).
Tactics transform the proof state:

```
goal: ∀ n : Nat, n + 0 = n
tactic: induction n
subgoal 1: 0 + 0 = 0
subgoal 2: ∀ n, n + 0 = n → (succ n) + 0 = succ n
```

### K.9 Proof artifact discipline

Every proof produces a kernel-checkable certificate. The certificate
can be verified without the elaborator, tactics, or automation.

---

## Track Q: Cubical type theory completion

### Q.1 Full dependent composition

The current `comp` handles non-dependent cases only. The dependent
case requires:

```
comp^i A [φ → u] u0 : A(i1)
```

where `A : I → Type` is a line of types, and `u` and `u0` must
be adjusted along `A`. The key missing piece: transport along `A`
for the boundary condition.

### Q.2 Complete face entailment

The current face lattice handles basic cases. Full entailment needs:
- `(i = 0) ∧ (i = 1) ⊢ ⊥` (contradiction)
- `(i = 0) ∨ (i = 1) ⊢ φ` when `φ` follows from either case
- Disjunctive normal form for arbitrary face formulas

### Q.3 General Glue/univalence computation

The current Glue handles single-face cases. Full Glue needs:
- Multi-face Glue with systems
- Composition for Glue (the hardest reduction rule in CCHM)
- The full `ua` computation: `transport (ua e) x ≡ e(x)`

### Q.4 Interval-variable alpha-equivalence

Currently, interval variables are compared by name. Proper
alpha-equivalence for interval binders requires:
- de Bruijn indices for interval variables
- Or a name-swapping approach

### Q.5 Full HIT recursors and computation

The S¹ scaffold needs:
- `S1-rec C cb cl : S1 → C` with:
  - `S1-rec C cb cl base ≡ cb`
  - `S1-rec C cb cl (loop @ i) ≡ cl @ i` (requires `ap` of the recursor)
- Composition for S¹-typed terms

### Q.6 Higher inductive families

Generalize HITs to families indexed by other types:
```
data Quotient (A : Type) (R : A → A → Type) : Type where
  [_] : A → Quotient A R
  eq : (a b : A) → R a b → [a] = [b]
  trunc : isSet (Quotient A R)
```

### Q.7 Canonicity theorem

Prove (or at least test extensively) that closed terms of base type
reduce to canonical values. This is the key metatheoretic property
of cubical type theory.

---

## Implementation order

### Phase 1: Foundations (this session)
1. **Syntax objects** (R.1) — AST_SYNTAX node, datum/context/span
2. **Trusted kernel boundary** (K.1) — separate kernel API
3. **Persistent vector** (C.1) — basic immutable array

### Phase 2: First features
4. syntax-rules with ellipsis (R.3)
5. Kernel term representation (K.2)
6. Persistent HAMT (C.2)
7. Full dependent composition (Q.1)

### Phase 3: Elaboration
8. Metavariables and holes (K.4)
9. Unification (K.5)
10. syntax-case (R.4)
11. Face entailment (Q.2)

### Phase 4: Production
12. Module namespaces (R.6)
13. Inductive families (K.6)
14. Atoms and concurrency (C.4-C.8)
15. General Glue (Q.3)
16. Termination checker (K.7)
17. Tactics (K.8)
18. HIT recursors (Q.5)

### Phase 5: Polish
19. Macro phases and stepper (R.5, R.8)
20. #lang (R.9)
21. Proof artifacts (K.9)
22. Canonicity (Q.7)
23. STM (C.5)

---

## What exists today

| Feature | Status |
|---|---|
| Basic syntax-rules (no ellipsis) | partial |
| Modal logic (K/T/S4/S5/sym-S5) | checked |
| Propositional truncation | checked (3-arg scaffold + 4-arg checked) |
| S¹ | scaffold (no recursor) |
| Path types + cubical core | checked (non-dependent comp only) |
| Glue + ua | single-face only |
| HIT declarations | registry only (no computation) |
| Module system | basic (import with caching, no namespaces) |
| GC | segment-level collection |
| Bytecode VM | 30 opcodes with TCO |
| Structured diagnostics | 6 error paths carry source spans |
