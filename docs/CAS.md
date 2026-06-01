# A Proof-Producing CAS in Lizard

## The dream

A computer algebra system where every result carries a proof. You ask
for `d/dx x²` and get back not just `2x` but a **derivation** — a
finite tree of named rules — and that tree can be unfolded all the way
down to the **ZFC axioms**. The integral, the simplification, the limit:
each is a theorem, and lizard can show you the theorem's foundations.

This is the architecture lizard now provides a foundation for, in two
modules:

- `lib/cas.lisp` — the symbolic engine (simplify, differentiate,
  integrate)
- `lib/cas-proof.lisp` — the justification layer (axiom database,
  derivation trees, unfolding to ZFC)

## Why lizard is a good host for this

lizard already has the two halves a verified CAS needs:

1. **A symbolic substrate.** Scheme s-expressions ARE the natural
   representation for math expressions and for proof terms. Bignums are
   exact. Pattern matching (`lib/pattern.lisp`) and term rewriting
   (`lib/rewrite.lisp`) give you the rewrite engine a CAS is built on.

2. **A trusted proof kernel.** The dependent-type-theory kernel
   (`kernel-check`, `kernel-infer`, tactics) is the place where a
   *checked* proof can ultimately live. A CAS rule can be stated as a
   kernel theorem and discharged by the kernel — turning a "cited" rule
   into a "verified" one.

The CAS sits in the **derived tower**; the kernel is the **trusted
core**. A result is trustworthy exactly to the degree its derivation
reaches checked kernel theorems (and, below them, the axioms).

## The layered foundation

`cas-proof.lisp` encodes the dependency layers every calculus fact
rests on:

```
  Layer 5  CAS rewrite / calculus rules   d/dx xⁿ = n xⁿ⁻¹
              │
  Layer 4  the derivative                 f' = lim (f(x+h)−f(x))/h
              │
  Layer 3  limits & continuity            ε–δ over ℝ
              │
  Layer 2  field & order axioms           assoc, comm, distrib, …
              │
  Layer 1  number constructions           ℕ → ℤ → ℚ → ℝ (Dedekind cuts)
              │
  Layer 0  ZFC axioms                     extensionality, infinity, …
```

Each rule names its dependencies. `unfold-to-axioms` walks the chain
to the leaves; `print-dep-layers` prints the whole tree; and
`print-foundations` summarizes which ZFC axioms a given rule rests on.

So `d/dx x² = 2x` cites `calc-power`, which rests on `calc-product` and
`nat-construction`, which rest on `derivative-def`, `limit-def`,
`field-distrib`, …, which rest on `real-construction`, which rests on
`zfc-infinity`, `zfc-power-set`, `zfc-separation`. The black box opens
all the way down.

## What's implemented vs. scaffolded

**Implemented (computes for real):**
- Symbolic simplification with the standard identities
- Differentiation: constant, variable, sum, difference, product,
  quotient, power, chain, and sin/cos/exp/ln
- Basic polynomial integration
- Derivation trees with rule citations
- The full ZFC→calculus dependency database and its unfolding

**Scaffolded (structure, not yet machine-checked):**
- The *statements* of the axioms/lemmas are informal strings, not
  kernel terms. The next step is to state each as a kernel proposition.
- The dependency edges assert "rule X follows from Y, Z" without a
  checked proof of that entailment. Discharging those with the kernel
  is the path to a genuinely verified CAS.

**Machine-checked (done for differentiation):**
- `lib/cas/diff-cert.lisp` carries this out for the derivative. The
  differentiation rules are stated as kernel propositions — postulated
  constructors of a judgment `Der f g` ("g is the derivative of f") — and
  the differentiator emits, with each derivative, a proof term that the
  kernel type-checks against `Der (\x. f) (\x. f')`. A wrong derivative
  cannot be certified. See `examples/139-cas-certificates.lisp`. The same
  recipe (rule = postulated constructor; proof = nested application the
  kernel checks) extends to the remaining rules and to integration.


## Integrating with a Maxima-like CAS

If you are writing a Maxima-style CAS in lizard, the integration points
are:

1. **Share the expression representation.** Use the same s-expression
   convention (`(+ a b)`, `(* a b)`, `(^ a n)`, `(sin e)`, …) so your
   simplifier, your differentiator, and `cas-proof.lisp` all speak the
   same language. `cas.lisp` is deliberately small so you can replace
   its engine with yours.

2. **Emit a derivation alongside each transformation.** Wherever your
   CAS applies a rule, also record `(rule-name premises)`. The
   `deriv`/`diff-proof` shape in `cas-proof.lisp` shows the pattern:
   conclusion + rule + sub-derivations.

3. **Register new rules in `foundation-db`.** Add an entry
   `(your-rule "statement" (dep ...))`. As long as the deps bottom out
   in existing rules (eventually ZFC), `unfold-to-axioms` works for
   free on your rule.

4. **Promote rules to checked theorems.** For the rules you most care
   about, state them as kernel propositions and prove them with the
   tactic engine (`begin-proof` … `qed`). Replace the informal string
   with a reference to the checked proof. Now those branches of every
   derivation are machine-verified.

## Roadmap to a verified CAS

1. **Kernel-state the axioms.** Encode ZFC (or a working subset) and
   the field axioms as kernel terms.
2. **Construct ℝ in the kernel** (or assume it as a structure with the
   field+completeness axioms), so `real-construction` becomes a real
   object, not a label.
3. **Prove the derivative rules** from `derivative-def` using the
   tactic engine; attach the proof objects to the database entries.
4. **Make the CAS proof-carrying end to end:** every `derivative`
   call returns `(result . derivation)` where the derivation type-checks
   against the kernel.
5. **Certificate checking:** a `check-derivation` pass that walks a
   derivation and confirms each step against its kernel theorem — the
   CAS analogue of proof-checking.

At that point a result like `∫₀¹ x² dx = 1/3` would come with a
certificate the kernel accepts, and you could trace it down to the
construction of ℝ and the axioms of ZFC — exactly the dream.

## See also

- `examples/125-cas-symbolic.lisp` — the symbolic engine
- `examples/126-cas-proof-to-zfc.lisp` — unfolding a derivative to ZFC
- `lib/rewrite.lisp` — term rewriting + induction (the rewrite substrate)
- `lib/proof.lisp`, `lib/tactics-ext.lisp` — the kernel-facing proof tools

## Polynomial algebra and factorization (computes for real)

`lib/cas/poly.lisp` and `lib/cas/factor.lisp` add an exact univariate computer
algebra layer over the rationals — the first stage of making the CAS
competitive on a coherent algebraic core rather than only on differentiation.

A polynomial is a dense coefficient list, low-to-high, over lizard's exact
rationals (bignum numerators/denominators), so the whole layer is exact and
total. `poly.lisp` provides ring arithmetic, Horner evaluation, derivative,
division with remainder, monic gcd (Euclid), content/primitive part over Z,
square-free decomposition (Yun), an s-expression bridge (`expr->poly` /
`poly->expr` sharing the `(+ a b)`/`(* a b)`/`(^ x n)` convention), and
sign-and-rational-aware pretty printing.

`factor.lisp` factors a univariate polynomial over Q into irreducibles by the
standard modern route: square-free decomposition reduces to the square-free
case; the primitive integer factor is factored modulo a good prime by
CANTOR-ZASSENHAUS (distinct-degree then equal-degree splitting over F_p); that
factorisation is HENSEL-lifted past the Landau-Mignotte coefficient bound; and
the lifted factors are RECOMBINED and trial-divided over Z (Zassenhaus).
Non-monic inputs are handled by the monic-reduction substitution
`G(y) = b^(n-1) f(y/b)`, factoring the monic `G`, and transferring factors back.
Finite-field factorization (`cz-factor`) is exposed in its own right.

Crucially, the result is always **checked by multiplying the factors back**
(`factor-verify`): a wrong factorisation can never be returned. This is the
CAS analogue of the differentiation certificate — most algebra results
self-certify cheaply, and factorization is the cleanest example (multiply
back), just as integration certifies by differentiating and a solved root
certifies by substituting.

Verified against a battery including `x^6-1` (four cyclotomic factors), the
Swinnerton-Dyer quartic `x^4-10x^2+1` (irreducible over Z though it splits
modulo every prime — the canonical Hensel/recombination stress test), non-monic
`6x^2+5x+1 = (2x+1)(3x+1)`, repeated factors, and rational coefficients. See
`examples/152-polynomial-algebra.lisp` (self-checking) and the `cas_poly`
golden test.

**Next on this track:** rational-function normalisation and partial fractions
(built on factorization), then rational-function integration certified end to
end by the `Der` judgment — the headline "competitive *and* proven" result.

## Rational functions, partial fractions, and certified integration

`lib/cas/ratfun.lisp` adds rational-function arithmetic over Q (normalised to a
gcd-reduced fraction with monic denominator) and PARTIAL-FRACTION decomposition.
A proper `p/q` is split using the Chinese-remainder formula
`P_i = p * (q/m_i)^{-1} (mod m_i)` over the coprime prime-power denominators
`m_i = f_i^{e_i}` (the `f_i` coming from `factor-Q`), followed by the `f_i`-adic
expansion of each `P_i`; improper inputs first split off a polynomial part.
Every decomposition is checked by recombining the terms over a common
denominator (`pf-verify`).

`lib/cas/integrate.lisp` integrates rational functions and — this is the point
— **certifies each antiderivative by differentiating it back to the integrand**.
After partial fractions, each term integrates in closed form: the polynomial
part by the power rule; linear factors to logarithms (multiplicity 1) and
rational terms (higher multiplicity); complex-root quadratic factors by
splitting off the `f'`-proportional part (a log) and leaving `mu/f`, an arctan.
The answer is then differentiated and checked, **exactly as rational functions
over Q**, to equal the integrand. The elegant part: the arctan term's
irrational constant `sqrt(D)` never enters the check, because the derivative of
`(2 mu / sqrt(D)) arctan((2a x + b)/sqrt(D))` is the *rational* function `mu/f`
by a closed-form identity. So the entire certificate lives in Q and is a
complete decision procedure — a wrong antiderivative cannot pass.

This is the "competitive *and* proven" result: Maxima integrates rational
functions, but does not hand back a checkable proof. Examples:
`INT 1/(x^2+1) = arctan(x)`, `INT 1/(x^2+x+1) = (2/sqrt 3) arctan((2x+1)/sqrt 3)`,
`INT (5x+4)/((x-1)(x^2+4)) = (9/5) log(x-1) - (9/10) log(x^2+4) + (8/5) arctan(x/2)`,
each differentiate-back verified. Cases needing algebraic numbers beyond Q
(quadratics with real irrational roots, repeated quadratics with an arctan part,
irreducible factors of degree >= 3) are reported as `cannot` rather than
integrated wrongly or partially. See `examples/153-rational-integration.lisp`
(self-checking) and the `cas_integrate` golden test.

The remaining gap to the differentiation certificate already in
`lib/cas/diff-cert.lisp` is to route this rational-function equality check
through the kernel `Der` judgment, turning the (already sound) algebraic
certificate into a kernel-checked proof object — the bridge between this track
and the proof kernel.

**Next on this track:** algebraic-number support to close the remaining
integration cases (Rothstein–Trager logarithmic part), then equation solving
(univariate via factorization; linear systems), limits and series (Gruntz), and
symbolic linear algebra — each self-certifying.

## Tier 1 shipped: resultants, algebraic numbers, equation solving

Three modules build directly on the factorizer and move several Maxima
capabilities into the "computes for real, and self-checks" column.

`lib/cas/resultant.lisp` computes the **resultant** of two polynomials over Q as
the determinant of the Sylvester matrix (exact Gaussian elimination over Q), the
**discriminant** as `(-1)^(n(n-1)/2) res(f,f')/lc(f)`, and the parametric
resultant `R(z) = res_x(p - z q', q)` needed for Rothstein-Trager, obtained by
evaluation + Lagrange interpolation so every step stays a plain rational
computation. The self-check is that `res(f,g)=0` exactly when `f,g` share a
factor.

`lib/cas/algnum.lisp` implements exact arithmetic in an algebraic number field
`Q(alpha) = Q[x]/(minpoly)`: add/sub/mul, and inverse via extended Euclid (valid
because the minimal polynomial is irreducible, so the quotient is a field). It
evaluates a rational polynomial at an algebraic element and reports surds in
lowest form (`sqrt(2)`, `i`, golden-ratio expressions), with `RootOf` for higher
degrees. This is the substitution oracle the solver uses and the coefficient
arithmetic Rothstein-Trager will need.

`lib/cas/solve.lisp` solves polynomial equations over Q-bar by factoring and
reading roots off the irreducible factors: linear -> exact rational; quadratic
-> the two conjugate roots as exact surds `p +- q*sqrt(D)` in `Q(sqrt D)`; degree
>= 3 -> `RootOf(factor)` annotated with the number of REAL roots, counted exactly
by a **Sturm sequence**. Multiplicities are carried through. Rational and surd
roots are **certified by substituting them back** (in Q, or in `Q(sqrt D)`) and
checking zero, so a wrong root cannot be reported. Sturm counting (`count-real-
roots`, `count-real-roots-in`) is verified against known counts (e.g. the
Swinnerton-Dyer quartic has exactly four real roots). See
`examples/154-solving-and-algebra.lisp` and the `cas_solve` golden test.

With resultants + algebraic numbers + the parametric resultant all in place, the
**Rothstein-Trager logarithmic part** is now within reach: factor `R(z)`, and for
each residue `c` form `c * log(gcd(p - c q', q))`. The rational-residue case
needs only the Q-arithmetic already here; the genuinely-algebraic case needs
polynomials over `Q(alpha)` (coefficients in an algebraic field), which is the
next brick and the step that closes the integration cases currently deferred
(quadratics with real irrational roots).

## Tier 2 shipped: the Rothstein-Trager logarithmic part (Risch, rung 1)

`lib/cas/apoly.lisp` provides polynomials whose coefficients are algebraic
numbers (Q(alpha)[x]): the same dense representation as poly.lisp but with
alg-arithmetic for coefficients, giving exact long division and a monic
Euclidean GCD because Q(alpha) is a field. This is the coefficient layer the
algebraic-residue case needs.

`lib/cas/rt.lisp` implements the **Rothstein-Trager logarithmic part** of
integrating p/q with q squarefree and deg p < deg q. It forms
`R(z) = resultant_x(p - z q', q)`, factors R over Q, and for each residue c (a
root of R) produces `c * log(gcd(p - c q', q))`. Linear factors of R give
rational residues and ordinary logs; higher-degree irreducible factors give
conjugate algebraic residues, whose log argument is computed once in Q(c) with
apoly and reported as a RootSum over the conjugates. This integrates exactly the
cases the partial-fraction integrator defers -- `1/(x^2-2)` (real irrational
residues), `1/(x^2+1)` (complex residues, equivalent to arctan), `1/(x^3-2)`,
and mixed denominators -- so **rational-function integration is now complete over
Q-bar for squarefree denominators**.

Crucially this stays self-certifying even with algebraic residues. The whole log
part is checked over Q by the polynomial identity
`p = sum_residues c * g_c' * (q / g_c)`, where the sum over the conjugate
residues of an irreducible factor is computed as `Trace_{Q(c)/Q}` of
`c * g' * (q/g)` (power sums of the roots via Newton's identities). The
certificate is a plain equality of rational polynomials, so a wrong antiderivative
cannot pass -- it holds across rational, real-algebraic, complex, and cubic
residues. See `examples/155-rothstein-trager.lisp` and the `cas_rt` golden test.

This is the first rung of the Risch ladder: integration in Q(x) is now complete
and certified. The next rungs add a differential-field tower (exp/log monomials),
Hermite reduction in the tower, and the Risch differential-equation solver, at
which point the integrator decides elementary integrability for transcendental
elementary functions -- beyond what Maxima's partial Risch implementation does,
and uniquely with proofs.

## Tier 3 begun: transcendental Risch (the summit, first rung)

`lib/cas/risch.lisp` is the start of the actual Risch decision procedure: it
integrates over a single transcendental monomial extension of Q(x) -- theta =
exp(u) or theta = log(x) -- with polynomial coefficients, computing an
elementary antiderivative when one exists and PROVING non-elementarity when one
does not.

A "tower polynomial" is a polynomial in theta whose coefficients are polynomials
in x (a list of poly coefficient-lists). A derivation D acts on it knowing
`D(e^u) = u' e^u` and `D(log x) = 1/x`; for the exponential monomial D is
diagonal in the theta-degree, `D(sum a_i theta^i) = sum (a_i' + i u' a_i)
theta^i`, and for the logarithmic monomial it couples adjacent degrees.

Exponential integration is term-by-term: `INT a_i e^{i u} = b_i e^{i u}` where b_i
solves the **Risch differential equation** `b_i' + i u' b_i = a_i` over Q[x]. With
polynomial data any rational solution is forced to be polynomial, so a degree
bound plus a triangular linear solve DECIDES the equation -- it returns the
unique b_i, or proves none exists. Logarithmic (primitive) integration is a
triangular recurrence of polynomial antiderivatives, choosing integration
constants so divisions by x stay polynomial. Both decide on their domain.

The decisive property is that this stays self-certifying. Every elementary
answer is checked by differentiating it back with D and comparing to the
integrand (`tpoly-equal?`), so `risch-exp`/`risch-log` only return an answer they
have just re-derived. Worked results include `INT x e^x = (x-1)e^x`,
`INT (x^2+1)e^x`, `INT 2x e^(x^2) = e^(x^2)`, `INT x log x = (1/2)x^2 log x -
(1/4)x^2`, `INT (log x)^2 = x(log x)^2 - 2x log x + 2x`; and as genuine
decisions, `INT e^(x^2) dx`, `INT x^2 e^(x^2) dx`, `INT x^4 e^(x^2) dx` are all
**proved to have no elementary antiderivative**. See
`examples/156-transcendental-risch.lisp` and the `cas_risch` golden test.

This is the same kind of decision Maxima's `risch` performs only partially for
the transcendental case, here done with a checkable proof. The remaining climb
to the full summit: nested monomial towers (extensions over Q(x, theta_1, ...)),
Hermite reduction in the tower for proper-rational-in-theta integrands, the full
Risch differential equation with denominators (rational coefficients), and
eventually the algebraic case (Trager-Bronstein).

## Tier 3 continued: rational functions of a monomial, with the exact derivation

`lib/cas/tower.lisp` upgrades the coefficient field from Q[x] to Q(x), so the
differential-field derivation now handles `D(log x) = 1/x` exactly. A tower
polynomial (`rfpoly`) is a list of Q(x) coefficients low-to-high in theta; a
tower rational function is a pair `(N D)` of rfpolys, and `tr-deriv` differentiates
it with the quotient rule through the field. (The missing Q(x) operations -- sub,
inverse, division, derivative -- are added on top of ratfun.lisp.)

On this it integrates two genuinely new families, each certified:

* **New logarithms.** When the integrand equals `D(g)/g` for a tower element g,
  the integral is `log(g)`. The recognizer tries the denominator and theta as
  candidates and verifies the defining identity exactly, which is itself the
  certificate. This gives `INT 1/(x log x) = log(log x)`, `INT e^x/(e^x+1) =
  log(e^x+1)`, and `INT 2x/(x^2+1) = log(x^2+1)`.

* **Hermite rational part of a primitive monomial.** For negative powers,
  `INT (c/x)(log x)^(-k) = -c/(k-1) (log x)^(1-k)`; e.g. `INT 1/(x (log x)^2) =
  -1/log x` and `INT 1/(x (log x)^3) = -1/(2 (log x)^2)`. The rational answer is
  checked by differentiating it back with `tr-deriv` and comparing to the
  integrand.

Crucially the toolbox DECLINES what it cannot certify rather than faking it:
`INT 1/log x dx` (the non-elementary `li(x)`) and `INT 1/(log x)^2 dx` are
correctly left unresolved. See `examples/157-tower-integration.lisp` and the
`cas_tower` golden test.

This is the start of the proper-rational case of transcendental Risch. The
remaining pieces to a complete single-monomial integrator are general Hermite
reduction for arbitrary denominators (not just pure powers) and the in-tower
Rothstein-Trager logarithmic part for squarefree denominators; then nested
towers, and finally the algebraic case.

## Tier 3 continued: general Hermite reduction (the proper-rational engine)

`lib/cas/tower.lisp` now includes general **Hermite reduction** for a proper
rational function a/d of a primitive monomial (theta = log x), the core of the
proper-rational case of transcendental Risch. Where the previous step handled
only pure powers theta^(-k), this handles arbitrary denominators.

The machinery added over Q(x)[theta]: the formal theta-derivative, polynomial
division and a monic Euclidean GCD, extended Euclid (Bezout) and inverses modulo
a factor, exponentiation, and Yun squarefree factorization. The reduction is
derived from first principles: for the squarefree factor v of highest
multiplicity m (with d = v^m w, gcd(v,w)=1, and v normal so gcd(v, D(v)) = 1),
solve `b == -a (w (m-1) D(v))^{-1} (mod v)` by inverting modulo v; then
`b/v^(m-1)` is an exact rational part and the remainder
`(a + w((m-1) b D(v) - D(b) v))/v` has the v-power reduced by one. Iterating
leaves a squarefree denominator. The rational part is reduced to lowest terms.

`integrate-proper` runs Hermite and hands the squarefree remainder to the
new-logarithm finisher, so it returns an exact rational part plus (optionally) a
logarithm: e.g. it recovers `INT D(1/(log x - 1)^2 + log log x)` as exactly
`1/(log x - 1)^2 + log(log x)`, and the squarefree base case folds in so
`INT 2x/(x^2+1) = log(x^2+1)`. The whole answer is CERTIFIED by differentiating
it back through the field's derivation and comparing to the integrand
(`proper-verify`); the strongest test is round-trip: differentiate a known
answer, integrate it back, and check the derivative of the result matches. See
`examples/158-hermite-reduction.lisp` and the `cas_hermite` golden test.

What remains for a complete single-monomial transcendental integrator is the
multi-residue in-tower Rothstein-Trager logarithmic part (the finisher currently
resolves the single-logarithm case); then nested monomial towers, and the
algebraic case. The Hermite rational part -- the harder, denominator-shrinking
half -- is now done and certified.

## Tier 3 continued: elementary integration over a monomial by substitution

`lib/cas/elem.lisp` finishes large classes of single-monomial integrals by
reducing them to the complete, certified rational-function integrator with two
exact substitutions:

* primitive (theta = log x): `INT (1/x) R(log x) dx = [INT R(t) dt]_{t=log x}`,
  since `(1/x) dx = dt`;
* exponential (theta = e^x): `INT R(e^x) dx = [INT R(u)/u du]_{u=e^x}`, since
  `dx = du/u`.

The right-hand side is an ordinary rational-function integral in the monomial,
which `integrate.lisp` solves and certifies by differentiating back over Q --
including the polynomial part, MULTIPLE logarithms, and ARCTANGENTS. Correctness
is that Q-certificate (in the monomial variable) together with the substitution
theorem. This yields, all certified:
`INT 1/(x log x) = log log x`, `INT (log x)^2/x = (1/3)(log x)^3`,
`INT 1/(x((log x)^2-1)) = (1/2)log(log x -1) - (1/2)log(log x +1)`,
`INT 1/(x((log x)^2+1)) = arctan(log x)`, `INT e^x/(e^x+1) = log(e^x+1)`,
`INT 1/(e^x+1) dx = x - log(e^x+1)`, and `INT e^x/(e^(2x)+1) = arctan(e^x)`.

It declines honestly when the reduced integral needs algebraic residues (e.g.
`INT 1/(x((log x)^2-2))` reduces to `INT 1/(t^2-2) dt`, which the partial-
fraction integrator defers); those reduce to the Rothstein-Trager module.
Combined with the polynomial-case Risch and general Hermite reduction, the
single-monomial transcendental integrator now covers the rational/log/arctan
families end to end, with proofs. See `examples/159-elementary-substitution.lisp`
and the `cas_elem` golden test.

## Tier 4 begun: Gosper's algorithm (indefinite hypergeometric summation)

`lib/cas/gosper.lisp` is the discrete analogue of the Risch decision procedure.
A term t(n) is hypergeometric when r(n) = t(n+1)/t(n) is rational; Gosper decides
whether t has a hypergeometric antidifference S (with S(n+1)-S(n)=t(n)) and, if
so, returns a rational R(n) with S(n) = R(n) t(n), so the sum telescopes -- and
otherwise PROVES no such S exists.

It reuses earlier machinery: resultants (for the Gosper-Petkovsek normal form,
whose shifts are the non-negative integer roots of resultant_n(a(n), b(n+h))),
factorization (to read those roots off), and a new exact Gauss-Jordan linear
solver over Q (for Gosper's equation a(n) x(n+1) - b(n-1) x(n) = c(n) under a
degree bound). The decision is complete: a polynomial solution exists iff the
term is Gosper-summable.

The certificate is purely rational and needs no hypergeometric reasoning:
S(n+1)-S(n) = t(n) holds iff R(n+1) r(n) - R(n) = 1 as rational functions of n,
which is checked exactly. Worked, certified results: SUM k = n(n-1)/2,
SUM k^2, SUM k^3, SUM k^4, SUM k*k! (R = 1/n, i.e. S(n) = n!), SUM n*2^n,
SUM n^2*2^n, SUM 1/(n(n+1)) (R = -(n+1)). And as genuine decisions, the harmonic
sum SUM 1/n, SUM 1/n^2, SUM n!, SUM n^2*n!, and the central binomial sum
SUM C(2n,n) are all PROVED to have no hypergeometric antidifference. See
`examples/160-gosper-summation.lisp` and the `cas_gosper` golden test.

This opens the summation track (the natural next step is Zeilberger's algorithm
for definite hypergeometric sums), and -- like the integration side -- it is a
decision procedure that answers "no closed form" with a proof, not a shrug.

## Tier 4: exact symbolic linear algebra

`lib/cas/linalg.lisp` adds exact linear algebra over Q (matrices as lists of
rational rows), and is where several earlier modules pay off at once. The
characteristic polynomial is computed by Faddeev-LeVerrier (only Q matrix
arithmetic, no polynomial-entry determinant), which also yields the determinant
and a built-in Cayley-Hamilton identity. Determinants reuse the Sylvester-style
matrix-det from resultant.lisp (and are cross-checked against the FL value).
Linear systems and the inverse reuse the exact Gauss-Jordan solver introduced
for Gosper. And EIGENVALUES are the exact roots of the characteristic polynomial
via solve.lisp: rationals, surds, or i.

So diag(2,3,5) gives eigenvalues 2,3,5; [[1,2],[3,4]] gives (5 +/- sqrt 33)/2;
the Fibonacci/companion matrix [[0,1],[1,1]] gives the golden ratio
(1 +/- sqrt 5)/2; and the 90-degree rotation [[0,-1],[1,0]] gives +/- i -- all
exactly, none numerically. Every result is certified: Cayley-Hamilton p(A)=0
(evaluated by Horner over matrices), A A^{-1}=I, det consistency across two
methods, eigenvalues by back-substitution into the characteristic polynomial,
and rational eigenvalues additionally by det(A - lambda I)=0. See
`examples/161-linear-algebra.lisp` and the `cas_linalg` golden test.

With this the system spans both analysis (integration, summation -- decision
procedures that prove when no closed form exists) and algebra (polynomials,
factorization, resultants, equation solving, algebraic numbers, and now linear
algebra), every answer carrying its own machine-checkable certificate.

## Tier 4: Wilf-Zeilberger creative telescoping (proofs of binomial identities)

`lib/cas/wz.lisp` adds the definite-summation counterpart to Gosper: it produces
machine-checked proofs of hypergeometric/binomial identities by creative
telescoping. To prove SUM_k summand(n,k) = rhs(n), set F = summand/rhs so the
claim is SUM_k F(n,k) = 1, and find a rational certificate R(n,k) with
F(n+1,k) - F(n,k) = G(n,k+1) - G(n,k) where G = R F. Summing over k telescopes
the right side to zero, so SUM_k F is constant in n and one base value finishes
the proof.

Dividing the identity by F(n,k) turns it into a purely rational identity in
(n,k): r1(n,k) - 1 = R(n,k+1) r2(n,k) - R(n,k), with r1 = F(n+1,k)/F(n,k) and
r2 = F(n,k+1)/F(n,k). This is THE certificate, and it is checked EXACTLY using a
new layer of bivariate polynomial arithmetic over Q[n][k] -- so a wrong R cannot
slip through. Discovery posits R = P/D with P an unknown bivariate polynomial of
bounded degree; clearing denominators makes the identity LINEAR in P's
coefficients, an exact Q-linear system solved by the same Gauss-Jordan solver
used for Gosper and linear algebra. The certificate found is then re-verified.

Worked results: the system discovers, on its own, the classic certificate
R = -k/(2(n+1-k)) proving SUM_k C(n,k) = 2^n; it discovers the certificate for
SUM_k k C(n,k) = n 2^(n-1); and it verifies the certificate
R = -k^2(3n+3-2k)/(2(2n+1)(n+1-k)^2) proving the central-binomial identity
SUM_k C(n,k)^2 = C(2n,n). Each is corroborated by an independent numeric check.
See `examples/162-creative-telescoping.lisp` and the `cas_wz` golden test.

With Gosper (indefinite) and Wilf-Zeilberger (definite) both in hand, the
summation side now mirrors the integration side: closed forms when they exist,
proofs of the answer either way -- and, as always, every result self-certifying.

## Tier 4: kernel, rank, and integer normal forms

`lib/cas/normalform.lisp` completes the linear-algebra pillar with the structural
decompositions. Over Q it computes reduced row echelon form, rank, and a nullspace
(kernel) basis -- each kernel vector certified by A v = 0, with rank-nullity as a
cross-check. Over Z it computes the two classical normal forms.

The Hermite Normal Form writes H = U A with U unimodular: a row echelon form
reached by integer row operations (Euclidean reduction within each column, then
back-reduction of the entries above each pivot). The certificate is exact:
U A = H, U has integer entries, and det U = +/-1, so U is a genuine automorphism
of Z^m.

The Smith Normal Form writes D = U A V with U and V both unimodular and D diagonal
with d_1 | d_2 | ... | d_r -- this is the structure theorem for finitely generated
abelian groups, made constructive. The algorithm alternates row and column
operations, repeatedly pivoting on the smallest-magnitude entry and fixing
divisibility, while accumulating U (row side) and V (column side). The certificate
checks U A V = D, det U = +/-1, det V = +/-1, that D is diagonal, and that the
divisibility chain holds. The invariants are then cross-checked against forced
values: d_1 equals the gcd of all entries, and the product of the invariants
equals |det A|. So [[2,0],[0,3]] has invariants (1,6); [[6,0],[0,4]] has (2,12);
and a worked 3x3 has (2,2,156). See `examples/163-normal-forms.lisp` and the
`cas_normalform` golden test.

The linear-algebra pillar now covers determinant, characteristic polynomial,
exact eigenvalues, inverse, linear systems, rank, kernel, and both integer normal
forms -- every result self-certifying.

## Tier 4: Zeilberger's algorithm (recurrences for definite sums)

`lib/cas/zeilberger.lisp` is the full creative-telescoping engine: given a definite
hypergeometric sum S(n) = SUM_k F(n,k), it DISCOVERS the linear recurrence with
polynomial coefficients that S(n) satisfies. It finds a_0(n),...,a_J(n) and a
certificate R(n,k) with SUM_j a_j(n) F(n+j,k) = G(n,k+1) - G(n,k), G = R F; summing
over k telescopes the right side to zero, giving SUM_j a_j(n) S(n+j) = 0.

This is where the whole stack converges. Dividing the telescoping identity by F(n,k)
turns it into a bivariate rational identity (each F(n+j,k)/F(n,k) is a product of
shifts of r1 = F(n+1,k)/F(n,k)). Clearing denominators -- using den_J as the common
denominator, since the shift denominators form a divisibility chain -- yields a
bivariate polynomial identity that is homogeneous and linear in the unknown
coefficients of the a_j and of R = P/D. A nontrivial solution with some a_j nonzero
is exactly a NULLSPACE vector of the resulting rational matrix, found with the kernel
routine from normalform.lisp; the bivariate identity from wz.lisp then re-checks the
candidate exactly, so a spurious recurrence cannot pass.

Worked results: for S(n) = SUM_k C(n,k) it discovers the first-order recurrence
S(n+1) = 2 S(n); for the central Delannoy numbers S(n) = SUM_k C(n,k) C(n+k,k) it
discovers the genuine SECOND-order recurrence
(n+2) S(n+2) - 3(2n+3) S(n+1) + (n+1) S(n) = 0. Each is corroborated independently
by checking that the discovered recurrence annihilates the actual integer sequence
(powers of two; 1,3,13,63,321,...). See `examples/164-zeilberger.lisp` and the
`cas_zeilberger` golden test.

The summation side is now complete in the same sense as the integration side:
indefinite summation (Gosper), identity proofs (Wilf-Zeilberger), and recurrence
discovery for definite sums (Zeilberger) -- all proof-carrying.

## Tier 4: power series and limits

`lib/cas/series.lisp` adds truncated power series over Q. A series to order N is the
coefficient list (c_0 ... c_{N-1}) for c_0 + ... + c_{N-1} x^{N-1} + O(x^N). It
provides the full arithmetic (add, multiply, reciprocal, divide, formal derivative
and integral, and composition a(b(x)) when b has no constant term), the series of
any rational function p/q, and the standard elementary series exp, log(1+x), sin,
cos, and the binomial (1+x)^a for rational a.

As everywhere in this CAS the results are checkable. The series S of a rational
function p/q satisfies q*S = p exactly modulo x^N. Each elementary series satisfies
its defining ODE, truncated: exp has S' = S; log(1+x) has (1+x) S' = 1; sin and cos
have S'' = -S; and (1+x)^a has (1+x) S' = a S. Composition reproduces the expected
inverses -- exp(log(1+x)) = 1+x and log(1+(e^x-1)) = x -- and sin^2 + cos^2 = 1 holds
to the truncation order.

Series also give exact limits. For lim_{x->0} g(x)/h(x), comparing the orders of
vanishing resolves the indeterminate form exactly (an exact form of L'Hopital):
lim sin(x)/x = 1, lim (1-cos x)/x^2 = 1/2, lim (e^x-1-x)/x^2 = 1/2, and a ratio whose
numerator vanishes to lower order than its denominator is reported as infinite. See
`examples/165-power-series.lisp` and the `cas_series` golden test.

## Tier 4: series solutions of ODEs

`lib/cas/ode.lisp` solves linear ODEs with polynomial coefficients as power series.
For p_0(x) y + p_1(x) y' + ... + p_m(x) y^(m) = r(x) at an ordinary point, matching
the coefficient of x^k on both sides leaves a single unknown, c_{k+m}, because the
i-th derivative contributes (k+i)!/k! * c_{k+i} to x^k; so from the initial data the
Taylor coefficients follow one at a time. The solution is certified by substitution:
the residual sum_i p_i(x) y^(i)(x) - r(x) must vanish to the truncation order.

This recovers the familiar closed forms -- y'=y gives exp, y''+y=0 gives sin and cos,
(1-x)y'=y gives 1/(1-x) -- and, more interestingly, solves equations with no
elementary solution: the Airy equation y''=xy yields 1 + x^3/6 + x^6/180 + ..., and
y'' - 2x y' + 4y = 0 yields the Hermite polynomial 1 - 2x^2. Each is checked both by
the substitution certificate and against the known coefficients. See
`examples/166-ode-series.lisp` and the `cas_ode` golden test.

## Tier 4: multivariate polynomials and Groebner bases

`lib/cas/groebner.lisp` adds multivariate polynomials over Q -- monomials as
exponent vectors, polynomials as lex-sorted lists of (coeff . monomial) terms --
with full arithmetic and multivariate division (normal form modulo a list of
polynomials). On top of this it implements Buchberger's algorithm: process the
S-polynomial of each pair, reduce it modulo the current basis, and add any nonzero
remainder, until none remain. A minimal reduced monic basis gives a canonical form.

Because the basis G is built from the input F entirely by ideal operations,
<G> = <F>; and G is certified to be a genuine *Groebner* basis by Buchberger's
criterion -- every S-polynomial of the basis reduces to 0 modulo G. The example
checks both this criterion and that every original generator reduces to 0 modulo G
(so F is contained in <G>).

This makes polynomial systems solvable by elimination. A lex Groebner basis is
triangular, so the last generator involves only the final variable: x^2+y^2=1 with
x=y reduces to { x - y, y^2 - 1/2 }; xy=1 with x=y reduces to { x - y, y^2 - 1 };
the two circles x^2+y^2=4 and (x-1)^2+y^2=1 reduce to { x - 2, y^2 }, locating the
tangent point. Normal form decides ideal membership (x^2 - y^2 lies in <x-y>,
x^2 + y^2 does not), and an inconsistent system such as { x-1, x-2 } collapses to
{ 1 }, certifying that it has no solutions. See `examples/167-groebner.lisp` and the
`cas_groebner` golden test.

## Tier 4: ideal operations and system solving

`lib/cas/idealops.lisp` builds on Groebner bases to provide the ideal-theoretic
operations and to actually solve polynomial systems. The elimination ideal is read
off a lex Groebner basis as the generators involving only the later variables (the
Elimination Theorem), which projects a variety; ideal sum is the Groebner basis of
the combined generators; and ideal intersection uses the classic t-trick -- with a
fresh variable t ordered above the rest, a Groebner basis of {t*f} union {(1-t)*g}
is computed and t eliminated, so that <x> ∩ <y> = <xy> and <x^2> ∩ <x> = <x^2>.

Most strikingly, this joins the multivariate and univariate machinery. A lex
Groebner basis of a zero-dimensional system is triangular, so its generator in the
last variable alone is an ordinary univariate polynomial; passed to solve-poly it
yields exact roots. The system x^2+y^2=1, x=y eliminates to y^2-1/2 and solves to
y = +/- (1/2)sqrt(2); the system xy=1, x=y eliminates to y^2-1 and solves to
y = +/- 1. The roots are verified back against the elimination polynomial, and the
elimination polynomial is confirmed to lie in the ideal. See
`examples/168-ideal-ops.lisp` and the `cas_idealops` golden test.

## Tier 3+: in-tower Rothstein-Trager (algebraic-residue integration)

`lib/cas/rt-tower.lisp` closes the integration cases that the reducer over a
primitive monomial (theta = log x) previously deferred. INT (1/x) R(log x) dx
becomes the rational integral INT R(t) dt under t = log x. The reducer in
integrate.lisp handles the polynomial and Hermite rational parts, rational-residue
logarithms, and arctangents, and declines only when the logarithmic residues are
genuinely algebraic -- as in INT 1/(t^2-2) dt, whose antiderivative needs sqrt(2).

In that case, when the denominator is squarefree, the proper part R/den is handed to
rt-log-part (the Rothstein-Trager logarithmic part from rt.lisp), whose answer is a
RootSum over the algebraic residues; the polynomial part is integrated directly and
recombined. Rothstein-Trager's own certificate -- the fully rational identity
p = sum_c c * g_c' * (q / g_c), with the algebraic factor handled as a field trace --
verifies the logarithmic part exactly, and the chain rule (1/x dx = d log x) lifts it
back. An answer is therefore returned only when certified.

So INT 1/(x(log^2 x - 2)) dx, previously "not resolved", now evaluates to
RootSum(c: c^2 - 1/8 = 0, c*log(log x - 4*sqrt(1/8))) + C, certified; and mixed
integrands such as INT log^2 x/(x(log^2 x - 2)) dx return log x plus the algebraic
logarithm. See `examples/169-rt-tower.lisp` and the `cas_rttower` golden test.

## Tier 4: limits of rational functions (any point, and infinity)

`lib/cas/ratlimit.lisp` extends the limit machinery beyond x->0 to every finite point
and to infinity, for rational functions, exactly. To evaluate lim_{x->a} p/q, both
polynomials are expanded about a -- the coefficients of p(a+t) are the Taylor
coefficients of p at a, obtained exactly by repeated synthetic division by x-a -- and
the orders of vanishing are compared. Ordinary points give p(a)/q(a); a removable 0/0
singularity gives the ratio of the leading nonzero coefficients (an exact L'Hopital,
e.g. lim_{x->1}(x^2-1)/(x-1) = 2 and lim_{x->3}(x^2-9)/(x^2-5x+6) = 6); a pole is
reported as infinite. At infinity the degrees decide: 0 when deg p < deg q, the ratio
of leading coefficients when equal (lim (3x^2-x)/(2x^2+5) = 3/2), infinite otherwise.
Everything is exact over Q and the local expansion is itself the certificate. See
`examples/170-rational-limits.lisp` and the `cas_ratlimit` golden test.

## Tier 4: closed-form first-order ODEs (separable)

`lib/cas/ode1.lisp` solves separable first-order ODEs in closed form, reusing the
certified integrator. The equation y' = f(x) g(y) separates to
INT (1/g(y)) dy = INT f(x) dx + C; with g = gnum/gden the left integrand is gden/gnum
(rational in y) and the right is fnum/fden (rational in x). Each side goes to
integrate-rational, which returns an antiderivative and verifies it by differentiating
back over Q. The implicit solution G(y) = F(x) + C is therefore certified exactly when
both antiderivatives are -- differentiating it implicitly gives G'(y) y' = F'(x), i.e.
(1/g(y)) y' = f(x), i.e. y' = f(x) g(y), the original equation. No separate
differentiation engine is needed; the integrator's FTC certificate is the proof.

So y'=y gives log y = x + C; y'=y^2 gives -1/y = x + C; y'=1+y^2 gives arctan(y)=x+C;
y'=x*y gives log y = x^2/2 + C; y'=x/y gives y^2/2 = x^2/2 + C; and y'=(1+y^2)/x gives
arctan(y) = log x + C -- each certified. See `examples/171-ode-firstorder.lisp` and
the `cas_ode1` golden test.

## Tier 4: bivariate polynomial GCD

`lib/cas/mgcd.lisp` computes the greatest common divisor of bivariate polynomials
over Q. A polynomial f(x,y) is carried as a list of Q[y] coefficients in x (the ring
Q[y][x]). The GCD uses the classic split gcd(f,g) = gcd_y(cont f, cont g) *
pp(gcd over Q(y)[x] of f and g): the content is the Q[y]-gcd of the x-coefficients,
and the gcd over the field Q(y)[x] is obtained by the ordinary Euclidean algorithm
with Q(y) (rational functions in y) as the coefficient field -- which sidesteps
pseudo-division and subresultant bookkeeping. The field result is cleared of
y-denominators and made primitive over Q[y] (Gauss's lemma), recovering the gcd in
Q[x,y].

The result is checked the right way: a gcd must divide both inputs. Divisibility of f
by a primitive g is decided by division over Q(y)[x] (the remainder vanishes iff g
divides f over Q[x,y]). So gcd(x^2-y^2, (x+y)^2) = x+y, gcd((x+y)^2(x-1),
(x+y)(x-1)^2) = x^2+(y-1)x-y, and coprime inputs like x+y and x-y return 1 -- each with
both divisibility checks confirmed. See `examples/172-multivariate-gcd.lisp` and the
`cas_mgcd` golden test.

## Tier 4: bivariate squarefree factorization

`lib/cas/msqfree.lisp` separates the repeated factors of a bivariate polynomial over
Q by Yun's algorithm, built on the bivariate GCD. From gcd(f, df/dx) it produces
pairwise-coprime squarefree factors a_1, a_2, ... with f = prod a_i^i; each step is a
bivariate gcd and an exact bivariate division over Q(y)[x]. The result is certified
two ways: reconstruction (prod a_i^i must equal f up to a constant) and squarefreeness
of every factor (gcd(a_i, a_i') constant).

So (x+y)^2(x-1) factors as (x-1)^1 (x+y)^2, (x-1)^2(x+y)^3 as (x-1)^2 (x+y)^3, and
(x^2-y^2)^2 returns its radical x^2-y^2 at multiplicity 2 -- each with reconstruction
and squarefreeness confirmed. See `examples/173-squarefree-bivariate.lisp` and the
`cas_msqfree` golden test. (Full factorization into irreducibles -- Hensel lifting --
remains the next step.)
