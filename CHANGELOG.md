# CAS — axiom mode (simpler proving UX) + the ramified-place integral element (frontier pushed harder)

- Added `lib/cas/axmode.lisp`: an AXIOM MODE for lightweight theorem proving. Load axioms once (ax-assume facts, ax-assume-rule rules, ax-assume-not negative facts), then ax-check any statement for a three-valued verdict: proven (derivable), disproven (negation derivable -- never from mere absence), or independent (neither). A contradictory axiom set is flagged 'inconsistent. Built on logic.lisp's Horn-clause engine, so the verdicts are exactly the trusted engine's. From {human(socrates), mortal(X):-human(X)}: mortal(socrates) proven, mortal(zeus) independent, then disproven once (not (mortal zeus)) is added; multi-step ancestry resolved.
- Added `lib/cas/ramplace.lisp`: the RAMIFIED-PLACE integral element on y^2 = f, the local integral-closure generator at a cusp (a root of f of multiplicity m >= 2, where the branch is a Puiseux series in a fractional power) -- harder than the multi-branch node case. At x=a the valuations are v(x-a)=2, v(y)=m; the element w = y/(x-a)^floor(m/2) is integral with w^2 = f/(x-a)^{2 floor(m/2)} a polynomial (monic minimal polynomial certificate). Verified: cusp y^2=x^3 -> y/x, w^2=x (ramified); y^2=x^5 -> y/x^2, w^2=x; y^2=(x-1)^3 at x=1; even multiplicity y^2=x^2 splits (w a unit); a non-root point is unramified-regular. Place classified ramified/split/unramified-regular (local Riemann-Hurwitz data).
- Added examples 390 (axiom mode) and 391 (ramified place) and their goldens, with soundness controls (inconsistent set flagged; non-root yields no element).
- Updated the comparison with the two new rows; the roadmap's integral-closure horizon now reads: finite squarefree + nodes + ramified hyperelliptic places done; general-degree ramification and infinite places open.
- Zero regressions across the logic, integral-closure, and theorem-proving chains.

# CAS as theorem prover — certified definite integrals and the Dirichlet/sinc integral = pi/2

- Added `lib/cas/defint.lisp`: CERTIFIED DEFINITE INTEGRALS by the Fundamental Theorem of Calculus. For a polynomial integrand, computes the antiderivative F, discharges "F is an antiderivative of f" with the differentiation arbiter, evaluates F(b)-F(a) exactly, and emits a re-checkable proof record (a tampered value fails re-check). INT_0^1 x^2 = 1/3, INT_0^2 (3x^2+2x+1) = 14, etc., each a theorem.
- Added `lib/cas/dirichlet.lisp`: the Dirichlet integral INT_0^inf sin(x)/x dx = pi/2 (the sinc value), proved by the parameter-integral (Feynman) method -- NOT by an antiderivative (the integrand is non-elementary). I(s)=INT_0^inf e^{-sx} sin x/x dx; I'(s)=-INT e^{-sx} sin x=-1/(s^2+1) (Lemma A, Laplace transform certified from its antiderivative); I(s)=C-arctan(s) (Lemma B, arctan-derivative certified); boundary gives C=pi/2; I(0)=pi/2. The transcendental lemmas are certified by exact-agreement-at-samples; the algebraic backbone is exact; the proof record re-checks.
- Caught a real reader bug during construction: a quoted symbol containing parentheses (laplace-sine=1/(s^2+1)) broke the s-expression reader though the parens balanced lexically; fixed by removing the embedded parens from the symbol name.
- Added examples 388 (definite-integral theorems) and 389 (sinc/Dirichlet theorem) and their goldens, including soundness controls (tampered records rejected).
- The honest division of labor: the FTC case (defint) and the non-elementary parameter-integral case (dirichlet) are kept distinct, naming which arbiter certifies which step.
- Updated the comparison with the two theorem-proving rows. Zero regressions.

# CAS frontier — multi-branch combined integral element (general-curve integral closure)

- Added `lib/cas/vanhoeijmb.lisp`: the MULTI-BRANCH combined-correction integral element on y^2 = f, building and certifying an integral-basis element w = (A + B y)/d that is integral at a place where several branches meet at once -- the case vanhoeij.lisp deferred as 'needs-place-combination. Found by the exact integral-closure test in K = Q(x)[y]/(y^2-f): w is integral iff its minimal polynomial w^2 - (2A/d)w + (A^2-B^2f)/d^2 has polynomial coefficients (trace and norm both divide out). The norm sees all branches, certifying integrality everywhere simultaneously.
- Verified: at the node y^2=x^2(x+1) the element y/x (trace 0, norm -(x+1)) is the combined-branch element; at the two-node y^2=x^2(x-1)^2(x+1) the element y/(x(x-1)) is integral at both; non-integral candidates (y/x^2, y/(x-1) at a smooth point) are rejected with the failing trace/norm exhibited.
- Caught a zero-representation bug during construction: the certificate compared () vs (0) for the zero trace polynomial; fixed by comparing trimmed forms.
- Added example 387 and golden cas_vanhoeijmb with full coverage including the rejection controls.
- Updated the comparison (new multi-branch row) and roadmap (the integral-closure horizon now reads: finite squarefree + multi-branch nodes done; ramified and infinite places open).
- Zero regressions across the integral-closure, superelliptic, and aperiodicity chains.

# CAS last rung closed — unconditional aperiodicity proof for the third-kind integral

- Added `lib/cas/hyperaperiodic.lisp`: an UNCONDITIONAL certificate that y^2 = f has no Pell unit -- hence that INT dx/sqrt(f) is non-elementary -- by traversing the full cycle of complete quotients of the continued fraction of sqrt(f) until a pair (P_i, Q_i) repeats. A closed cycle with no nonzero-constant Q (past the trivial start) is a finite PROOF of non-elementarity, converting polycf's bounded "aperiodic-up-to-B" into a real theorem. Both directions of the third-kind elementarity decision are now closed: periodic gives the explicit logarithm, proven-aperiodic gives non-elementarity.
- Cross-checked by independent agreement: y^2=x^6+1 has the unit (x^3,1) (elementary, torsion), y^2=x^6+x^2+1 closes its cycle with no unit (proven non-elementary, non-torsion) and the bounded polycf search independently agrees (aperiodic up to 100). x^6+x^4+1 and x^6+x+2 likewise proven non-elementary.
- Caught a subtle bug during construction: the trivial first complete quotient (P_0,Q_0)=(0,1) has Q_0=1 constant by construction and must be excluded from the unit test (else every curve looks periodic); the test runs over the tail of the cycle only.
- Added example 386 and golden cas_hyperaperiodic with full coverage including the elementary/non-elementary split and the bounded-vs-unconditional cross-check.
- The Trager ladder now shows zero open rungs: every rung has a sound certified core, with the full third-kind construction at summit and the genuine research horizon (uniform period bound per genus) named honestly in the roadmap.
- Zero regressions across the Pell, continued-fraction, Jacobian-torsion, and hyperelliptic chains.

# CAS frontier — hardening the Pell unit engine: square guard + reverse-CF cross-check

- Added a perfect-square guard and explicit unit classification to `lib/cas/polycf.lisp`: pcf-is-square? flags curves where sqrt(f) is a polynomial (no Pell unit), and pcf-unit-status returns one of 'square / (unit A B) / 'unit-unverified / 'no-unit-up-to -- so every verdict is explicit rather than a degenerate unit-unverified. Verified x^4+2x^2+1=(x^2+1)^2 is flagged 'square.
- Added a reverse-CF round-trip cross-check (example 385): a unit (A,B) with constant norm c determines f=(A^2-c)/B^2, and the continued fraction of that f must independently recover a certified unit. This validates the unit-finder against the opposite construct-from-unit direction -- two independent methods agreeing. Verified the engineered curve x^6+2x^4+3x^2+2 from (x^4+x^2+1, x, 1) is recovered by the CF with exactly that nonconstant-B unit, plus several further engineered units round-tripping.
- Over Q, polynomial Pell with period >= 3 is rare (none found in a wide small-coefficient scan), so the reverse-CF construction is the rigorous validator of the convergent/norm machinery beyond period 1.
- Added golden cas_pellcrosscheck; existing cas_polycf golden unaffected (additive functions).
- Updated the comparison with one new capability row.
- Zero regressions across the polycf, Pell, third-kind, and algebraic chains; every positive result certified.

# CAS frontier — period-2 Pell units fixed, and a CF-driven third-kind construction for any periodic curve

- Fixed a convergent/iteration-indexing bug in `lib/cas/polycf.lisp`: the Abel recurrence must start from the state AFTER a0 (P_1 = a0, Q_1 = f - a0^2), not from (P=0, Q=1) which recomputed a0 and shifted every partial quotient. With the fix, genuine period-2 curves certify: y^2 = x^6 + x now has period 2 and a certified fundamental unit (2x^6+1+..., 2x^2) of constant norm, where before it was honestly deferred as unit-unverified. The period-1 family (incl. f=h^2+c) is regression-clean. Updated example 383 and golden cas_polycf to reflect the period-2 certification.
- Added `lib/cas/hyperpellcf.lisp`: the CF-driven genus-2 third-kind Pell construction. Given ANY hyperelliptic curve y^2 = f, it asks polycf for the certified fundamental unit (A,B), builds g0 = A + B y in the genus-agnostic field (algfunc), and produces INT ((g0^n)'/g0^n) = log(g0^n), each gated by the differentiation certificate. Generalizes hyperpell past the f=h^2+c family to every periodic curve, including genuine period-2. Verified on y^2=x^6+x (period 2, not h^2+c) for n=1,2,3, agreement with hyperpell on x^6+1, and an honest no-unit for a non-periodic curve.
- Added example 384 and golden cas_hyperpellcf with full coverage including the soundness control.
- Updated the comparison (one new row) and the Trager ladder (the open third-kind rung now records the CF-driven construction and period-2; longer periods at higher genus remain).
- Zero regressions across the polycf, Pell, third-kind, and algebraic chains; every positive result certified.

# CAS frontier — the continued fraction of sqrt(f): deciding a hyperelliptic curve's Pell unit

- Added `lib/cas/polycf.lisp`: the continued fraction of sqrt(f) over Q[x] (Abel's algorithm), with periodicity detection and the fundamental-unit convergent -- the function-field analogue of the numeric CF for sqrt(N). Computes polypart(sqrt f) by coefficient matching, iterates the complete-quotient recurrence, detects periodicity (Q returns to a constant), and reads the fundamental Pell unit off the convergent. Every unit is gated by its norm A^2-B^2 f being a nonzero constant (pcf-certify-unit, pcf-unit-verified): the period-1 family (incl. f=h^2+c) is fully certified; higher even periods return unit-unverified (never a wrong unit); curves not closing within the bound return no-unit-up-to (honest bounded negative). Verified the certified units of x^6+1 (x^3,1 norm -1), (x^3+x)^2+2, the genus-0 x^2+1, and the honest deferrals for x^6+x (period 2) and x^6+x^2+1 (aperiodic in bound). This lets the genus-2 third-kind Pell construction work for curves where the unit is not visible by inspection.
- Fixed a convergent-indexing bug found during testing (the fundamental unit uses the first L quotients a_0..a_{L-1}, not the period-closing quotient); caught because the unit norm was non-constant, then verified against hyperpell.
- Added example 383 and golden cas_polycf with full coverage including the soundness deferrals.
- Updated the comparison with one new capability row.
- Zero regressions across the Pell, third-kind, algebraic, and continued-fraction chains; every positive result certified.

# CAS frontier — genus-2 nonconstant-B third-kind via the function-field Pell unit

- Added `lib/cas/hyperpell.lisp`: the genus-2 nonconstant-B third-kind construction. On the family f = h^2 + c the element g0 = h + y is a fundamental unit (constant norm -c); its powers g0^n = A_n + B_n y have B_n nonconstant (n>=2) and norm (-c)^n, each a third-kind logarithm argument INT ((g0^n)'/g0^n) = log(g0^n), certified by the norm relation AND by differentiation in the field (algfunc, genus-agnostic). For deg h = 3 these are genuine genus-2 curves. Verified on y^2=x^6+1 (unit x^3+y, norm -1; g0^2=(2x^6+1,2x^3); g0^3), a second curve (x^3+x)^2+2, and a non-unit rejection. This is the genus-2 companion to the genus-0 elliptic3pell construction, completing the third-kind construction past the a+y shape at genus 2.
- Added example 382 and golden cas_hyperpell with full coverage including the soundness control.
- Updated the comparison (one new genus-2 row) and the Trager ladder (the open third-kind rung now records nonconstant-B at genus 2 via the Pell unit; arbitrary genus / non-periodic sqrt(f) remain).
- Zero regressions across the third-kind, algebraic, and hyperelliptic chains; every positive result certified.

# CAS frontier — genus-2 third-kind logarithm + unified hyperelliptic integration driver

- Added `lib/cas/hyperthird.lisp`: the genus-2 (and general hyperelliptic) third-kind logarithm INT (g'/g) dx = log(g) for g = a(x)+y on y^2=f. Reuses algfunc (genus-agnostic field/derivation) and the tested esp-poly-sqrt; recovers a from the differential's denominator via the norm a^2-f, certifies by the cleared identity (a+y)*omega = D(a+y). Verified on y^2=x^5+1 (a=x, x^2, x^2+1), soundness rejection, and the genus-1 cross-check. Generalizes sethird to arbitrary-genus hyperelliptic. Advances the full third-kind construction into genus 2.
- Added `lib/cas/hyperint.lisp`: the unified hyperelliptic integration driver -- one entry point that dispatches the second-kind (hyperell: elementary Q sqrt(f) or first-kind non-elementarity proof) and third-kind (hyperthird: logarithm) cases over y^2=f at any genus, returning a single certified verdict. Verified second-kind elementary + non-elementary, third-kind logarithm, honest non-recognition, and identical decisions across genus 1 and 2. Advances the general algebraic Risch for the hyperelliptic family.
- During development, caught a bug in a from-scratch polynomial square root (failed on arguments with intermediate terms) and fixed it by reusing the already-tested esp-poly-sqrt rather than duplicating the algorithm.
- Added examples 380-381 and goldens with full coverage including soundness controls.
- Updated the comparison (two new genus-2 rows) and the Trager ladder (the open third-kind rung now records the explicit genus-2 logarithm; arguments beyond a+y and arbitrary genus remain).
- Zero regressions across the algebraic, third-kind, and hyperelliptic chains; every positive result certified.

# CAS third-kind frontier — the genus-2 Jacobian: group law + torsion-based elementarity decision

- Added `lib/cas/hyperjac.lisp`: the Jacobian group law of a genus-2 hyperelliptic curve y^2=f (deg 5) by Mumford representation and Cantor's algorithm (composition + reduction over Q[x], with a from-scratch extended gcd). The identity [1,0], points [x-a,b], negation [u,-v], addition, doubling, scalar multiples; every result certified by the Mumford curve condition u | (v^2-f). Verified P+0=P, P+Q=[x^2+x,x+1] on y^2=x^5+1, P+(-P)=0, and Weierstrass 2-torsion.
- Added `lib/cas/hyperjactor.lisp`: the genus-2 third-kind torsion decision -- the genus-2 analogue of the elliptic order test. The class [P]-[iota P] is elementary iff torsion; computed as the order of P under the Jacobian group law by bounded search, reporting (torsion n) or an HONEST (no-torsion-up-to B) -- never a false non-elementary claim. Verified a Weierstrass point is order 2, (0,1) on y^2=x^5+1 is order 5 (elementary), the soundness bounded-miss, and a cross-check on y^2=x^5-x.
- Added examples 378-379 and goldens with full coverage including the soundness controls.
- Updated the comparison and the Trager ladder: two new genus-2 capability rows; the open third-kind rung now records genus-2 progress (group law + torsion decision done; explicit genus-2 logarithm and arbitrary genus remain).
- Zero regressions across the third-kind, algebraic, and RUNG-5 chains; every positive result certified.

# CAS RUNG 5 — stacked algebraic towers: the field of x^(1/4) and its third-kind logarithm

- Added `lib/cas/algtower2.lisp`: a tower of two algebraic extensions Q(x)[y][z]/(z^2-y, y^2-x), the field of x^(1/4). Element arithmetic (z^2->y reduction) and the certified derivation D(a+bz) = a' + (b' + b/(4x))z, reducing soundly to algfunc's inner derivation plus the scalar z'/z = 1/(4x). Verified z^2=y, z^4=x, y'=y/(2x), and INT (5/4)x^(1/4) = x^(5/4) by differentiation in the tower.
- Added `lib/cas/algtower2log.lisp`: the field inverse (via the outer conjugate, one inner inverse) and the third-kind logarithm INT (e'/e) dx = log(e), certified by the inverse-free cleared identity e*(e'/e) = D(e). Verified log(1+x^(1/4)), log(x^(1/4)), log(sqrt x), with sound rejection of non-logarithmic differentials.
- Added examples 376-377 and goldens with full coverage including soundness controls.
- Corrected the comparison chart: the algebraic-Risch frontier (previously a single stale 'none') is now split into the nine certified sub-capabilities actually built (hyperelliptic decision, algebraic Hermite, genus-1 elliptic logs, Puiseux/integral basis, superelliptic family, van Hoeij, mixed towers, the double tower), leaving exactly ONE honest 'none' row: general algebraic Risch at arbitrary genus/tower, the open summit. The two height-two 'none' rows are corrected to 'part' (single-residue cases certified; only the general multi-residue x-dependent case is memory-bound).
- Zero regressions across the algebraic and RUNG-5 chains; every positive result certified by differentiation.

# CAS frontier — genus-0 algebraic integration (first rung of the algebraic-case Risch problem)

- Added `lib/cas/algquadint.lisp`: the genus-0 algebraic integral INT (px+r)/sqrt(ax^2+bx+c) dx, the first rung of the algebraic-case Risch frontier (where lizard was at 'none' and even Maxima only 'part'). The linear numerator splits into a certified algebraic (second-kind) part (p/2a) sqrt(q) plus a first-kind part: a logarithm (arcsinh form) when a>0, an arcsine when a<0 with the radicand having real roots. Every closed form certified by differentiation; integrands with no real form reported 'no-real-form. The arcsine condition (a<0 AND discriminant>0, argument (2ax+b)/sqrt D) was derived and verified via D-(2ax+b)^2 = -4a q.
- Added example 374 and golden cas_algquadint with full coverage (log case, pure-algebraic case, mixed case, two arcsine cases, and the honest no-real-form control).
- Implemented from the published algorithms (Euler substitution, classical first/second-kind reduction) from scratch with certificates -- not adapted from any GPL source.
- Zero regressions across the elliptic, hyperelliptic, Hermite, and rational-integration chains.

# CAS ladder — three rungs climbed: complex tuples, superelliptic integral basis, F4 reduction

- Added `lib/cas/cplxtuples.lisp`: complete complex solution tuples over the Gaussian rationals Q(i). Gaussian-rational arithmetic (gr-), perfect-square detection turning a (complex re im2) into its two Q(i) roots, triangular-system assembly into complete complex points, certified by evaluating every generator to zero in Q(i). Non-rational imaginary parts reported 'not-gaussian. Climbs the complex-coordinates ladder rung.
- Added `lib/cas/superintbasis.lisp`: the integral basis at the finite places of a superelliptic curve y^n = f. For squarefree f the certified power basis {1, y, ..., y^(n-1)}, each element integral via its monic defining polynomial t^n - f^k, with discriminant n^n f^(n-1) and maximality decided by the squarefree test; non-squarefree f reported non-maximal with the repeated factor. Climbs the degree>2 integral-closure ladder rung.
- Added `lib/cas/groebnerf4.lisp`: the linear-algebra reduction at the heart of F4. A Macaulay matrix (columns = monomials sorted descending, rows = polynomials) is row-reduced to RREF over Q; the nonzero rows read back are the reductions, pivot columns the leading monomials. Exact over Q, with row-space preservation certified. Climbs the F4-class-engine ladder rung.
- Added examples 371-373 and their goldens with full coverage including honest-deferral controls.
- Updated the three infographics: the Trager ladder now shows a single remaining open rung (the full third-kind / Jacobian-torsion construction), with the three climbed rungs at summit; the comparison and roadmap reflect the new certified capabilities.
- Zero regressions across the Groebner, superelliptic, and complex chains; every positive result certified.

# CAS summit — exponential single-logarithm completion + open-source surface

- Added `lib/cas/tower2expfull.lisp`: the exponential single-logarithm recognizer that completes the proper-fraction branch of the height-two exponential integrator. A squarefree remainder As/Ds is recognized as the single logarithm c·log(Ds) exactly when As = c·D2(Ds) for a tower constant c, found by one polynomial division over K1[theta2] (no Sylvester resultant) and certified by differentiation. This closes the case the unified driver's power-sum wrapper previously rejected whenever the denominator was squarefree but not a bare power theta2^j (e.g. theta2 - e^x), promoting the exponential second-monomial capability from partial to a genuine integrator on the single-log case. The multi-residue x-dependent RootSum continues to route through the dedicated fraction-free resultant integrator (`tower2expff.lisp`) to avoid memory blow-up.
- Added `examples/370-exponential-single-log.lisp` and `tests/cas_tower2expfull.expected` with full coverage including the honest-deferral control (`notrecognized`).
- Added `CONTRIBUTING.md` documenting the build/test commands, library-module structure, the per-feature workflow (verify-first, additive-safety grep, module + example + golden + docs), and the certificate-and-honest-scope discipline.
- Corrected stale counts in `README.md` (now 271 library modules, ~26,000 lines of Lisp, 371 examples) and fixed a dead documentation link (`docs/ROADMAP.md` -> the actual `docs/MASTER_PLAN.md` and `docs/TRAGER_ROADMAP.md`); added a `CONTRIBUTING.md` pointer.
- Zero regressions across the height-two tower and the CAS chains; every positive result remains certified by differentiation.

# Phase 3F — tokenizer source wrapper + explicit GC metadata classification

- Fixed `lizard_tokenize_source` link failure by implementing the tokenizer-source wrapper declared in `tokenizer.h`.
- Source-tokenization diagnostics now preserve caller-provided filenames instead of always reporting `<string>`.
- Added `lizard_heap_alloc_tagged` so constructors can register explicit GC object kinds instead of relying only on size inference.
- Refactored core AST/list-node constructors in `mem.c` to use explicit GC metadata classification.
- Extended GC metadata stats with per-kind counters.
- Added tokenizer source diagnostic regression coverage.
- Extended ownership audit to require explicit GC classification scaffolding.
- Kept collector behavior non-moving and unchanged.

# Phase 3A — object model / value ownership audit

- Added `src/object_model.c` / `src/object_model.h` with ownership and tracing-policy metadata for heap, C-owned, borrowed, static, and context-owned objects.
- Added `tests/object_model_test.c`.
- Added `scripts/check-ownership-audit.py` and `make ownership-audit`.
- Wired `ownership-audit` into `make ci`.
- Added `docs/OWNERSHIP.md` documenting current ownership rules and the object-level non-moving GC transition target.
- Kept allocator/GC behavior unchanged; this is a strict metadata/audit scaffold.

# Phase 2Y — include graph / layering audit

- Added `scripts/check-include-layers.py` to audit public/internal header layering.
- Added `make include-audit` and wired it into `make ci`.
- Added `docs/INCLUDE_LAYERS.md` describing the public API, wrapper, implementation-root, tooling-leaf, and implementation-header layers.
- The include audit detects public headers leaking private headers, implementation headers including the public wrapper, parent-relative includes, selected leaf headers depending on the interpreter core, and cycles in the `src/*.h` quoted include graph.
- Kept all strict/security warning flags intact.

# Phase 2X — syntax/header boundary audit

- Fixed the `surface_term.h` include-order regression by requiring the header to
  include `lizard_api.h` before exposing `lizard_expansion_trace_event_t`.
- Added `scripts/check-header-boundaries.sh` to scan `src/*.h` for public API
  types and verify direct `lizard_api.h` inclusion.
- Added `make header-audit` and wired it into `make ci`.
- Kept all strict/security warning flags intact; no warning suppressions added.

# Changelog

## Documentation refresh + proof-producing CAS

- Rewrote `README.md` to reflect the current state: the four research
  tracks (R/C/K/Q, all feature-complete at the library level), the
  45-module standard library, the self-hosting demos, and bignum
  performance (within ~200ms of MIT Scheme on the 123^12312 loop).
- Added `docs/USAGE.md` — a getting-started guide and library recipes.
- Added `docs/CAS.md` — the proof-producing / verified CAS architecture
  and a roadmap connecting the CAS to the trusted kernel and ZFC.
- New `lib/cas.lisp` — symbolic algebra: simplification, differentiation
  (constant/variable/sum/product/quotient/power/chain + sin/cos/exp/ln),
  and basic polynomial integration.
- New `lib/cas-proof.lisp` — a layered foundation database (ZFC axioms →
  real construction → field axioms → limits → derivative → calculus
  rules), derivation trees with rule citations, and unfolding of any
  CAS result down to the ZFC axioms it rests on.
- New examples 125 (symbolic CAS) and 126 (a derivative unfolded to ZFC).

# Lizard v5 — engineering infrastructure (in progress)

This section logs the major infrastructure work built on top of the v4
type-theory + diagnostics baseline.

## Phase 0: Runtime refactor (B.1–B.2)

- Added `lizard_runtime_t *runtime` back-pointer on `lizard_heap_t`, so
  any function holding the heap can reach runtime state via `heap->runtime`.
- Moved 9 process-global variables into `lizard_runtime_t`:
  `gensym_counter`, `sr_counter`, `callcc_buf/active/value`,
  `logic_config_head`, `logic_last_set_bundle`, `hit_registry_head`,
  `flag_list`.
- Accessor functions (`logic_config_ptr()`, etc.) with static fallbacks
  for backward compat with standalone heaps.
- Sequential multi-instance now works: two runtimes in the same process
  have independent logic configs, HIT registries, and counters.

## Phase C: Module loader

- `(import "path.lisp")` — load once with caching, resolved via search path.
- `(module-loaded? "path")`, `(module-search-path)`, `(add-module-path! "dir")`.
- Default search path includes `lib/`. Module cache keyed by both raw
  and resolved paths.

## Phase D: Garbage collector (D.1–D.3)

- `gc_mark` bit on every `lizard_ast_node_t`. `lizard_heap_alloc` zeroes
  memory to ensure marks start at 0.
- `lizard_gc_mark_node` — recursive mark traversal covering 80+ AST types.
- `lizard_gc_mark_env` — walks environment chains and closure captures.
- Segment-level sweep: `lizard_gc_collect` frees heap segments with zero
  live objects. No pointer updating needed.
- `(gc)` — run mark + sweep, report freed bytes and before/after stats.
- `(gc-stats)` — report segments, bytes, total/live/garbage node counts.

## Phase E: Bytecode compiler + VM (E.1–E.2)

- 30-opcode stack-based VM (`src/bytecode.h`, `src/bytecode.c`).
- Compiler handles: constants, variables, arithmetic, comparisons,
  if/else, define/set!, lambda with closures, general function calls,
  begin, cons/car/cdr, display/newline.
- **Tail-call optimization**: `OP_TAIL_CALL` replaces the current frame
  (chunk, env, ip, sp) and restarts the dispatch loop. Zero C stack growth.
- `(vm-eval expr)`, `(disassemble expr)`, `(vm-time expr)`, `(time-eval expr)`.

## Profiler

- `(profile expr)` — compile + execute with full instruction counting.
  Reports: elapsed time, total instructions, call/tail-call counts,
  MIPS, and per-opcode breakdown.

## Structured diagnostics (Phase F)

- `lizard_make_error_at(heap, code, span)` — error with source location.
- Six key eval error paths carry spans (unbound symbol, invalid apply,
  bad define, bad assignment).
- Error printer prepends `line:col:` when span is available.
- `(error-location err)` — programmatic access to error span.

## Documentation

- `docs/HIT.md` — comprehensive HIT layer reference (~350 lines).
- `docs/MODAL.md` — updated with full M.5.* coverage.
- `DESIGN.md` — added "Further documentation" index.
- `docs/CLAIMS_MATRIX.md` — updated throughout.

---

# Lizard v4 — type-theory expansion + diagnostics/scaffolds (in progress)

This section logs the changes from v3 (post-restructure baseline) to the current
head. The v3 section follows. For per-phase detail, see `DESIGN.md`, `docs/MODAL.md`,
`docs/CLAIMS_MATRIX.md`, and `docs/OPTIONAL_PROOF_SCAFFOLDS.md`.

## Public/internal header split

- `include/lizard.h` is now a 13-line compatibility shim re-exporting
  `include/lizard_api.h`. All AST node definitions and interpreter internals
  moved to `src/lizard_internal.h`.
- Embedders get a stable opaque public surface. Internals can evolve without
  ABI breaks. Older embedders that `#include <lizard.h>` keep working.

## Source spans on every AST node

- `lizard_source_span_t span` field on each `lizard_ast_node_t`.
- Tokens already carried `line`/`column`/`offset`; this propagates them
  forward into the AST so diagnostics can point at the right place in source.
- Foundation for future structured error messages.

## Scaffold/checked discipline

- New convention: experimental syntax lives behind opt-in **logic rules** and
  is documented as "scaffold" in `docs/CLAIMS_MATRIX.md` until promoted to
  "checked" status.
- New bundles: `cubical-S1`, `truncations`, `proof-scaffold`.
- New toggles: `cubical-s1-enabled`, `truncations-enabled`,
  `theory-extensions-enabled`.
- Documented migration path in `docs/OPTIONAL_PROOF_SCAFFOLDS.md`.

## Phase H.2 — Propositional truncation promoted from scaffold to checked

- AST nodes `Trunc`, `trunc`, `trunc-elim` (originally scaffold) now have real
  typing rules and a primary computation rule.
- Typing:
  - `(Trunc level A) : Universe-of-A` — universe-preserving.
  - `(trunc x) : (Trunc A)` infers `A` from `x : A`; level left NULL.
  - `(trunc-elim C h e) : C` when `e : (Trunc _ A)` and `h : Π _:A. C`.
- Reduction: `(trunc-elim C h (trunc x)) ⟶ (@ h x)`, deterministic.
- Honest gap: propositionality obligation on motive `C` not structurally
  enforced (see `docs/CLAIMS_MATRIX.md`).
- All operations gated on `truncations-enabled`.
- New test `tests/tt_truncation_test.c`, walkthrough `examples/62-truncation.lisp`.

## Cubical S¹ scaffold (unchanged from upload)

- `S1`, `base`, `loop` with minimal typing spine.
- Remains scaffold-only — no recursor, no `loop`-computation rule, no
  Kan composition.

## Type-theory work prior to H.2 (carried forward from earlier v4 state)

- Lambda cube (M.2, M.3): 8 cube corners + CoC as named bundles.
- Substructural rules (M.4): `weakening`/`contraction`/`exchange` toggles.
- Universe lattice (L.1–L.5): pi-fresh/co-pi-fresh, couniverse, lattice toggles.
- HIT scaffolding (H.1): AST nodes + registry, no computation rules.
- Modal logic layer (M.5.1–M.5.9): K, T, S4, S5 operationally distinct;
  asymmetric forms (box/unbox/diamond/let-diamond/box-app/diamond-bind);
  **symmetric S5 (M.5.9 Turn 2b)**: `dia`, `poss-coerce`, judgment-kind
  tracking, kind propagation through `let-diamond`, kind check in `box-intro`.
  See `docs/MODAL.md`.

## Diagnostics and proof-scaffold infrastructure (community contribution)

- `docs/CLAIMS_MATRIX.md` — precise feature status (implemented / partial /
  scaffold / not implemented), updated whenever a feature changes tier.
- `docs/OPTIONAL_PROOF_SCAFFOLDS.md` — explains the scaffold/checked
  discipline and the intended migration path.
- `tests/tt_optional_extensions.lisp` + `.expected` — golden test for the
  new opt-in nodes at the construction layer.
- Enhanced `runtime.c/h` and `lizard_api.h` for richer embedding surface.
- Generic `theory-extension` AST node (scaffold) for plugging in
  experiments without changing the AST.

## Scoreboard at v4 head (after H.2 + merge)

```
57 C unit tests + 5 Lisp golden tests passing
62+ examples including modal layer (M.5.*) walkthroughs and H.2 truncation
Benchmark: ~0.5s
Builds clean: release, debug, asan, coverage
```

---

# Lizard v3 — restructure + tests + features

## Structural changes

- **`src/` + `include/` split.** All implementation in `src/`, single
  public header `include/lizard.h`. The public header no longer
  pulls in internal `errors.h`/`en.h`/`lang.h`; those become true
  internals and are included only by the `.c` files that need them.
- **`tests/` directory** with:
  - A header-only test harness (`test_harness.h`).
  - A façade (`test_helpers.{h,c}`) that gives each test one line to
    spin up a fresh interpreter and another to evaluate strings.
  - Five C unit tests (`arith_test`, `control_test`, `lambda_test`,
    `lists_test`, `macros_test`).
  - Two golden-output tests (`scripting`, `error_propagation`) with
    matching `.expected` files.
- **`examples/`** unchanged in content but now formally part of the
  layout (with a README of its own).
- **`tests/tests.mk`** included from the top-level Makefile gives
  `make test`, `make test-c`, `make test-lisp`, plus colourised
  PASS/FAIL output per test.
- **`lizard_install_primitives()`** moved from `repl.c` into the
  library so tests and the REPL share one source of truth for the
  set of registered built-ins.

## New features

- **`display`**, **`write`**, **`newline`** primitives — standard
  Scheme I/O so `.lisp` scripts can produce real output rather than
  relying on the REPL's auto-echo.
- **`(load "path")`** primitive — reads a file and evaluates every
  top-level form in the current environment. Reports proper errors
  (`LIZARD_ERROR_LOAD_ARGC`, `_ARGT`, `_OPEN`, `_READ`). Lets
  scripts depend on the prelude:
  ```lisp
  (load "examples/prelude.lisp")
  (display (sum (range 1 101))) (newline)   ; 5050
  ```

## Bugs fixed since the user's snapshot

The upload had the begin/macro fixes but reverted several others. This
release re-applies them on top of the upload's structural improvements
(typedef aliases, `lizard_make_number_copy`, `static` globals,
`lizard_repl_strdup`):

1. **Tokenizer treats all whitespace as whitespace.** `\t`, `\n`, `\r`
   are skipped; previously only `' '` was, so any multi-line file with
   tabs or newlines tokenised into garbage symbols.
2. **`;` line comments** are recognised by the tokenizer.
3. **REPL is stdin-friendly.** When stdin is not a TTY the prompt,
   raw-mode line editor, and `\033[K` escapes are suppressed. The
   continuation join uses `\n` instead of a space so `;` comments
   terminate at the original line boundary.
4. **Scheme-style value printer in the REPL.** Output is `3628800` /
   `(1 2 3)` / `<procedure>`, not `Number: 3628800` and 14-line AST
   dumps. Errors print only their message (no doubled `error: Error:`).
5. **`cond` is implemented in the evaluator.** The parser had been
   building `AST_COND` nodes, but the evaluator had no case for them,
   so every `cond` form returned `LIZARD_ERROR_NODE_TYPE`.
6. **Multi-body function definitions** (`(define (f x) e1 e2 e3)`)
   parse correctly; previously only the first body expression was
   consumed and the rest produced "missing closing paren in define".
7. **`let` accepts non-symbol binding names in macro bodies**, so
   `` `(let ((,name ,value)) ,body) `` works.
8. **Unquote-splicing handles real cons-pair lists**, not only
   `AST_APPLICATION`. Empty list splices to nothing.
9. **Zero-parameter lambdas/macros work.** `((lambda () 42))` and
   `(define-syntax k (lambda () 42))` no longer report "alternative
   lambda parameter format is wrong" — the parser turns `()` into
   `AST_NIL`, which both call paths now accept.
10. **Unbound-symbol errors propagate** instead of being rewritten to
    "attempt to apply a non-function" when they appear in the operator
    position.

## What's verified

`make test`:

```
  PASS  arith_test          (mutation guard, variadic +/-/*//, bignums, div0)
  PASS  control_test        (if, cond/else, begin, and/or/not, let)
  PASS  lambda_test         (recursion, multi-body, mutual, bignum, closures)
  PASS  lists_test          (cons, car/cdr, quote, predicates, user map)
  PASS  macros_test         (quasiquote, splice, special-form expansion, let)
C tests: 5/5 passed
  PASS  error_propagation   (golden-output)
  PASS  scripting           (golden-output)
Lisp tests: 2/2 passed
All tests passed.
```

`make examples` runs all six example files and the prelude without
errors. `06-scripting.lisp` produces a formatted factorial table
ending in `20! is exactly: 2432902008176640000`.

## Open

- `call/cc` still uses module globals (`callcc_buf`, `callcc_active`);
  nested `call/cc` will clobber itself. No regression test for it
  because the existing implementation isn't reliable enough to pin
  golden behaviour to.
- `write` is currently identical to `display` for non-string values.
  In R5RS Scheme, `write` escapes strings (`"a\"b"` would print
  literally). Worth tightening.
- No `string-length`, `string-append`, `number->string` yet — the
  string facilities are minimal. Add when needed by a script.
- Memory: `lizard_heap_destroy` releases everything at REPL exit,
  but inside a long session the bump arena grows and never returns
  memory to the OS (it's grow-only, not a real GC).

# Lizard v4 — performance round

## Tail-call optimisation

Lambda application now tail-calls its **last body expression** by
trampolining through the existing `for(;;)` dispatch loop in
`lizard_eval` — the same mechanism that already drove TCO for `if`,
`begin`, and `cond`. Non-tail bodies are still evaluated for side
effects only, then `(node, env)` are rewritten to the tail body and
`continue` is used.

Before this change every Scheme call ate a real C stack frame:
```lisp
(define (count n) (if (= n 0) 'done (count (- n 1))))
(count 100000)    ; segfault — 8MB C stack exhausted around ~10^4
```

After this change a properly tail-recursive function runs to any
depth limited only by the heap. The new `tco_test.c` exercises
100,000 iterations of plain tail recursion, 50,000 of mutual tail
recursion (`even?`/`odd?`), tail calls out of `cond` clauses, and
tail calls as the last expression of a `begin`.

## Fast bignum primitives

Lizard's bump-allocator heap is grow-only and has no GC, so
loops like
```lisp
(define (pow2-iter n acc) (if (= n 0) acc (pow2-iter (- n 1) (* 2 acc))))
(pow2-iter 1000000 1)
```
allocate `O(N²)` of intermediate bignums and exhaust memory long
before they finish — even with TCO eliminating the stack cost.
MIT Scheme handles this because of its generational GC; lizard
now sidesteps the problem entirely by exposing GMP's fast routines
as primitives so the same computation completes in *one* mpz call:

| Primitive            | Backed by         | Cost     |
| -------------------- | ----------------- | -------- |
| `arithmetic-shift`   | `mpz_mul_2exp` / `mpz_fdiv_q_2exp` | O(1) GMP |
| `expt`               | `mpz_pow_ui`      | O(log e) GMP |
| `gcd`                | `mpz_gcd`         | Lehmer in GMP |
| `lcm`                | `mpz_lcm`         | Lehmer + multiply |
| `quotient`           | `mpz_tdiv_q`      | one division |
| `remainder`          | `mpz_tdiv_r`      | one division |
| `abs`                | `mpz_abs`         | one op |
| `square`             | `mpz_mul`         | one op |
| `modular-expt`       | `mpz_powm`        | O(log e), bounded intermediates |

`(arithmetic-shift 1 1000000)` now produces a 301,030-digit exact
integer in milliseconds. `(modular-expt 2 65537 (- (expt 2 127) 1))`
finishes instantly — the RSA-style core works on arbitrary sizes.

## New tests

- `tests/tco_test.c` — depth-100,000 tail recursion, mutual
  recursion, `cond` and `begin` tail positions.
- `tests/fastprims_test.c` — arithmetic-shift (left/right/huge),
  expt, gcd, lcm, quotient/remainder (with negative dividends),
  abs, square, modular-expt, plus the error paths
  (`(quotient 1 0)`, `(expt 2 -1)`, type mismatch).

## New example

- `examples/14-perf.lisp` — count-down from 1,000,000;
  `arithmetic-shift 1 1000000`; `expt 7 5000`; gcd of two giant
  bignums; a toy RSA encrypt/decrypt cycle.

## What's still not addressed

The arena is still grow-only. Long-running idiomatic Scheme
without the fast primitives still piles up garbage. A real GC
is the right next investment — generational, with the arena
becoming the nursery. That's a larger surgery and was deliberately
kept out of this round so the TCO + fast-prims wins land cleanly.

# Lizard v5 — reflection, types, strings

## Type reflection

```scheme
(type-of 42)           ; 'number
(type-of "hi")         ; 'string
(type-of 'foo)         ; 'symbol
(type-of '(1 2))       ; 'pair
(type-of (lambda(x)x)) ; 'procedure
(type-of +)            ; 'primitive
(type-of #t)           ; 'boolean
```

Returns a symbol naming the runtime type. Covers every AST node
type lizard can produce: number, string, symbol, boolean, nil,
pair, list, procedure, primitive, macro, quote, quasiquote,
promise, error, continuation.

## Environment + procedure introspection

```scheme
(defined? 'my-binding)       ; #t / #f without throwing
(env-keys)                   ; list of every bound symbol in scope
(procedure-arity f)          ; number of formal params, or 'variadic
```

`env-keys` walks every frame from innermost outward; primitives,
user-defined functions, and macros all appear. `procedure-arity`
returns a number for lambdas (counting positional params, including
0 for `(lambda () …)`) and the symbol `'variadic` for C primitives.

## Strings — proper operations, not just opaque blobs

```scheme
(string-length s)
(string-append a b ...)          ; n-ary
(substring s start [end])        ; end defaults to length
(string=? a b)
(number->string n)
(string->number s)               ; -> number, or #f if unparseable
(symbol->string s)
(string->symbol s)
```

The tokenizer also now handles backslash escapes inside string
literals: `\n` `\t` `\r` `\\` `\"` `\0`. Before this change `"say
\"hi\""` lex-failed; now it parses to the obvious 9-char string.

## Records, the lisp-y way

No new C — records are tagged conses with a small Lisp protocol:

```scheme
(define (make-point x y) (make-record 'point (list x y)))
(define (point-x p) (field-nth p 0))
(define (point-y p) (field-nth p 1))
(record? (make-point 3 4))     ; #t
(record-type (make-point 3 4)) ; 'point
```

See `examples/15-types.lisp` for the full pattern. Build on
`record?`, `record-type`, and positional `field-nth` to define any
record shape you want.

## Pattern dispatch

Lizard doesn't have varargs in lambdas, so there's no `syntax-rules`
shaped `match` macro yet. `examples/16-match.lisp` shows a runtime
dispatcher that takes a list of `(tag thunk)` clauses and threads
into the matching one. The example builds a tiny arithmetic
AST, an evaluator, and full symbolic differentiation on top of it:

```
d/dx (3 + 4*x) = (plus (lit 0) (plus (times (lit 0) (var x))
                                     (times (lit 4) (lit 1))))
d/dx -(x + 7) evaluates to -1
```

## New tests

- `tests/reflection_test.c` — every type-of case, defined? for
  bound/unbound/non-symbol args, env-keys completeness via
  member-search, procedure-arity for 0/1/N parameters and
  primitives.
- `tests/strings_test.c` — length/append/substring/equality,
  number↔string round-trip, symbol↔string round-trip, bignum→string,
  out-of-range substring errors.

## New examples

- `examples/15-types.lisp` — reflection, records, an `inspect`
  function that dispatches on `type-of` and prints type-aware
  summaries.
- `examples/16-match.lisp` — pattern dispatcher + symbolic
  differentiation of `(3 + 4x)`, `x*x`, `-(x+7)`.
- `examples/17-strings.lisp` — string manipulation, conversions,
  string-reverse via recursive substring, formatted output helper.

## Scoreboard

```
$ make test
15/15 C tests passing  (arith, bignum, closures, control,
                        deep_recursion, errors, fastprims,
                        higher_order, lambda, lists, macros,
                        quasiquote, reflection, strings, tco)
4/4  Lisp golden tests passing
All tests passed.
```

# Lizard v6 — varargs, exceptions, vectors, hashes (extreme edition)

This release adds four major language features and tightens error
semantics so they all compose cleanly.

## Varargs lambdas

```scheme
(define sum (lambda xs
  (define (loop ys acc)
    (if (null? ys) acc (loop (cdr ys) (+ acc (car ys)))))
  (loop xs 0)))

(sum)                  ; 0
(sum 1 2 3 4 5)        ; 15
(sum 1 2 3 4 5 6 7 8 9 10)  ; 55
```

When a `lambda`'s parameter spec is a single symbol instead of a list,
the symbol is bound to a list of *all* the call arguments. This is the
R5RS "rest" form and unlocks `(define (count . xs) ...)`-style
patterns. Implemented at both eval-time and `lizard_apply` call sites.

Dotted-pair varargs `(lambda (a b . rest) ...)` is not yet supported —
that would need new tokenizer support for `.`.

## Exceptions: try / raise / error-object? / error-value

```scheme
(define (safe-div a b)
  (try (lambda () (/ a b))
       (lambda (err)
         (display "caught: ") (display (error-value err)) (newline)
         0)))

(safe-div 10 2)   ; 5
(safe-div  7 0)   ; prints diagnostic, returns 0
```

User code can raise structured payloads:

```scheme
(raise (list 'invalid-input value "must be positive"))
```

The handler receives an error object and uses `(error-value err)` to
unwrap the payload. `error-object?` tests whether a value is an error.

Two C primitives are exempt from auto-propagation (see below) so that
they can *receive* errors as values: `error-object?` and `error-value`.

## Vectors

```scheme
(define v (vector 10 20 30 40 50))
(vector-length v)         ; 5
(vector-ref v 2)          ; 30
(vector-set! v 2 999)
v                         ; #(10 20 999 40 50)
(vector->list v)          ; (10 20 999 40 50)
(list->vector '(a b c))   ; #(a b c)
```

O(1) indexed access + mutation. Fixed-size; create with
`(make-vector n [fill])` or `(vector v1 v2 ...)`. Printer renders as
`#(...)`. `vector?` predicate.

## Hash tables

```scheme
(define h (make-hash-table))
(hash-set! h 'name "lizard")
(hash-set! h 'age 5)
(hash-ref h 'name)              ; "lizard"
(hash-ref h 'missing 'default)  ; default
(hash-has-key? h 'name)         ; #t
(hash-size h)                   ; 2
(hash-keys h)                   ; (age name)
(hash-remove! h 'age)
```

Open-addressed with linear probing. Doubles capacity when load > 0.75.
Hash function: FNV-1a for strings/symbols, mpz_get_ui for numbers.
Key equality is value-based for numbers, strings, symbols, booleans,
and nil. Printer renders as `#hash((k . v) ...)`.

## Auto-propagation of errors

Before this release, `(display (raise 'oops))` would silently print
`oops` because display didn't know its arg was an error. Now errors
short-circuit at every eval boundary:

- **Primitive calls**: if any argument forces to AST_ERROR, the call is
  skipped and the error propagates (except for `error-object?` and
  `error-value`, which receive errors as data).
- **`if`, `cond`**: an erroring predicate propagates without firing
  any branch.
- **`begin`, lambda body**: an intermediate expression that errors
  short-circuits the sequence.

This makes `(try ...)` actually work the way users expect — without
this, a `(raise ...)` inside a `display` call would be silently
absorbed.

## gensym

```scheme
(gensym)              ; g1
(gensym)              ; g2
(gensym "tmp-")       ; tmp-3
```

Produces a fresh symbol on each call. Useful for hand-written
hygienic-ish macros until proper `syntax-rules` lands.

## type-of extended

Now covers vectors and hashes:

```scheme
(type-of (vector 1 2 3))     ; vector
(type-of (make-hash-table))  ; hash
```

## String escape sequences (bonus)

The tokenizer now decodes `\n`, `\t`, `\r`, `\\`, `\"`, `\0` inside
string literals. Before, `"say \"hi\""` lex-failed.

## New tests

- `tests/exceptions_test.c` — raise + try + nested try + error-value
  with various payload types + system errors (div by zero)
- `tests/vectors_test.c` — every op + mutation persistence + accumulator
  via vector + mixed-type contents + out-of-range errors
- `tests/hashes_test.c` — 200-entry growth stress + key types
  (symbols, strings, bignums) + remove + missing-key + default
- `tests/varargs_test.c` — zero/one/many args + mixed types + variadic
  sum, max, count

## New example

`examples/18-extreme.lisp` exercises all four new features in one
program:
- variadic `puts` formatter dispatching on `type-of`
- in-place insertion-sort on a vector
- word-frequency counter on a hash table
- `safe-div` with try/raise/error-value
- structured `(invalid-age N reason)` payloads with cond-based handler
- fresh gensyms

```
$ ./build/lizard < examples/18-extreme.lisp
hello world, 2025
type-of 42 is: number

before sort: #(5 2 8 1 9 3 7 4 6)
after sort:  #(1 2 3 4 5 6 7 8 9)

word frequencies: #hash((brown . 1) (lazy . 2) (dog . 1) ...)
  'the' appeared 4 times
  'fox' appeared 2 times
  'lazy' appeared 2 times

safe-div 10 2 = 5
  caught error, falling back to 0
safe-div  7 0 = 0
safe-div 99 3 = 33

valid age: 25
valid age: rejected age -5: must be non-negative
valid age: rejected age 200: too old to be plausible
valid age: 99

fresh gensym: swap-tmp-1
another:      swap-tmp-2
distinct?     #t
```

## Scoreboard

```
$ make test
19/19 C tests passing  (arith, bignum, closures, control,
                        deep_recursion, errors, exceptions,
                        fastprims, hashes, higher_order, lambda,
                        lists, macros, quasiquote, reflection,
                        strings, tco, varargs, vectors)
4/4  Lisp golden tests passing
All tests passed.
```

## Known deferred work

- **Hygienic macros (`syntax-rules`)** — would need proper
  alpha-renaming. Significant.
- **Dotted-pair varargs `(lambda (a b . rest) body)`** — tokenizer
  needs `.` handling.
- **Real GC** — currently grow-only arena. A generational collector
  with the arena as the nursery is the obvious next step.
- **call/cc** — exists but uses module globals (`callcc_buf`,
  `callcc_active`), so nested call/cc would collide.
- **`write` vs `display` on strings** — currently identical; R5RS
  says write should escape.

## Phase 3: Deep feature expansion

### Kernel completion
- Sigma types: full inference + computation (proj1/proj2 reduce on pairs).
- J-eliminator: `J C d A a b (refl a) → d a` — the fundamental identity
  elimination principle.
- `sexp_to_kterm` extended: `natrec`, `app`, `lam`, `Pi`, `Sigma`, `pair`,
  `fst`, `snd`, `J`, `Id`, `refl` — the full dependent type theory toolkit.
- `(kernel-reduce expr)` — normalize kernel terms to WHNF.
- `(kernel-equal? a b)` — check definitional equality.

### Proof state + tactics
- `src/tactics.c` + `src/tactics.h` — proof state management.
- `(begin-proof type)` — start proof with one goal.
- `(tactic-intro name)` — introduce Pi binder.
- `(tactic-exact term)` — provide exact proof term.
- `(tactic-refl)` — solve Id a a goals.
- `(qed)` — finish proof, extract term.

### Track C: Atoms
- `(atom val)`, `(deref a)`, `(swap! a f)`, `(reset! a v)`, `(atom? x)`.
- Mutable reference cells with CAS-style swap.

### Exceptions
- `(raise val)` — raise an exception.
- `(guard handler body)` — catch exceptions.

### String operations
- `string-ref`, `string-contains?`, `string-upcase`, `string-downcase`.
- `string-split`, `string-join`.

### Lazy evaluation
- `(delay expr)` — create a promise (thunk).
- `(force p)` — evaluate and cache.
- `(promise? x)` — predicate.

### Standard library (lib/match.lisp)
- sort, zip, partition, flatten, take, drop, any, every, enumerate.
- compose, ->, alist-ref, alist-set, range.
- identity, const, flip, curry, uncurry, repeat, complement, memoize, juxt.

### Persistent data structures
- Persistent vectors: pvec, pvec-ref, pvec-set, pvec-push, pvec-count.
- Persistent hash maps: phash-map, phash-get, phash-set, phash-keys.
- Syntax objects: datum->syntax, syntax->datum, syntax-e, syntax-source.

## Phase 4: Elaboration foundations

### Metavariables / Holes (K.4)
- `KT_META` kernel term tag — placeholder for unknown terms.
- `meta_ctx_t` — metavariable context tracking type, solution, and ID.
- `meta_fresh(heap, mctx, type)` — create a fresh typed hole.
- `meta_solve(mctx, id, solution)` — fill a hole.
- `meta_zonk(heap, mctx, term)` — substitute solved metas in a term.
- `(kernel-hole type)`, `(kernel-solve id term)`, `(kernel-zonk term)`,
  `(kernel-meta-state)` — Lisp-facing primitives.

### Unification (K.5)
- `kt_unify(heap, ctx, mctx, a, b)` — first-order unification.
- Flex-rigid: unsolved meta on one side → solve with the other.
- Structural: recursively unify corresponding subterms.
- Works with WHNF reduction — unfolds definitions before comparing.
- `(kernel-unify a b)` — Lisp-facing primitive.
- `?N` syntax in sexp_to_kterm for metavariable references.

### Kernel Bool + Unit types
- `Bool`, `true`, `false` — boolean type with constructors.
- `(if b t f)` — Bool eliminator with computation rules:
  `if true t f → t`, `if false t f → f`.
- `Unit`, `*` — unit type with unique inhabitant.

### Tactic: assumption
- `(tactic-assumption)` — search context for hypothesis matching goal.

# Phase 2U — diagnostic metadata and example-manifest hardening

- Added diagnostic severity/category metadata to `lizard_diagnostic_t`.
- Added severity/category defaulting and public name helpers.
- Parser diagnostics now classify parse failures as `severity=error`, `category=parser`.
- Added API regression coverage for diagnostic metadata.
- Added `scripts/check-example-manifest.sh` for static manifest hygiene checks.
- Hardened `scripts/run-examples.sh` so manifest entries without files are counted and fail CI.
- Added `make examples-audit`.
- Added/restored `examples/63-pow.lisp` so the manifest and filesystem agree.
- Marked currently incomplete showcase examples as experimental instead of dishonest pass gates.

# Phase 2V — restore report API + diagnostic report v2

- Restored the public diagnostic/syntax report typedefs and function prototypes
  required by `diagnostic_report_test.c` and external tooling.
- Added `report_writer.c/.h`, `diagnostic_report.c/.h`, and
  `syntax_expansion_report.c/.h` to the library build.
- Made tokenizer unterminated-string errors recoverable instead of `exit(1)`.
- Added diagnostic report v2 text/JSON output with severity and category fields.
- Added `tests/diagnostic_report_metadata_test.c` for severity/category report
  metadata.
- Fixed `scripts/clean.sh --check` recursion caused by an uncommented usage
  line and allowlisted the legitimate top-level `lib/` directory.
- Kept all strict warning/security flags intact.

# Phase 2W — public report API boundary audit

- Restored `lizard_expansion_trace_event_t` to `include/lizard_api.h` so syntax-object headers can expose trace-event APIs without unknown-type failures.
- Added `tests/api_report_types_test.c` and extended `tests/public_header_test.c` to lock public report/event typedef visibility.
- Added `scripts/check-public-api.sh` and `make api-audit` to prevent future accidental removal of report/syntax public API types.
- Added `api-audit` to the `ci` target.
- Kept strict/security warning flags intact and added no warning suppressions.

# Phase 2Z — diagnostic construction unification

- Added `src/diagnostics.c` / `src/diagnostics.h` as the canonical diagnostic/span construction path.
- Added public diagnostic construction helpers: `lizard_source_span_clear`, `lizard_source_span_set`, `lizard_diagnostic_clear`, `lizard_diagnostic_set`, `lizard_diagnostic_set_simple`, and `lizard_diagnostic_copy`.
- Moved diagnostic severity/category default mapping and name lookup into `diagnostics.c`.
- Refactored tokenizer, parser, runtime, and syntax-expansion reports to use the shared diagnostic helpers.
- Added `tests/diagnostic_construction_test.c` and `tests/diagnostic_category_paths_test.c`.
- Extended `scripts/check-public-api.sh` to guard the diagnostic construction API.
- Kept diagnostic report v2 stable and preserved text/JSON output shapes.
- No strict/security warning flags were removed and no warning suppressions were added.

# Phase 3B — GC metadata side-table scaffold

- Added `src/gc_metadata.c` / `src/gc_metadata.h`.
- Added a C-owned per-heap side table for object size/kind/owner/trace-policy metadata.
- Wired heap creation/destruction to create/destroy the side table.
- Wired heap allocation to register metadata opportunistically without changing allocation semantics.
- Added metadata stats and lookup helpers through `gc.h`.
- Added `tests/gc_metadata_test.c`.
- Extended ownership audit coverage for the metadata side table.
- Kept object layout, mark traversal, sweep behavior, evaluator semantics, and strict warning policy unchanged.

# Phase 3C — liblizard build graph closure audit

- Fixed the syntax-object scaffold link regression class by making `LIB_SRCS`
  close over additional `src/*.c` modules automatically while keeping the core
  source order explicit.
- Added `scripts/check-build-graph.py` to audit that implementation sources are
  represented in the library build graph and that stale `LIB_SRCS` entries are
  rejected.
- Added `make build-graph-audit` and wired it into `make ci`.
- Preserved strict compiler/security flags; no warning suppressions were added.

# Phase 3D — conservative build graph recovery

- Replaced the aggressive Phase 3C `src/*.c` library closure with a conservative allowlisted optional-source closure.
- Prevented incomplete experimental modules such as `prims_kernel_*`, `prims_modules`, `prims_bytecode`, and `kernel_sexp` from being compiled merely because they exist in `src/`.
- Added `scripts/phase3d-recover-build.py` for idempotent recovery of locally drifted Phase 3 trees.
- Hardened public/report header boundaries for report schema and expansion trace APIs.
- Added parser/tokenizer prototypes required by syntax-object scaffolding.
- Replaced the build-graph audit with an allowlist-aware audit.
- Kept all strict/security flags intact and added no warning suppressions.

# Phase 3E — public API duplicate-definition recovery

- Fixed duplicate `lizard_expansion_trace_event_t` definition in `include/lizard_api.h`.
- Added `scripts/check-public-api-duplicates.py` to reject duplicate public typedef/enum definitions.
- Wired duplicate-definition checks into `make api-audit` through `scripts/check-public-api.sh`.
- Hardened `scripts/phase3d-recover-build.py` so repeated recovery runs remove duplicate public API blocks instead of appending more.
- Kept strict warning/security flags unchanged.

# Phase 3G — context expansion trace public API recovery

- Restored context-level expansion trace public API declarations in `include/lizard_api.h`.
- Added `scripts/phase3g-recover-trace-context-api.py` for mixed local trees.
- Extended `scripts/check-public-api.sh` so `make api-audit` catches missing trace context declarations.
- No strict/security warning flags were removed and no warning suppressions were added.

# Phase 3H — surface filename propagation and audit cleanup

- Propagated caller-provided filenames through tokenizer/source parsing into parser diagnostics, top-level AST spans, SurfaceTerm spans, expansion trace origins, and traced context evaluation.
- Made `lizard_context_eval_file` route through a filename-aware internal evaluation path so file diagnostics report the real path instead of `<string>`.
- Restored direct `lizard_api.h` inclusion for `surface_term.h` to satisfy header-boundary audits.
- Updated ownership/build-graph audits for the current live bridge/report modules.
- Added `.gitignore` and removed generated build artifacts plus wrong-project leftovers from the packaged tree.
- Kept strict compiler/security flags unchanged.
