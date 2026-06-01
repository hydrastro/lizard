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

## Tier 4: bivariate factorization into irreducibles

`lib/cas/mfactor.lisp` factors bivariate polynomials over Q into irreducibles by
evaluation, y-adic Hensel lifting, and recombination. For a squarefree, primitive,
monic-in-x f(x,y): a shift s is chosen so f(x,s) keeps full x-degree and stays
squarefree; f(x,s) is factored over Q into monic irreducibles; that factorization is
Hensel-lifted from mod (y-s) up to mod (y-s)^N with N > deg_y f; and the lifted
factors are recombined, the true factors being the subsets whose product divides f
exactly (checked with the bivariate division from mgcd). A squarefree split (Yun) is
run first so repeated factors are recovered with their multiplicities.

The entire result is gated by reconstruction: the product of the returned factors,
raised to their multiplicities, must equal f, so a wrong factorization is never
reported. So x^2 - y^2 factors as (x+y)(x-y); (x-y)(x+y)(x+1) is recovered as three
linear factors; (x+1)(x^2+y) splits a linear factor from an irreducible quadratic;
x^2+y and x^2-2 are returned whole as irreducibles; and (x+y)^2(x-1) comes back with
the square intact. See `examples/174-mfactor.lisp` and the `cas_mfactor` golden test.

## Tier 4: integer number theory

`lib/cas/numbertheory.lisp` adds exact arbitrary-precision number theory, self-contained
over the bignums. It provides extended Euclid (Bezout coefficients), modular
exponentiation and inverse, deterministic Miller-Rabin primality (the witness set
{2,3,...,37} is a proof for every n below 3.3e24 and correctly rejects Carmichael
numbers such as 561 and 1729, which fool the Fermat test), integer factorization by
trial division (which always finds a prime factor <= sqrt n of a composite, so it
terminates and is exact), the Euler totient and the divisor-count and divisor-sum
functions read off the factorization, the Chinese remainder construction, and
multiplicative order.

Everything is checkable. A factorization is gated by reconstruction -- the prime powers
multiply back to n and every base is certified prime -- so 360 = 2^3 * 3^2 * 5,
1234567 = 127 * 9721, and 1000000 = 2^6 * 5^6. Primality agrees with the known values
(97, 1000003, and the Mersenne prime 2^31-1 are prime; 561 and 1729 are not). The
totient satisfies Euler's theorem (2^phi(9) = 1 mod 9), a modular inverse times its
argument is 1, a CRT solution satisfies each congruence (x = 2 mod 3, x = 3 mod 5 gives
8), and the multiplicative order divides the totient. See
`examples/175-number-theory.lisp` and the `cas_numbertheory` golden test.

## Tier 4: Pade approximants

`lib/cas/pade.lisp` computes the [m/n] Pade approximant of a power series over Q: the
rational function P(x)/Q(x) with deg P <= m, deg Q <= n, Q(0) = 1, matching the series
S to order x^(m+n+1), i.e. S*Q - P = O(x^(m+n+1)). The n denominator coefficients solve
the exact rational linear system coming from the degree m+1..m+n coefficients of S*Q
(handled by the Gauss-Jordan solver from gosper.lisp); the numerator is then a
convolution of Q with S. Every approximant is verified by expanding S*Q - P as a series
and checking it vanishes to order m+n+1, so a wrong approximant is never returned.

This both condenses a series into a compact rational model and recovers a rational
function exactly from its expansion. The diagonal [2/2] approximant of exp is the classic
(1 + x/2 + x^2/12)/(1 - x/2 + x^2/12); the [1/1] of 1 + x + x^2 + ... is 1/(1-x); the
series (1, 2, 1, -1, -2, -1) of (1+x)/(1-x+x^2) is recovered exactly by its [2/2]
approximant; and log(1+x) gives (x + x^2/2)/(1 + x + x^2/6). See
`examples/176-pade.lisp` and the `cas_pade` golden test.

## Tier 4: real-root counting and isolation (Sturm)

`lib/cas/sturm.lisp` counts and isolates the real roots of a univariate polynomial over
Q exactly. The canonical Sturm chain (p, p', and successive negated remainders) gives,
by Sturm's theorem, the number of distinct real roots in a half-open interval as the
drop V(a) - V(b) in the chain's sign variations; making the polynomial squarefree first
turns every root simple, so the count is exact. All real roots sit inside the strict
Cauchy bound, so the total real-root count is the count over that bracket; isolation
bisects it, always splitting at a non-root midpoint, until each rational interval holds
exactly one root and exhibits a strict sign change p(lo)*p(hi) < 0 -- an independent
certificate layered on the rigorous Sturm count.

So x^2-2 has 2 real roots and x^2+1 has none; (x-1)(x-2)(x-3) gives 3, isolated as
(0, 3/2), (3/2, 21/8), (21/8, 15/4); the double-root polynomial (x-1)^2(x+2) reports 2
DISTINCT real roots; and a root of x^2-2 refines to a rational bracket of width below
1/1000 around sqrt(2). See `examples/177-sturm.lisp` and the `cas_sturm` golden test.

### Reader note

The lizard reader counts parentheses everywhere, including inside string literals and
comments, when deciding where a top-level form ends. Source files therefore keep round
parentheses balanced even in prose comments and display strings (half-open intervals in
labels are written with inequalities rather than a lone "(...]"). This is why the
example labels read "0 < x <= 2" instead of interval notation.

## Tier 4: partial fraction decomposition

`lib/cas/pfd.lisp` decomposes a rational function p/q over Q. The polynomial part is
divided out so deg r < deg q, then q is factored over Q into irreducible powers; the
decomposition p/q = s + sum_i sum_j A_ij/qi^j has exactly deg q unknown coefficients,
found all at once by clearing denominators and matching the deg q coefficients of
r = sum A_ij*(q/qi^j) -- a square rational linear system solved with the Gauss-Jordan
solver. Irreducible quadratics are kept intact, so the output is a genuine real partial
fraction with no complex numbers. Each decomposition is gated by recombination: s*q plus
the sum of the partial fractions must equal p exactly.

So 1/(x^2-1) = (-1/2)/(x+1) + (1/2)/(x-1); 1/(x^3+x) keeps the irreducible quadratic as
1/x + (-x)/(x^2+1); x^3/(x^2-1) returns a polynomial part x plus simple fractions; and
1/((x-1)^2(x+1)) expands the repeated factor into both its first and second powers. See
`examples/178-partial-fractions.lisp` and the `cas_pfd` golden test.

## Tier 4: closed-form linear recurrences

`lib/cas/linrec.lisp` solves constant-coefficient (C-finite) linear recurrences
a_n = c_1 a_{n-1} + ... + c_d a_{n-d}. The characteristic polynomial
x^d - c_1 x^{d-1} - ... - c_d is factored over Q; when it splits into linear factors the
closed form is a sum of P_i(n)*r_i^n with deg P_i below the multiplicity of the root r_i,
and the coefficients of the P_i are fixed by the initial conditions through a square
rational linear system. Repeated roots contribute the polynomial factors n, n^2, ...
The closed form is verified by evaluating it and comparing against the directly iterated
sequence over many terms, so a wrong form is never reported.

So a_n = 5a_{n-1} - 6a_{n-2} with a_0=0, a_1=1 gives 3^n - 2^n; the repeated-root
recurrence a_n = 2a_{n-1} - a_{n-2} with a_0=1, a_1=3 gives the arithmetic 2n+1; a
third-order recurrence with roots 1, 2, 3 is solved exactly; and mixed signs give
2^n + (-1)^n. When a root is irrational -- as for Fibonacci, whose characteristic
polynomial x^2 - x - 1 is irreducible over Q -- the solver declines a rational closed
form but still reports the polynomial and computes the terms 0, 1, 1, 2, 3, 5, 8, ...
exactly. This is the dual of Zeilberger, which discovers such recurrences. See
`examples/179-linear-recurrence.lisp` and the `cas_linrec` golden test.

## Tier 4: continued fractions and Pell's equation

`lib/cas/contfrac.lisp` expands continued fractions over the integers. A rational has a
finite expansion (the Euclidean algorithm); its convergents follow the standard
recurrence and the last one reconstructs the rational exactly. For a non-square d the
continued fraction of sqrt(d) is eventually periodic, computed with the exact integer
(m, Q, a) recurrence, and the convergent built from one period gives the FUNDAMENTAL
solution of Pell's equation x^2 - d y^2 = +-1, checked exactly -- so the periodicity and
the Pell identity together certify the expansion.

So 415/93 = [4; 2, 6, 7] and 355/113 (the classic pi approximation) reconstruct exactly;
sqrt(2) = [1; 2, 2, ...] yields the Pell solution (1, 1) with value -1; sqrt(7) yields
(8, 3) with value 1. Because the arithmetic is exact over the bignums, even the famously
enormous fundamental solution for sqrt(991) -- a thirty-digit x with x^2 - 991 y^2 = 1 --
is produced and verified. See `examples/180-continued-fractions.lisp` and the
`cas_contfrac` golden test.

## Tier 4: cyclotomic polynomials

`lib/cas/cyclotomic.lisp` builds the cyclotomic polynomials over Q from the product
identity prod_{d | n} Phi_d(x) = x^n - 1, via the exact recurrence
Phi_n = (x^n - 1) / prod_{d | n, d < n} Phi_d. Each Phi_d is computed only once by
threading a memoized table over the divisors in increasing order (every proper divisor of
a divisor of n is itself a divisor of n, so it is already in the table). Two independent
checks gate every result: the product over all divisors of n must rebuild x^n - 1
exactly, and deg Phi_n must equal the Euler totient phi(n) counted separately.

So Phi_1 = x - 1, Phi_4 = x^2 + 1, Phi_6 = x^2 - x + 1, Phi_12 = x^4 - x^2 + 1, and
Phi_15 = x^8 - x^7 + x^5 - x^4 + x^3 - x + 1. The well-known surprise is Phi_105, the
smallest cyclotomic polynomial whose coefficients leave {-1, 0, 1}: it has degree
phi(105) = 48 and coefficients equal to -2, which the module computes and certifies. See
`examples/181-cyclotomic.lisp` and the `cas_cyclotomic` golden test.

## Tier 4: power sums and symmetric functions

`lib/cas/powersums.lisp` computes the power sums p_k = sum_i a_i^k of a polynomial's
roots a_i from the coefficients alone, with no root finding. The elementary symmetric
functions e_j are the signed coefficients of the monic polynomial, and Newton's
identities turn them into p_1, p_2, ... to any order. The result is checked by a round
trip: reconstructing e_1..e_n from p_1..p_n through the inverse identity must return the
original symmetric functions, and p_0 must equal the degree.

Because only the coefficients are used, the sums are exact even when the roots are
irrational or complex. The roots 1, 2, 3 of x^3-6x^2+11x-6 give p_1..p_5 =
6, 14, 36, 98, 276; the roots of x^2-x-1 give the Lucas numbers 2, 1, 3, 4, 7, 11, ...
without ever naming the golden ratio; the imaginary roots of x^2+1 give the real sums
2, 0, -2, 0, 2, ...; and the roots of x^2-2 give 2, 0, 4, 0, 8, ... See
`examples/182-power-sums.lisp` and the `cas_powersums` golden test.

## Tier 4: modular square roots and quadratic residues

`lib/cas/modsqrt.lisp` adds the quadratic-residue toolkit. The Legendre symbol (a/p)
comes from Euler's criterion a^((p-1)/2) mod p; the Jacobi symbol (a/n) from the
reciprocity recursion that pulls out factors of two and flips signs; and Tonelli-Shanks
extracts a square root r with r^2 = a (mod p) when a is a residue (with the direct
(p+1)/4 formula when p = 3 mod 4). Every square root is checked by squaring it back, so a
wrong root is never returned; the Legendre symbol is cross-checked against a direct count
of roots for small primes, and the Jacobi symbol against its multiplicative behaviour.

So sqrt(2) mod 7 is 4 (and 3), sqrt(10) mod 13 is 7 (and 6), and 3 is correctly reported
as a non-residue mod 7; for a large prime p = 1 mod 4, the root of 314159^2 mod p is
recovered and verified by Tonelli-Shanks; and the classic Jacobi symbol (1001/9907) comes
out as -1. See `examples/183-modular-sqrt.lisp` and the `cas_modsqrt` golden test.

## Tier 4: sum of two squares (Fermat)

`lib/cas/twosquares.lisp` writes an integer as a sum of two squares whenever possible. A
prime p = 1 (mod 4) is split by Cornacchia's method -- take a square root of -1 modulo p
(from the modular-sqrt module) and run the Euclidean algorithm on (p, x) until the
remainder drops to sqrt(p); the last pair (a, b) gives a^2 + b^2 = p. A general n is
assembled from its prime-power factors through the Brahmagupta-Fibonacci identity
(a^2+b^2)(c^2+d^2) = (ac-bd)^2 + (ad+bc)^2, with 2 = 1^2 + 1^2 and a prime q = 3 (mod 4)
of even power 2k contributing (q^k, 0). Existence follows Fermat's criterion: n is a sum
of two squares iff every prime q = 3 (mod 4) divides it to an even power. Every
representation is gated by a^2 + b^2 = n.

So 13 = 3^2 + 2^2, 97 = 9^2 + 4^2, 50 = 1^2 + 7^2, 325 = 1^2 + 18^2, and the large prime
1000033 = 913^2 + 408^2 (Cornacchia over Tonelli-Shanks); while 3, 7, and 21 are honestly
reported as having no representation. See `examples/184-two-squares.lisp` and the
`cas_twosquares` golden test.

## Tier 4: discovering recurrences (Berlekamp-Massey)

`lib/cas/berlekamp-massey.lisp` finds, from the raw terms of a sequence, the SHORTEST
linear recurrence s_n = d_1 s_{n-1} + ... + d_L s_{n-L} that generates it -- the minimal
LFSR. It maintains a connection polynomial, forms the discrepancy at each step, and
corrects the polynomial by a shifted multiple of the previous one, growing the register
only when forced. The recurrence coefficients are the negated connection coefficients,
and the discovered recurrence is certified by replaying it against every given term.

This is the exact dual of the linear-recurrence solver: Berlekamp-Massey DISCOVERS the
recurrence, linrec SOLVES it. Composed, the two turn a list of numbers into a closed
form. So 0, 1, 1, 2, 3, 5, 8, 13 yields a_n = a_{n-1} + a_{n-2}; the powers of two yield
a_n = 2 a_{n-1}; the squares 0, 1, 4, 9, 16, 25 yield a_n = 3 a_{n-1} - 3 a_{n-2} +
a_{n-3}; and the terms of 3^n - 2^n yield (5, -6), which linrec then turns back into the
closed form 3^n - 2^n. See `examples/185-berlekamp-massey.lisp` and the `cas_bm` golden
test.

## Tier 4: transcendental limits via series

`lib/cas/translimit.lisp` resolves limits of indeterminate forms f(x)/g(x) as x -> 0 by
expanding numerator and denominator as exact power series over Q and comparing leading
orders -- L'Hopital's rule in series form, with no derivatives and no floating point.
Where the rational-limit module handles only ratios of polynomials, this works for
combinations of exp, sin, cos, log(1+x), and tan (the last as sin/cos), all built from
their exact Taylor coefficients. Each value is certified independently: for a finite
limit L the series of f - L*g must vanish to strictly higher order than g, which
re-derives the limit without reusing the leading-coefficient quotient.

So sin(x)/x and log(1+x)/x and tan(x)/x all tend to 1; (1 - cos x)/x^2 and
(e^x - 1 - x)/x^2 and (1 - cos x)/(x sin x) tend to 1/2; (sin x - x)/x^3 tends to -1/6;
x^2/sin(x) tends to 0 and sin(x)/x^2 is reported as infinite. See
`examples/186-transcendental-limits.lisp` and the `cas_translimit` golden test.

## Tier 4: Bernoulli numbers and power sums (Faulhaber)

`lib/cas/bernoulli.lisp` computes the Bernoulli numbers over Q from the recurrence
sum_{k=0}^{m} C(m+1,k) B_k = 0 (B_0 = 1), recovering 1, -1/2, 1/6, 0, -1/30, 0, 1/42, ...
Faulhaber's formula then turns the power sum into a polynomial in n,
S_k(n) = (1/(k+1)) sum_{j=0}^{k} C(k+1,j) B_j^{+} n^{k+1-j}, of degree k+1 with zero
constant term. Each polynomial is certified exactly by matching the directly computed sum
1^k + ... + m^k at m = 0, 1, ..., k+2 -- agreement past degree+1 points fixes the unique
polynomial, so a wrong formula is never returned.

So the module reproduces sum i = n(n+1)/2, sum i^2 = (2n^3+3n^2+n)/6, sum i^3 =
(n^2(n+1)^2)/4, and the degree-5 and degree-6 forms; evaluating them gives
sum_{1..10} i^3 = 3025 and sum_{1..100} i^2 = 338350. See `examples/187-faulhaber.lisp`
and the `cas_bernoulli` golden test.

## Tier 4: Stirling and Bell numbers

`lib/cas/stirling.lisp` computes the Stirling numbers of both kinds and the Bell numbers.
S(n,k) (second kind) counts partitions of n elements into k nonempty blocks; the unsigned
c(n,k) (first kind) counts permutations of n with k cycles; B(n) is the total number of
partitions. Both triangles are built row by row, so there is no exponential recomputation.

The numbers are certified by exact polynomial identities rather than spot values:
x^n = sum_k S(n,k) x^{falling k} ties the second kind to the falling-factorial basis;
x(x+1)...(x+n-1) = sum_k c(n,k) x^k makes the unsigned first kind the coefficients of the
rising factorial; sum_k c(n,k) = n!; and B(n) summed from the second-kind row matches B(n)
from the independent Bell recurrence B(n) = sum_k C(n-1,k) B(k). So the module reproduces
S(5,.) = (0,1,15,25,10,1), c(5,.) = (0,24,50,35,10,1), and the Bell numbers
1, 1, 2, 5, 15, 52, 203, 877, 4140. See `examples/188-stirling.lisp` and the
`cas_stirling` golden test.

## Tier 4: the integer partition function

`lib/cas/partitions.lisp` computes p(n), the number of unordered ways to write n as a sum
of positive integers, through Euler's pentagonal number theorem
p(n) = sum_{k>=1} (-1)^{k-1} ( p(n - k(3k-1)/2) + p(n - k(3k+1)/2) ), a roughly
O(n^{1.5}) recurrence evaluated with a memo table. The values are certified independently
against the generating function sum_n p(n) x^n = prod_{m>=1} 1/(1 - x^m), whose
coefficients are formed by multiplying the truncated series for each factor; the two
computations must agree on p(0)..p(N).

So p(0..10) = 1, 1, 2, 3, 5, 7, 11, 15, 22, 30, 42, and the module reproduces exactly the
classic p(50) = 204226 and p(100) = 190569292. See `examples/189-partitions.lisp` and the
`cas_partitions` golden test.

## Tier 4: primitive roots and the discrete logarithm

`lib/cas/discretelog.lisp` finds primitive roots and discrete logarithms modulo a prime.
A primitive root g generates the whole multiplicative group; rather than computing its
order directly, g is tested by checking g^((p-1)/q) != 1 (mod p) for each prime q dividing
p-1, and the smallest such g is returned. The discrete logarithm -- the exponent x with
g^x = h (mod p) -- is found by Shanks' baby-step giant-step method, tabulating g^0..g^{m-1}
with m = ceil(sqrt(p-1)) and then taking giant strides of g^{-m} from h until a baby step
matches, so x = i*m + j.

Both are certified independently: the primitive root by confirming its order is exactly
p-1 through the order-mod routine, and the logarithm by raising g back to the recovered
exponent and checking g^x = h. So the smallest primitive roots mod 7, 11, 13 are 3, 2, 2;
log_3(5) = 5 mod 7 and log_2(9) = 6 mod 11; a secret exponent 123 round-trips through
g^123 mod 1009 and back; and a target that is not a power of g is honestly reported as
having no solution. See `examples/190-discrete-log.lisp` and the `cas_discretelog` golden
test.

## Tier 4: the Mobius function and Dirichlet convolution

`lib/cas/mobius.lisp` adds multiplicative number theory built around Dirichlet
convolution. The Mobius function mu(n) is read off the factorization (zero on
non-squarefree n, otherwise (-1)^k for k distinct primes); the Mertens function sums it.
Dirichlet convolution (f * g)(n) = sum_{d|n} f(d) g(n/d) is provided as a higher-order
operation on arithmetic functions, with the constant 1, the identity, and epsilon = [n=1]
as first-class values, and Mobius inversion exposed as recovering a function from its
divisor-sum.

The structural identities serve as the certificates, each an independent check: mu * 1 =
epsilon, phi * 1 = N (the divisor sum of Euler's totient is n), phi = id * mu, and a full
Mobius-inversion round trip that recovers an arbitrary function from its summatory
function. So mu(6) = 1, mu(30) = -1, mu(4) = 0; the divisor sum of phi at 12 is 12; and
inverting the summatory function of n^2 returns n^2 exactly. See
`examples/191-mobius.lisp` and the `cas_mobius` golden test.

## Tier 4: Catalan numbers and binomial identities

`lib/cas/catalan.lisp` provides exact binomial coefficients via the multiplicative rule
C(n,k) = C(n,k-1)*(n-k+1)/k (integer at every partial product, so no factorials or
rationals appear) and the Catalan numbers in closed form C_n = C(2n,n)/(n+1). Each result
is gated by a classical identity used as an independent check: the Catalan convolution
C_{n+1} = sum_i C_i C_{n-i}, the ratio recurrence C_{n+1} = C_n*2(2n+1)/(n+2), Pascal's
rule, the row sum sum_k C(n,k) = 2^n and the alternating sum 0, Vandermonde's identity
sum_k C(m,k)C(n,p-k) = C(m+n,p), and the hockey-stick identity sum_{i=r}^n C(i,r) =
C(n+1,r+1).

So C(52,5) = 2598960 (the poker hands), the Catalan numbers run 1, 1, 2, 5, 14, 42, 132,
429, 1430, 4862, 16796, and every identity holds across the tested ranges. See
`examples/192-catalan.lisp` and the `cas_catalan` golden test.

## Tier 4: perfect numbers and amicable pairs

`lib/cas/perfect.lisp` studies the aliquot sum s(n) = sigma(n) - n. A number is perfect
when sigma(n) = 2n, abundant when s(n) > n, and deficient when s(n) < n; two numbers form
an amicable pair when each is the other's aliquot sum. The Euclid-Euler theorem builds an
even perfect number 2^(p-1)(2^p - 1) from each Mersenne prime 2^p - 1. Every
classification is decided through the sigma function from the number-theory module, an
independent multiplicative computation, which serves as the certificate.

So 6, 28, 496 are found by search; 8128 and the fifth perfect number 33550336 (from the
Mersenne exponent p = 13) are verified directly; 12 is abundant and 8 is deficient; and
(220, 284) and (1184, 1210) come out amicable while 2^11 - 1 = 2047 is correctly rejected
as a Mersenne prime. See `examples/193-perfect-numbers.lisp` and the `cas_perfect` golden
test.

## Tier 4: best rational approximation

`lib/cas/bestapprox.lisp` finds, for a rational x and a denominator bound N, the fraction
p/q with q <= N closest to x. By the classical theory the optimum is always a
continued-fraction convergent or a semiconvergent, so the module walks the convergents
h_i/k_i (threading the last two), and when the next convergent's denominator would exceed
N it forms the best semiconvergent (h_{i-2} + t h_{i-1})/(k_{i-2} + t k_{i-1}) with t =
floor((N - k_{i-2})/k_{i-1}), returning whichever of that and the last convergent lies
closer to x.

Optimality is certified independently by exhaustion: for every q from 1 to N the nearest
numerator is round(q x), and the smallest error found that way must equal the error of the
continued-fraction answer (ties between equally good fractions are allowed). This recovers
the historical approximations of pi -- 22/7 (Archimedes) within denominator 10, and
355/113 (Zu Chongzhi's Milue) within denominator 113, which stays optimal all the way to
denominator 16603 -- as well as 193/71 for e and 7/5 for the square root of 2. See
`examples/194-best-approximation.lisp` and the `cas_bestapprox` golden test.

## Tier 4: the Frobenius number and numerical semigroups

`lib/cas/frobenius.lisp` computes the Frobenius number -- the largest amount not payable
with coins whose gcd is 1 -- and the related numerical-semigroup data. Working modulo the
smallest coin m, a Bellman-Ford relaxation finds the Apery set dist[r], the least payable
amount congruent to r; then an amount n is payable exactly when dist[n mod m] <= n, the
Frobenius number is max(dist) - m, and the genus (number of unpayable amounts) is
(sum(dist) - m(m-1)/2)/m.

The results are cross-checked two independent ways: for two coins the Apery computation
must agree with the classical closed form ab - a - b, and for any coin set the Frobenius
number must itself be unpayable while the next m consecutive amounts are all payable. So
the two-coin cases reproduce 7, 17, 119; the genus of (4,7) is 9; and the Chicken McNugget
number for 6, 9, 20 comes out 43, the largest amount not expressible, with everything past
it payable. See `examples/195-frobenius.lisp` and the `cas_frobenius` golden test.

## Tier 4: binary quadratic forms and class numbers

`lib/cas/quadforms.lisp` reduces positive-definite binary quadratic forms and counts
classes. A form a x^2 + b xy + c y^2 is written (a b c) with discriminant b^2 - 4ac; for
D < 0 and a > 0, Gauss reduction brings it to the unique equivalent reduced form
(-a < b <= a <= c, with b >= 0 when a = c) using a swap (0 -1; 1 0) and translations that
normalise b, while accumulating the SL2(Z) transformation matrix M. The class number h(D)
is the number of primitive reduced forms of discriminant D, found by direct enumeration
(every reduced form has a <= sqrt(-D/3)).

Reduction carries its own proof, certified four independent ways: the discriminant is
unchanged, the output satisfies the reduced predicate, det M = 1, and applying M to the
original form reproduces the reduced form. So 3 4 5 reduces to 3 -2 4, the equivalent
forms 1 0 1 and 10 34 29 share the reduced representative 1 0 1, and the class numbers
come out h(-4) = 1, h(-23) = 3, h(-47) = 5, h(-71) = 7, and the Heegner discriminant
h(-163) = 1. See `examples/196-quadratic-forms.lisp` and the `cas_quadforms` golden test.

## The bridge, extended: kernel-certified higher-order derivatives

`lib/cas/diffn-cert.lisp` builds on the proof-carrying differentiator (diff-cert) to certify
HIGHER-order derivatives. Because the differentiator emits, for d/dx e, a term inhabiting
the kernel judgment Der (\x. e) (\x. e') and the result e' is again a ring term, it can be
iterated: the k-th derivative is obtained by applying it k times, and the entire chain
f -> f' -> ... -> f^(k) is certified by having the trusted kernel type-check every
single-step proof. A new linearity axiom der_neg_lin (d/dx (-g) = -g') lets the
derivatives of sin and cos close up, since cos' = -sin introduces negation.

The same machinery is a soundness witness. The kernel accepts exactly the term the
differentiator produced and rejects every other claimed derivative -- including
mathematically-equal-but-unsimplified variants, because the bare ring judgment carries no
simplification axioms. So exp is certified through several orders, sin and cos through the
third derivative, and the chain rule applied to sin(x*x) is verified, while wrong claims
such as (x*x)' = x, sin' = sin, or the rule der_sin : Der sin sin are all rejected by the
kernel. A correct derivative is the only thing that can inhabit the type. See
`examples/197-higher-derivative-certificates.lisp` and the `cas_diffn` golden test.

## Tier 4: Pratt primality certificates

`lib/cas/pratt.lisp` turns primality from a test into a checkable PROOF. By Lucas's
theorem n is prime iff some witness a has multiplicative order exactly n-1, that is
a^(n-1) = 1 (mod n) while a^((n-1)/q) /= 1 for every prime q dividing n-1. Establishing
that the q are themselves prime makes the certificate recursive: it carries the
factorisation of n-1 together with a Pratt certificate for each prime factor, bottoming
out at 2. The whole object is a tree (n a ((q1 e1 cert) ...)).

The verifier re-derives every node from scratch -- the prime powers multiply to n-1,
a^(n-1) = 1, each a^((n-1)/qi) /= 1, and each sub-certificate -- so it shares no work with
the builder and a forged certificate (for instance one with a non-generator witness) is
rejected. The result is cross-checked against the independent Miller-Rabin test from the
number-theory module, and the two always agree. So 1009 (witness 11), 9973, and 104729
come with verified certificates, the Carmichael number 561 has none, and tampering with a
certificate makes verification fail. See `examples/198-pratt-certificates.lisp` and the
`cas_pratt` golden test.

## Tier 4: the Lucas-Lehmer test and Lucas sequences

`lib/cas/lucas.lisp` adds the Lucas-Lehmer test, the exact primality proof for Mersenne
numbers M_p = 2^p - 1. With s_0 = 4 and s_{i+1} = s_i^2 - 2 (mod M_p), M_p is prime iff
s_{p-2} = 0; only p-2 modular squarings are required, so primes well past trial division
are certified at once. The exponents that pass are exactly those behind the even perfect
numbers, tying back to the perfect-number module. The module also provides the Lucas
sequences U_n, V_n with parameters (P, Q) -- specialising to the Fibonacci and Lucas
numbers at (1, -1) -- with the companion identity V_n^2 - D U_n^2 = 4 Q^n (D = P^2 - 4Q)
used as a certificate.

So the Mersenne-prime exponents below 40 come out 2, 3, 5, 7, 13, 17, 19, 31; M_127 =
170141183460469231731687303715884105727, a 39-digit prime, is confirmed instantly; the
Lucas-Lehmer verdicts agree with the independent Miller-Rabin test; and the Lucas identity
holds across the tested ranges. See `examples/199-lucas-lehmer.lisp` and the `cas_lucas`
golden test.

## Tier 4: the Gaussian integers Z[i]

`lib/cas/gaussint.lisp` implements arithmetic in the Gaussian integers. Z[i] is a
Euclidean domain under the norm N(a+bi) = a^2 + b^2: division rounds z*conj(w)/N(w) to the
nearest Gaussian integer, the remainder reduces the norm, and a Euclidean gcd follows. The
units are the four elements of norm 1. A rational prime p = 2 ramifies as a unit times
(1+i)^2, a prime p = 3 (mod 4) stays inert, and a prime p = 1 (mod 4) splits as
(a+bi)(a-bi) with a^2+b^2 = p -- precisely the sum-of-two-squares decomposition, so the
module reuses that result to produce the Gaussian prime factors.

Three independent identities certify the implementation: the norm is multiplicative,
Euclidean division genuinely reduces the norm (z = qw + r with N(r) < N(w)), and the
computed gcd divides both inputs exactly. So 5 = (2+i)(2-i), 13 = (3+2i)(3-2i), 97 =
(9+4i)(9-4i); gcd(5, 2+i) = 2+i; 2+i, 1+i, and the inert 3 and 7 are Gaussian primes while
5 is not. See `examples/200-gaussian-integers.lisp` and the `cas_gaussint` golden test.

## Tier 4: finite-difference calculus and Newton interpolation

`lib/cas/findiff.lisp` is the discrete analogue of calculus. From values at the integer
nodes 0..n the forward difference operator (D y)_i = y_{i+1} - y_i builds a difference
table whose leading entries D^k y_0 are the Newton coefficients of the unique interpolating
polynomial P(x) = sum_k (D^k y_0) C(x,k), returned in monomial form over the rationals.
Summation is the discrete integral: by the hockey-stick identity sum_{x=0}^{m-1} C(x,k) =
C(m,k+1), so sum_{i=0}^{m-1} P(i) = sum_k (D^k y_0) C(m,k+1) in closed form, and the
antidifference Q (with Q(x+1) - Q(x) = P(x)) comes from shifting the Newton coefficients up
one index. Faulhaber's power-sum polynomials are obtained by interpolating x^p.

Every result is checked independently: the interpolant reproduces the data at every node,
and each closed-form sum is compared against a brute-force sum. So 0,1,4,9,16 recovers
x^2, the sum of squares to 9 is 285 and of cubes to 10 is 3025, the power sums match
Faulhaber's formulas through several degrees, and each antidifference is verified to
difference back to its integrand. See `examples/201-finite-differences.lisp` and the
`cas_findiff` golden test.

## Tier 4: the Stern-Brocot tree

`lib/cas/sternbrocot.lisp` realises the Stern-Brocot tree, in which every positive rational
appears exactly once and is reached by a unique Left/Right mediant path from 1/1. Starting
from the boundary fractions 0/1 and 1/0, each node is their mediant (a+c)/(b+d); comparing
a target to the mediant chooses the branch, and reversing the moves reconstructs the
rational. The run-length encoding of the path is exposed, and its tie to continued
fractions is direct -- 355/113 has CF [3;7,16] and run-lengths 3,7,15.

Three independent certificates witness the structure: path round-trip (reconstruction
returns the original, verified over all p/q with small numerator and denominator), the
Farey-neighbour determinant c*b - a*d = 1 at every node (so every node is automatically in
lowest terms), and level distinctness (the 2^k rationals at depth k are pairwise distinct
and reduced, witnessing that no rational repeats). See `examples/202-stern-brocot.lisp`
and the `cas_sternbrocot` golden test.

## Tier 4: Pollard's rho integer factorization

`lib/cas/pollard.lisp` factors integers far beyond the reach of trial division. Iterating
the map f(x) = x^2 + c (mod n) under Floyd's two-pointer cycle detection, gcd(|x - y|, n)
exposes a nontrivial divisor of a composite n, with the constant c bumped if a run
collapses. Full factorization strips small primes and then recurses with rho, testing
primality by the deterministic Miller-Rabin from the number-theory module so each piece is
split until prime.

The result is checked the only way that matters: the factors multiply back to n and every
one is prime. So 8051 = 83 * 97, the Project-Euler number 600851475143 = 71 * 839 * 1471 *
6857, Fermat's fifth number 4294967297 = 641 * 6700417 (Euler's 1732 factorization), and
123456789 = 3^2 * 3607 * 3803 all come out fully factored and certified, while genuine
primes such as 1000000007 are returned whole. See `examples/203-pollard-rho.lisp` and the
`cas_pollard` golden test.

## Tier 3: polynomial factorization over finite fields F_p

`lib/cas/ffactor.lisp` factors polynomials over a prime field F_p into monic irreducibles
by the full Cantor-Zassenhaus method. Squarefree decomposition strips repeated factors via
f / gcd(f, f'), with a p-th-root step when f' = 0 (in characteristic p a polynomial with
zero derivative is a p-th power, and in F_p each coefficient is its own p-th root).
Distinct-degree factorisation then peels off the product of degree-i irreducibles by
gcd(f, x^(p^i) - x) using the Frobenius map, and equal-degree blocks are split with the
trace map a + a^2 + ... + a^(2^(d-1)) when p = 2 and with a^((p^d-1)/2) - 1 for odd p,
trying a deterministic stream of polynomials so the routine is reproducible. A standalone
irreducibility test (x^(p^n) = x mod f, with coprimality to x^(p^(n/r)) - x for each prime
r | n) is included.

The factorisation is gated two independent ways: the product of the prime powers must
reconstruct the monic input mod p, and every factor must pass the irreducibility test. So
x^2 - 1 = (x+1)(x+6) and x^4 + 5 = (x+5)(x+2)(x^2+4) over F_7; x^3 + 1 = (x+1)^3 over F_3
through the p-th-root branch; x^4 + 1 = (x+1)^4 over F_2; and x^2 + 1 is irreducible over
F_7 but splits over F_5. See `examples/204-finite-field-factorization.lisp` and the
`cas_ffactor` golden test.

## Tier 3: finite field GF(p^n) arithmetic

`lib/cas/gfp.lisp` builds the field with p^n elements as F_p[x]/(m), finding the smallest
monic irreducible m of degree n with the irreducibility test from the factoriser.
Elements are polynomials of degree < n; addition is coefficientwise mod p, multiplication
reduces modulo m, and the inverse uses Fermat's little theorem in the field (a^(p^n - 2),
since the multiplicative group has order p^n - 1). The module also computes element orders
and finds primitive elements.

Four independent facts certify the construction: the modulus is irreducible (so the
quotient is a field), every nonzero element is invertible, the Frobenius identity
a^(p^n) = a holds for every element, and a primitive element's successive powers enumerate
all p^n - 1 nonzero elements exactly once. So GF(8) = F_2[x]/(x^3+x+1), GF(16), GF(9),
GF(25), and GF(27) are all built and pass every law; in GF(8), x is primitive of order 7,
x*(x+1) = x^2 + x, and x^(-1) = x^2 + 1. See `examples/205-extension-fields.lisp` and the
`cas_gfp` golden test.

## Tier 3: elliptic curves over F_p

`lib/cas/ec.lisp` implements the group of points on y^2 = x^3 + a x + b over a prime field
F_p (with 4a^3 + 27b^2 /= 0). Points are the affine solutions together with a point at
infinity O. Addition uses the chord slope (y2-y1)/(x2-x1) for distinct points and the
tangent slope (3x1^2+a)/(2y1) for doubling, with P + (-P) = O; scalar multiplication is
double-and-add. The group order is p + 1 + sum over x of the Legendre symbol of
x^3 + a x + b, and a point's order is the least k with kP = O.

The implementation is held to the defining structure, every clause an independent check:
the curve is nonsingular, the sum of two points is again on the curve (closure verified
over every pair), the law is associative (verified over every triple), O and -P are
identity and inverse, the count obeys the Hasse bound |#E - (p+1)| <= 2 sqrt p, and the
order of every point divides #E (Lagrange). So y^2 = x^3 + 2x + 2 over F_17 has 19 points
(a prime, so the group is cyclic) with P = (5,1) a generator of order 19; and the curves
over F_5, F_11, F_23 (orders 9, 13, 20) obey all the same laws. See
`examples/206-elliptic-curves.lisp` and the `cas_ec` golden test.

## Tier 3: Shamir threshold secret sharing

`lib/cas/shamir.lisp` implements (t, n) threshold secret sharing over F_p. A secret is
placed as the constant term of a degree t-1 polynomial; the n shares are its evaluations
at x = 1..n. Any t shares determine the polynomial by Lagrange interpolation and recover
the secret as the value at 0, while any t-1 shares leave it completely undetermined.

Two independent facts certify the scheme: reconstruction from any t of the n shares returns
the original secret (checked over several different t-subsets), and the security property
holds -- given only t-1 shares, two distinct secrets each admit a degree t-1 polynomial
reproducing exactly those shares, so t-1 shares carry no information. So a (3,5) sharing of
1234 over F_2003 is recovered by any three shares but two shares give an unrelated value;
and (4,7) and (2,4) sharings behave likewise. See `examples/207-secret-sharing.lisp` and
the `cas_shamir` golden test.

## Tier 3: Reed-Solomon error-correcting codes

`lib/cas/reedsolomon.lisp` implements Reed-Solomon codes over F_p in evaluation form. A
k-symbol message is the coefficient list of a polynomial of degree < k, and its codeword is
the evaluations at n distinct points; this is a maximum-distance-separable [n, k] code with
minimum distance n - k + 1, correcting up to t = floor((n - k) / 2) symbol errors.

Decoding is the Berlekamp-Welch method. If the received word differs from a codeword in at
most t places, there is an error-locator E (monic of degree t, vanishing at the error
positions) and a numerator N of degree < t + k with N(x_i) = r_i E(x_i) at every point.
These relations are linear in the coefficients of E and N, so a Gaussian elimination over
F_p (with a particular-solution extractor for the rank-deficient case of fewer errors)
recovers them, and the message is the exact quotient N / E.

The whole scheme is gated by the round trip: encode a message, corrupt up to t positions,
and decoding must return the original message. So a [10,4] code over F_11 (distance 7,
t = 3) recovers its message from any one, two, or three symbol errors and reports failure
at four; and [12,6] over F_13 and [8,2] over F_29 behave likewise. See
`examples/208-reed-solomon.lisp` and the `cas_reedsolomon` golden test.
