# Lizard's foundations: an interaction-net substrate toward a type theory

This document records the foundational layer and is explicit about which work is BUILT and verified versus which is
ROADMAP.  Two complementary lines of foundation work exist; both are described honestly below rather than collapsed
into a single tidy tower.  The discipline throughout: build each floor on a verified one, never announce upper
floors while standing on the foundation stones.

## Two lines, and how they relate

LINE A -- KERNEL-ANCHORED (the rigorous line for soundness).  Modules inet.lisp, inetbridge.lisp, inettype.lisp,
inetdep.lisp.  These build the interaction net and then PROVE its behaviour agrees with lizard's existing trusted
kernel (kt_whnf for reduction, kt_infer for typing), rather than reimplementing a checker.  Because soundness rests
on the audited kernel, this line is the trustworthy one: nothing here can be "subtly unsound" in a way the kernel
would not catch.

LINE B -- SELF-CONTAINED (the exploratory line).  Modules stnet.lisp, dtt.lisp, idt.lisp.  These implement a typed
lambda calculus over the net with their OWN checkers (simply-typed, dependent, identity types).  They are useful as
standalone explorations of how the type theory could be expressed natively on the net, but their checkers are not
anchored to the trusted kernel and so carry the usual risk of a hand-written checker.

The honest relationship: Line A is the one to trust and to build on, because it is anchored; Line B is a parallel
exploration of native-on-the-net expression.  Where they overlap (simple types, dependency) Line A is the
authoritative account.

## Line A, floor by floor (BUILT and verified, kernel-anchored)

FLOOR 0 -- inet.lisp.  A bare interaction-net reducer: the parallel local-rewriting analogue of the lambda calculus.
Four 3-arrow agents (LAM, APP, DUP, SUP) distinguished by PORT POLARITY, which DERIVES the legal interaction table
(principals interact only at opposite polarity -> the four pairs {LAM,DUP}x{APP,SUP}); matching decides annihilate
(LAM~APP beta, DUP~SUP copy-completion) vs commute (LAM~DUP lambda-copying, APP~SUP distribution).  Only a
computational system; no types.  The label bet (polarity without labels) is validated on the elementary-affine
fragment and its boundary found.  Example 441, golden cas_inet.

R2 -- inetbridge.lisp.  Proves the net's REDUCTION is faithful to lizard's trusted kt_whnf: a corpus of closed
lambda terms (identity, K, nested beta) reduces to the same normal-form class under both the net and the kernel,
with the kernel's own kt_equal certifying the reducts, and the correctness boundary (outside the
one-source-of-duplication fragment) exhibited concretely.  Example 442, golden cas_inetbridge.

FLOOR 1 -- inettype.lisp.  The typed-port discipline (the lambda-arrow corner): each port carries a simple type, a
wire is well-formed iff it joins a producer-of-T to an observer-of-T, so checking is LOCAL wire-consistency -- the
construction/observation duality as a property of every wire.  Proven to AGREE with kt_infer: a net passes
wire-consistency iff the kernel accepts the corresponding term at the corresponding type, verified on acceptance and
rejection.  Example 445, golden cas_inettype.

FLOOR 2 -- inetdep.lisp.  DEPENDENT types (the first cube axis).  Dependency breaks Floor 1's locality -- in
(Pi (x : A) B) the codomain may mention x, so a port's type depends on another port's value, and a naive local check
would be unsound.  So the net CARRIES the dependent derivation (carriers aligned with the agents) and DELEGATES the
check to the trusted kt_infer.  Proven: carriers read back exactly to kernel syntax; with a type family F and
mk : Pi(n). F n, the kernel accepts (mk zero):(F zero) but rejects (mk zero):(F (succ zero)); net verdict equals
kernel verdict on acceptance and the discriminating rejection.  Example 446, golden cas_inetdep.

FLOOR 3 -- inetmodal.lisp.  CONTEXTUAL MODAL TYPE THEORY (the modal axis): necessity (Box) with the valid/truth
context distinction of Nanevski-Pfenning-Pientka contextual modal type theory.  The net carries the modal derivation
(box / unbox) and delegates the check to lizard's trusted dual-context S4 modal kernel (infer-modal); Delta (valid)
survives box-entry, Gamma (truth) is dropped, and Delta's preservation across nested boxes is the S4 4-axiom.
Asserts FULL AGREEMENT -- acceptance AND rejection: the net accepts iff the trusted kernel accepts and rejects iff
it rejects, all guard-free.  Acceptance is read via Box? (an accepted modal term infers a Box type); rejection via
error-object? (lizard's modal checker rejects by returning an error node).  Demonstrated: the 4-axiom at nesting
depth 3 on the accept side (a valid hypothesis surviving (box (box (box x)))), AND the truth-vs-valid REJECTION on
the reject side -- (box x) accepted when x is valid, (box y) REJECTED when y is only true, same shape and opposite
verdicts purely by context, which is the soundness heart of strict S4.  (An earlier iteration wrongly reported the
rejection as uncapturable; it had reached for guard, unreliable in this build, instead of error-object?.)  Example
447, golden cas_inetmodal.

CO-UNIVERSE / REFLECTION -- inetreflect.lisp.  Makes operational the insight that the co-universe (the observation
side of the construction/observation duality) is the hidden structure behind reflection.  Built on Floor 3 because,
under Pfenning-Davies, necessity (Box) and reflection are one phenomenon: a necessary/valid/closed term is exactly
code that can be quoted and reflected upon.  Demonstrated on lizard's real homoiconic terms: observing a term's
structure (head, binder, subterms) is the construction -> observation direction; rebuilding from observed parts is
the contravariant observation -> construction direction; the round-trip is exact (witnessing the lattices are
genuine duals); and the modal Box carrier viewed as code is its surface term, with the code view itself observable
(reflection is recursive).  A demonstration of the duality tied to the anchored modal layer -- not a new trusted
typing rule.  Example 448, golden cas_inetreflect.

FLOOR 4 -- inethott.lisp.  HIGHER OBSERVATIONAL TYPE THEORY (HOTT): equality determined by OBSERVATION (functions
pointwise, pairs componentwise, the equality type computed from how inhabitants are observed) -- the observational
successor to MLTT, distinct from homotopy type theory.  This is exactly the CO-UNIVERSE side of the duality:
observational equality IS equality on the observation lattice, so it continues the co-universe/reflection line.  The
net carries the equality derivation (Id / Path / refl) and delegates the check to lizard's trusted kernel
(kernel-check over the Id and Path formers).  Verified (example 449, golden cas_inethott): the Id/refl/Path carriers
read back exactly to kernel syntax; the kernel accepts (Id A a a) and (Path A a a) at Sort 0 and (refl a) at
(Id A a a); the kernel REJECTS (refl a) at (Id A a b) for distinct a,b (refl cannot prove a false equation -- the
soundness heart of equality); the net verdict equals the kernel verdict on acceptance and rejection; and equality is
also read operationally as matching observations (the co-universe tie).  Honest scope: this is the observational-
equality CORE anchored to the Id/Path/refl machinery the kernel exposes; full univalence and the equality-of-equality
tower are deeper and remain roadmap.

FLOOR 5 -- inetunivalence.lisp.  UNIVALENCE: an equivalence between types IS a path between them
(ua : (A ≃ B) -> (Id U A B)).  ua TYPING is KERNEL-ANCHORED (corrected from the prior iteration, which wrongly said
the kernel had no ua/Equiv): the trusted kernel contains KT_EQUIV and KT_UA, handled in kt_infer/kt_whnf/kt_equal,
and it is SOUND -- lizard's kernel_soundness_test (which builds ill-typed kterms directly and confirms kt_infer
rejects them) now includes ua cases: it accepts (ua e):(Id (Sort n) A B) and REJECTS ua of a non-equivalence, ua at
wrong endpoints, and Equiv of a non-type.  So the ua TYPING layer has the SAME audited-kernel guarantee as Floors
1-4.  What remains surface/roadmap is ua COMPUTATION (transport across ua reducing through Glue), which the kernel
deliberately omits -- there is no transp/Glue/comp operator in the trusted core, so transport across ua is not even
expressible there.  Adding it would be a multi-hundred-line CCHM implementation in the trusted core and is NOT
faked.  Verified: sangaku example 450 (kernel-anchored ua typing + rejections + readback faithfulness, golden
cas_inetunivalence); lizard example 392 (surface-anchored COMPUTATION: id-equiv reduces to identity, Glue collapses,
ua computes); lizard kernel_soundness_test (the permanent ua-typing soundness guard).  The honesty is in the split:
TYPING kernel-anchored, COMPUTATION surface/roadmap.

## Line B (BUILT, self-contained, NOT kernel-anchored)

stnet.lisp (simply-typed, example 442/golden cas_stnet), dtt.lisp (dependent, golden cas_dtt), idt.lisp (identity
types, golden cas_idt).  These implement their own checkers over the net.  They pass their own goldens but are
exploratory: their soundness is not delegated to the trusted kernel.  Treated as parallel exploration, not as the
authoritative typed foundation.

## Roadmap -- NOT yet built, stated as conjecture

THE TWO LATTICES.  A universe lattice on the construction side (terms : types : U0 : U1 : ...) and a co-universe
lattice on the observation side (variables : bindings : contexts : telescopes).  The judgement Gamma |- a : A is the
contravariant PAIRING of a universe element against a co-universe element -- which is why "one implies the other"
has fixed handedness and is not a true symmetry.  Connects to categorical type theory (types fibred over contexts,
substitution contravariant).

THE DUALITIES as one phenomenon: construction/observation, syntax/semantics, |- / |=, checking/searching, P/NP --
each an instance of the same contravariant pairing.

THE REST OF THE CUBE.  The base (simply-typed) and the first axis (dependency) are built and kernel-anchored (Line A
Floors 1-2).  Polymorphism (terms on types), type operators (types on types), and a higher-observational type theory (HOTT --
the Altenkirch/Shulman-style observational successor, distinct from homotopy type theory) remain roadmap.  Lizard
has a cubical/CCHM layer and HITs already; HOTT specifically would be the OBSERVATIONAL-equality flavour, a natural
fit for the construction/observation duality (observational equality is equality defined by how terms are OBSERVED,
i.e. on the co-universe side).  Each, to be added soundly, must be anchored to the trusted kernel the way Floors 1-3
were.  Curry-Howard is the bridge earned at the top, not assumed.

## The honest line

The kernel-anchored line (A) is real and trustworthy: the net's reduction is proven faithful to kt_whnf and its
typing (simple and dependent) proven faithful to kt_infer, with no net-native checker that could be subtly unsound.
The self-contained line (B) is a parallel exploration with its own checkers and is not the authoritative foundation.
Everything above the first cube axis is roadmap, to be added only by the same anchoring discipline.


## The Glue transp rule — held-face (regularity) case (kernel-anchored)
transp <i>(Glue A (cofib b b) T e) g, with the face held throughout, reduces to transport along the underlying
type line transp <i>T g.  Sound because the Glue boundary makes Glue == T on the held face, so the Glue line is
definitionally the T line and the (already sound) T-line transp applies -- no equivalence inverse, no hcomp
correction.  Both Glue-transp degenerate slices are now in the kernel: the EMPTY-face case (reduces to transport
in A) and the HELD-face case (reduces to transport in T).  Kernel soundness 121/0.  The GENERAL Glue transp (a
varying/proper face) is the one remaining CCHM piece -- it needs the equivalence inverse + is-equiv coherence +
comp, which the kernel's opaque Equiv does not provide, and it is named as the frontier, not faked.

CURRENT STATE (this arc, updated each iteration).  The cubical scaffold described above has since been completed
and is being wired into transport, one provably-sound rule at a time, each with a short type-preservation proof
and a kernel_soundness_test guard.  Now resident and guarded: the full transport tier (constant line, Pi with
constant and varying domain via interval negation, Sum, non-dependent Sigma, and -- this iteration -- DEPENDENT
Sigma via the transport filler); interval negation ~, and interval MEET /\ and JOIN \/ (the distributive-lattice
operations, total and sound); the hcomp tower (empty, single-face, two-face overlap lattice); the complete Glue
type-former layer (Glue, unglue, equiv-fun/inv/eta/eps, mk-equiv, id-equiv, gtransp) at its sound degenerate
depths; transp seeing through both Glue faces; comp (heterogeneous composition) made TOTAL for this single-
cofibration signature -- it reduces in every case, delegating to transp and hcomp1; and a BIDIRECTIONAL pair-
checker so dependent pairs type-check.  Kernel soundness 170/0 (up from 121/0 when the paragraph above was
written), never regressing.  THE DEPENDENT SIGMA WIRING is the first of the three downstream cases: transp
<i>(Sigma(x:A(i)) B(i,x)) (a,b) = (transp <i>A(i) a, transp <i>B(i,q(i)) b) with the filler q(i)=transp
<k>A(k/\i) a, the meet boundaries k/\i0=i0 and k/\i1=k making q(i0)=a, q(i1)=a' definitional.  The remaining
frontier is the OTHER TWO downstream cases -- Path-type-line transport (needs an i-varying-partial comp) and the
general Glue transport on an undecided face (needs the coherence-composed correction) -- each named, not faked.
Honest scope flag: "comp is total" and "dependent Sigma is wired" are real and verified FOR THIS REPRESENTATION
(a single cofibration, the partial given at the i0 end); full CCHM comp with i-varying partial systems is not
implemented.

UPDATE (next iteration).  The i-VARYING-partial comp now has its forced fragment: comp's partial may be a genuine
i-varying line <i>u(i) (a valid section, u(i):A(i) at every i), and on a HELD face the composite reduces to the
partial at i1 -- comp <i>A(i) [held -> <i>u(i)] u0 = u(i1), compatibility u(i0)=u0 enforced.  This is a genuine
component of the eventual Path-line machinery.  It was also PROVEN that the two remaining wirings cannot be
short-cut: the naive Path-line reduction <j> transp <i>A(i) (p@j) has j=0 boundary transp(u(i0)), but the
required boundary is u(i1), and for a generic endpoint section these differ -- so the genuine boundary-pinning
Kan composition is mathematically necessary.  The kernel is SOUND at the frontier: transp over a Path-type-line
is well-typed (it infers Path A(i1) u(i1) v(i1)) and stays NEUTRAL; the general Glue transport on an undecided
face stays neutral.  Kernel soundness 171/0.  The remaining frontier is the disjunction-system Kan composition
both wirings need -- an hcomp/comp that genuinely composes over a (j=0)\/(j=1) varying system with the interior
filled by transp -- which the deliberately-degenerate hcomp1/hcomp2 are not.  Named, not faked.

UPDATE (Path-line transport wired).  comp2 -- a TRUSTED-kernel two-face i-VARYING composition -- now lands, and
transp over a Path-type-line AUTO-REDUCES through it: transp <i>(Path A(i) u(i) v(i)) p = <j> comp2 <i>A(i)
[(cofib j i0) -> <i>u(i)] [(cofib j i1) -> <i>v(i)] (p@j), a path in A(i1) from u(i1) to v(i1).  comp2's sound
treads: a DECIDED held face yields that face's partial at i1, an EMPTY system yields transp line u0, otherwise
NEUTRAL.  At the Path-line's boundary one face is held (pinning u(i1) / v(i1)) and the other empty; in the interior
both are empty (pointwise transp); for an abstract A the result is a well-typed neutral path whose endpoints
compute.  comp2's face compatibilities are discharged by the path-endpoint judgmental rule (p@i0=u(i0),
p@i1=v(i0)).  This delivers the wiring proved necessary last iteration (the naive pointwise shortcut gave the wrong
boundary).  Kernel soundness 176/0.  The single remaining frontier is the general Glue transport on an undecided
face (the coherence-composed correction).

UPDATE (general Glue transport: the introduction term and the decided-face computation).  The GLUE INTRODUCTION
glue A (cofib r b) T e u a : Glue A (cofib r b) T e is now a trusted-kernel constructor whose typing enforces the
coherence (equiv-fun e) u == a ON THE FACE -- exactly what makes unglue well-defined.  Off-face it reduces to a,
on-face to u; the eliminator beta unglue (glue .. u a) = a holds.  With it the full CCHM Glue-transport result
res = glue A(i1) [phi -> ..] t1 a1 (t1 = transp <i>T(i) g0; a1 = comp <i>A(i) [phi -> <i> equiv-fun(e(i),
transp<k>T(k/\i) g0)] (unglue g0)) is expressible, and was VERIFIED to type-check and reduce to t1 on a HELD face,
with a1|phi reducing (via the i-varying comp) to equiv-fun(e(i1), t1) -- discharging the coherence.  The general
UNDECIDED-face case is the precisely-mapped wall: the partial leg is a section defined only on phi (off phi, g0 is
a Glue element, not a T-element, so the T-filler is ill-typed), and the kernel's comp demands a TOTAL section --
so it needs PARTIAL-SECTION typing (a section over a cofibration), the named next step.  The kernel stays sound:
transp over a Glue-line on an undecided face is well-typed and NEUTRAL; an incoherent glue is rejected.  Kernel
soundness 182/0.

UPDATE (the partial-elements layer opens; general Glue transport computes on undecided faces).  Two advances.
(1) The CCHM Partial former Partial (cofib r b) A : Sort -- the type of partial elements of A on a face -- now
reduces to A on a held face, to Unit on an empty face, neutral otherwise.  (2) PARTIAL-SECTION TYPING via a
face-restricted context (new helper kctx_restrict): comp's i-varying partial section and the glue introduction's
member are checked in the context with the face imposed (the de Bruijn variable defining phi set to its endpoint),
which refines hypotheses whose types mention it -- a glue element's Glue type collapses to its T-component, so it
becomes a T-element on the face.  This realises the CCHM judgement Gamma, phi |- at the kernel level, and it
unlocks the GENERAL GLUE TRANSPORT on an UNDECIDED face: transp <i>(Glue A(i) phi T(i) e(i)) g0 auto-reduces to
glue A(i1) phi T(i1) e(i1) t1 a1 (t1 = transp <i>T(i) g0; a1 = comp <i>A(i) [phi -> <i> equiv-fun(e(i),
transp<k>T(k/\i) g0)] (unglue g0)).  Verified: the result type-checks; on a decided face it computes (off phi ->
transport in A, on phi -> t1); unglue of the result is a1 (beta).  Sound: the restriction refines types but
weakens no check -- an ill-typed partial body, a wrong-typed base, or an incoherent glue is still rejected.
Kernel soundness 188/0.  Remaining: systems [phi -> u] as first-class partial-element introduction, and faces
that depend on the transport variable.

UPDATE (systems introduction; the i-dependent-face frontier mapped).  The partial-elements layer gains its
INTRODUCTION form: psys (cofib r b) A a -- the single-face system [phi -> a] : (Partial (cofib r b) A) -- reduces
to the value a on a held face, to * : Unit on an empty face, neutral otherwise.  Its value is typed under the
face-restricted context (kctx_restrict), so a value well-typed only on the face is accepted while a wrong-typed
value (even on the face) is rejected.  Partial now has both a former and an introduction.  The frontier "faces
depending on the transport variable" was mapped: for Glue TRANSPORT an i-dependent face is not a transport (a
moving gluing locus is a COMPOSITION -- comp over a Glue line, strictly larger than transport), so the kernel
leaves it NEUTRAL and never fabricates a value; comp over a Glue line is likewise well-typed and neutral.  The
real remaining work is comp over a Glue line (the full Glue-composition) and multi-face systems with disjunction
cofibrations.  Kernel soundness 192/0.

UPDATE (comp over a Glue line: composition machinery advances; the Glue-composition wall re-mapped).  Two sound
comp2 improvements: (1) a one-empty-face comp2 drops to the single-face comp (the engine of the composition
filler's regularity); (2) an index-correct overlap-coherence check (where both faces hold the two partials must
agree), fixing a de Bruijn bug in sequential two-variable substitution.  An earlier diagnosis -- "comp2 is not
genuinely Kan, the filler collapses to u(i1)" -- was corrected: it used the undamped filler.  The correct CCHM
filler damps the partial by i/\l, Tfiller(i) = comp2 <l>T [phi -> <l>u(i/\l)] [(cofib i i0) -> <l>g0] g0, and is
VERIFIED to satisfy Tfiller(i) = u(i) on phi, so the Glue-composition overlap-coherence holds mathematically.
The remaining gap is narrow and precise: comp2's held-compatibility for the fully-nested two-face term (an inner
filler inside the psi-leg) is not yet uniformly face-restricted.  Decided-psi comp over a Glue line works (psi
empty -> comp in A, psi held -> comp in T).  comp over a Glue line on an undecided psi stays NEUTRAL; nothing is
fabricated.  Kernel soundness 194/0.

UPDATE (homogeneous Glue-composition: blocker diagnosed to its exact de Bruijn mechanism; kernel unchanged).
Going for the next step -- closing the homogeneous Glue-composition by fixing comp2's nested typing -- the
blocker was diagnosed precisely instead of patched fragilely.  The face-restricted SECTION check is index-
inconsistent when a partial leg directly consumes a refined glue variable: kt_subst(body, vidx, ep) decrements
the body's indices above the face var, but kctx_restrict keeps the matching context entries at their original
index, so a leg like equiv-fun(e, gg) (needing gg refined to its T-component) mis-resolves gg's older-than-face
neighbours and is rejected; a leg consuming gg only via unglue/transp type-checks.  This is exactly why Glue
TRANSPORT (unglue/transp legs) computes but Glue-COMPOSITION (a direct equiv-fun(e, filler) leg) does not.  It is
a localized de Bruijn fix -- a single index-consistent restriction (drop the face entry, matching depth, applied
uniformly to context, body, and expected-type/base) -- not a Kan deficiency (the damped filler is regular).  The
working transport path was protected: every candidate was tested against it and the suite, and kctx_restrict was
reverted byte-for-byte to the known-good form when the fix did not fully land.  Kernel soundness 194/0.

UPDATE (homogeneous comp over a Glue line: FIXED -- now type-checks AND computes; soundness 195/0).
The blocker described in the previous UPDATE is resolved.  The fix is a face restriction expressed as a context
DEFINITION rather than a substitution: kt_whnf already reduces a KT_VAR to its context entry's value, so
kctx_restrict now marks the face variable's entry as value = ep and substitutes nothing.  Endpoints are closed,
so nothing shifts and every de Bruijn index stays put -- the previous scheme's mismatch (kt_subst decrementing
the body while the context kept its entries) simply cannot arise.  comp, comp2, glue introduction, and psys were
simplified to stop substituting the term; comp2's overlap-coherence and held-compatibility became stacked
value-based restrictions.  With this, the full CCHM homogeneous Glue-composition res = glue A psi T e t1 a1
type-checks, and comp over a constant Glue line on an undecided psi auto-reduces to that glue element.  The
wiring is sound: with psi undecided (wiring active) but phi decided, the result reduces to u(i1) on phi-held and
to the base on phi-empty -- the genuine composition boundary.  Soundness rose to 195/0 with a new guard that
rejects an overlap-incoherent comp2 (two held faces with disagreeing partials).  The old cc.lisp "failure" was a
mis-built test (kernel-assume constants are not KT_VAR, so the restriction never fired).  Next: the heterogeneous
comp over a Glue line (Glue type varying in i) and multi-face disjunction systems.

UPDATE (heterogeneous Glue-comp mapped to its prerequisite; cofibration disjunction cofib-or added, wired into hcomp1; soundness 198/0).
Pushing both named frontiers: the heterogeneous comp over a Glue line was attacked empirically and mapped to
its exact prerequisite, and the multi-face axis gained a real sound primitive.  Heterogeneous obstruction: the
naive transport+comp2 recipe fails overlap-coherence under a varying equivalence -- on psi /\ phi the phi-leg
equiv-fun(e(i),.) must agree with unglue of the base (equivalence e(i0)), forcing e(i0)==e(i), false for varying
e; verified at the surface.  The genuine fix is the CCHM "pres" / equivalence-coherence construction.  The
empty-system (transport) and decided-system cases over a varying Glue line already compute; only the undecided
case needs pres.  New primitive: a first-class cofibration DISJUNCTION (cofib-or c1 c2) -- the lattice join,
holding if either disjunct holds and empty only if both are empty -- with full kernel support and a tight
well-formedness rule (operands must structurally be cofibrations).  It is WIRED into hcomp1: a homogeneous
composition over a disjunction face reduces to the partial when either disjunct holds, to the base when both are
empty, and the held-face compatibility is enforced on a held disjunct.  This is the brick multi-face systems are
built on.  Soundness rose to 198/0 (well-formed accepted, bare-point rejected, whnf normalises).  Next: the pres
construction (unlocking the heterogeneous case) and wiring cofib-or into comp and an N-ary system former.

UPDATE (heterogeneous comp over a Glue line TYPE-CHECKS: A,T,e varying; one sound kernel fix; soundness 198/0).
The comp over a Glue line whose TYPE varies in i -- A(i), T(i), and the equivalence e(i) all moving, with phi
constant -- now type-checks at Glue A(i1) phi T(i1) e(i1).  The previous "obstruction" was largely a setup
artifact (a glue type with a constant equivalence but a varying phi-leg); with the equivalence varying
coherently in both, the overlap-coherence holds and no separate CCHM "pres" primitive is needed -- a connection-
line filler Tfill(i) = comp2 <l>T(i/\l) [phi -> wt(i/\l)] [(cofib i i0) -> wt(i0)] (wt i0) plays that role.  The
one enabling kernel fix: comp2 now SKIPS the section check on an EMPTY face (its partial is vacuous and, for a
connection line, cannot have the section type away from the face); non-empty faces are still fully checked, and
a wrong-typed partial on a non-empty face, an overlap-incoherent comp2, and an incoherent glue are all still
rejected.  The full result res = glue A(i1) phi T(i1) e(i1) (comp <i>T(i) [phi->wt] (wt i0)) (comp2 <i>A(i)
[psi-> unglue u][phi-> equiv-fun(e(i),Tfill)] (unglue g0)) type-checks.  Soundness 198/0.  Next: the auto-
reduction wiring for this heterogeneous case, and a phi that itself varies in i.

UPDATE (heterogeneous comp over a Glue line now COMPUTES; soundness 198/0).
comp over a VARYING Glue line (A(i),T(i),e(i) moving; phi constant in i) now auto-reduces, not just type-checks.
comp's whnf intercepts a Glue line with phi constant-in-i (checked by lowering phi across the binder) and
undecided, and builds res = glue A(i1) phi T(i1) e(i1) t1 a1 with t1 = comp <i>T(i) [phi-><i>u] g0, the
connection-line filler Tfill(k) = comp2 <l>T(k/\l) [phi-><l>u][(cofib k i0)-><l>g0] g0 as CCHM "pres", and
a1 = comp2 <k>A(k) [cof-><k>unglue u][phi-><k>equiv-fun(e(k),Tfill(k))] (unglue g0).  Correctness gate (passed):
with phi undecided so the wiring fires, the system face held -> u(i1) and the system face empty -> transp of g0
along the Glue line -- the genuine composition boundary, not a fabrication.  A non-Glue varying line still uses
transp+hcomp1; a decided phi reduces via the Glue boundary; phi-not-constant is left conservatively to the
fallback.  Soundness 198/0; incoherent glue, overlap-incoherent comp2, and wrong-typed non-empty legs still
rejected.  Next: a phi that itself varies in i (needs phi(1) and the equivalence-family action), and multi-face
disjunction-system composition.

UPDATE (forall-face quantifier cofib-forall added; varying-phi Glue-comp held case wired; soundness 198/0).
comp over a Glue line whose gluing cofibration phi(i) varies in i is the case needing CCHM's forall-face.
Confirmed the current varying-phi behavior is sound (neutral), then added (cofib-forall i phi) = forall i.phi(i)
as a first-class cofibration with full kernel support and a CONSERVATIVE sound decision: HELD only when phi is
certainly held for all i (independent of i and held, or reflexive); EMPTY when phi certainly fails at an
endpoint; else neutral.  Wired the decided-HELD case into comp: when phi holds throughout, Glue = T(i)
everywhere, so comp over the Glue line reduces exactly to the composition in the T-line.  The forall-EMPTY
moving-face case (phi(i)=(cofib i i1)) needs the CCHM equivalence lemma and stays neutral (sound).  Soundness
198/0; incoherence still rejected.  Next: the equivalence lemma for the moving-face case; multi-face composition.

UPDATE (the interaction-net machine, bridged to the trusted kernel; kernel untouched, soundness 198/0).
Toward lizard becoming an interaction-net machine, the disciplined first step: verify the existing
interaction-combinator runtime (src/inet.c -- CON for lambda/application, DUP for sharing, ERA for
erasure, exact-GMP OPR/OP1 arithmetic; Lafont nets / Lamping-Gonthier optimal reduction / the HVM model)
and BRIDGE it to the audited tree-walking kernel, ALONGSIDE rather than replacing kt_whnf. The decisive
experiment passes (sangaku example 451): for Church numerals N=0..7, the trusted kernel beta-reduces
church_N s z to s nested N deep, and the interaction net reduces church_N succ 0 to the integer N -- two
unrelated engines, one answer. The net core (tests/inet_test.c, 25 assertions) also does S K K -> I,
Church addition, 10^24 arithmetic, de Bruijn readback, and an honest readback-refusal boundary. Surface:
inet-normalize / inet-cost (the sharing measure) / inet-reduce. docs/INET.md has the full account. Purely
additive: kernel soundness stays 198/0, kernel_test 21/0. Open frontier: carrying DEPENDENT/cubical typing
on the net substrate (the shared fragment here is untyped); near-term architecture is two-layer (net
evaluator + audited checker); replacing kt_whnf is not on the near path.
