# Lizard trusted-kernel audit (refinement track)

This is an audit of lizard's existing trusted kernel -- the foundational layer the whole system's soundness rests
on -- read directly from src/ (kernel.c, kernel.h, the tt_check family).  The goal is the foundational task for any
trusted kernel: map exactly what must be correct, measure it against the de Bruijn criterion (the kernel must be
small enough to read and believe), and identify where refinement is warranted before any C is touched.  We chose the
asymmetric design for the foundations; this audit confirms the existing kernel already embodies it.

## The trusted core, mapped

The kernel uses its own term representation (kterm_t), separate from the surface AST, and exposes the classic
minimal trusted triad.  Everything else -- inference heuristics, tactics, elaboration -- produces terms the kernel
RE-CHECKS, which is the trust boundary (stated in kernel.h: "Everything else ... produces terms that the kernel
re-checks").

The genuinely trusted functions, measured:

| function   | role                                   | lines (approx) |
|------------|----------------------------------------|----------------|
| kt_shift   | de Bruijn index shifting               | ~205           |
| kt_subst   | capture-avoiding substitution          | ~215           |
| kt_whnf    | reduction to weak head normal form     | 156            |
| kt_equal   | definitional (conversion) equality     | 123            |
| kt_infer   | bidirectional type inference/checking  | 501            |
| kctx + kt_alloc + sort/nat helpers     | contexts, allocation     | ~150           |

Trusted total: ~1,350 lines.

## De Bruijn criterion finding

kernel.h claims "~500 lines of code that MUST be correct."  The actual trusted surface is ~1,350 lines.  This is
NOT bloat: the gap is honest growth from the type formers added since that comment -- the cubical layer (Interval,
Path, PathLam, PathApp), higher inductive types, and the Sum/List/Maybe/Empty/Unit families, each of which adds
genuine cases to kt_infer/kt_whnf/kt_equal.  ~1,350 lines is still well within the de Bruijn criterion -- a single
auditor can read it in a sitting -- but the header comment should be updated to reflect the true figure.  Honest
recommendation: revise the kernel.h docstring from "~500" to the measured ~1,350, so the claim matches the code.

The term alphabet (kterm_tag_t) is complete and clean: VAR, SORT, PI, LAM, APP, SIGMA, PAIR, PROJ1/2, NAT/ZERO/SUCC
/NAT_REC, ID/REFL/J, BOOL family, UNIT, LIST/MAYBE/SUM/EMPTY families with eliminators, the cubical INTERVAL/PATH/
PLAM/PAPP, plus META (holes) and CONST (opaque axioms).

## Soundness spot-check (verified live)

A trusted kernel's value is that it says NO correctly.  Running the kernel's own regression (example 129):
modus ponens (app imp p : Q) ACCEPTED; composition (lam (x P) (app qr (app imp x)) : Pi (x P) R) ACCEPTED;
p : Q correctly REJECTED (p proves P, not Q); mis-ordered composition correctly REJECTED.  The accept-good /
reject-ill-typed behaviour -- the core soundness property -- holds.

## The asymmetric design, confirmed in the kernel

The interaction-net Floor 0 (sangaku's src/cas/inet.lisp) and this kernel are two DIFFERENT MACHINES that meet,
not one thing:

- kt_whnf is a tree-walking, substitution-based reducer (sequential).
- the interaction net is a graph reducer by local rewriting (parallelizable).

Both compute the same beta/eta reduction.  The honest relationship: the interaction net is the PARALLEL EVALUATION
STRATEGY; the kernel's kt_infer/kt_equal is the TYPING DISCIPLINE.  And the asymmetric vision -- "the agents ARE the
type formers" -- is already literally true here: the kernel's KT_PI / KT_LAM / KT_APP are the function
constructor/observer agents, KT_SIGMA / KT_PAIR / KT_PROJ are the pair constructor/observer, with introduction and
elimination as the construction/observation duality.  The kernel is the asymmetric system, checked; the interaction
net is the asymmetric system, executed in parallel.

## Refinement roadmap (honest, risk-ordered)

- R1 (this audit): map and measure the trusted core; update the de Bruijn docstring.  DONE.
- R2 (DONE): proved the CORRESPONDENCE -- the interaction-net reducer (cas/inet.lisp) and lizard's trusted kt_whnf
  (via kernel-reduce) agree on a corpus of closed lambda terms (identity, K, nested beta), with the kernel's own
  trusted equality (kernel-equal? = kt_equal) independently certifying the reducts, and the correctness boundary
  exhibited concretely (outside the one-source-of-duplication fragment the unlabeled net diverges).  The bridge is
  built and checkable: see cas/inetbridge.lisp and example 442.  Floor 0 is now a demonstrably faithful parallel
  evaluation strategy for the trusted kernel, not an orphan experiment.
- R2-typing (DONE): the companion result for typing -- the typed-port discipline (cas/inettype.lisp) is proven to
  agree with kt_infer: a net passes local wire-consistency iff the kernel accepts the corresponding term at the
  corresponding type (example 445).  So both halves of the net foundation -- reduction and typing -- are now anchored
  to the trusted kernel, by agreement, not by reimplementation.
- R2-dependent (DONE): the dependent fragment too -- cas/inetdep.lisp carries the dependent derivation and delegates
  the check to kt_infer, with the net verdict proven equal to the kernel verdict on genuinely dependent terms
  (polymorphic identity; a type family F with mk : Pi n. F n) including the discriminating rejection that (mk zero)
  does not have type (F (succ zero)) (example 446).  No net-native dependent checker -- soundness rests on the audited
  kernel.
- R2-modal (DONE, full agreement): the modal axis -- cas/inetmodal.lisp carries contextual-modal derivations and
  delegates to lizard's trusted S4 checker (infer-modal), with FULL acceptance-AND-rejection agreement demonstrated
  guard-free (example 447).  Acceptance via Box?, rejection via the error-object? predicate (the modal checker
  rejects by returning an error node).  Includes the 4-axiom on the accept side and the truth-vs-valid discrimination
  on the reject side (the S4 soundness heart).  The previously-reported limitation is REMOVED: it was a mistake
  (reaching for guard instead of error-object?), now fixed.
- R2-hott (DONE): the observational-equality core -- cas/inethott.lisp carries Id/Path/refl derivations and delegates
  to the trusted kernel, with the net verdict proven equal to the kernel verdict including the discriminating
  rejection that (refl a) does not prove (Id A a b) for distinct a,b (example 449).  Observational equality is tied
  to the co-universe side of the construction/observation duality.  No net-native equality checker -- soundness rests
  on the audited kernel.  Full univalence remains roadmap.
- R3 (later, HIGH risk): any extension of the kernel in C.  A trusted core is changed only with a specific goal and
  full re-checking; never casually.  The typed-port interaction-net discipline, if it becomes the kernel's evaluator,
  is an R3 change and must be proven to preserve the accept/reject behaviour audited above.

## The honest line

Lizard already has a real, clean, ~1,350-line trusted dependent-type kernel with cubical and higher-inductive
features, and it correctly rejects ill-typed terms.  It does not need to be rebuilt; it needs the disciplined
refinement above.  The interaction-net work is the parallel evaluation substrate that should be PROVEN faithful to
this kernel (R2), not a replacement for it.  The de Bruijn criterion holds; the one documentation gap (the ~500 vs
~1,350 line claim) is noted for correction.
