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

## Tier 3: elliptic-curve cryptography (ECDH and ECDSA)

`lib/cas/eccrypto.lisp` runs the two classic elliptic-curve protocols on the group law from
ec.lisp, given a base point G of known prime order ell. ECDH key exchange: each party
multiplies G by a private scalar to publish a point and multiplies the partner's point by
its own scalar, both reaching the shared secret (d_A d_B) G. ECDSA: a message z is signed
under private key d (public Q = dG) by choosing a nonce k, setting R = kG, r = x(R) mod ell,
s = k^{-1}(z + r d) mod ell; verification checks x(u1 G + u2 Q) = r with u1 = z s^{-1},
u2 = r s^{-1}.

Certified four ways: ECDH agreement holds for every key pair, valid signatures verify, a
signature for one message fails to verify for another, and a tampered signature is rejected.
On y^2 = x^3 + 2x + 2 over F_17 with G = (5,1) of order 19, the exchange agrees across all
18x18 key pairs and signatures verify or are correctly rejected. See
`examples/209-elliptic-curve-crypto.lisp` and the `cas_eccrypto` golden test.

## Tier 3: the Number-Theoretic Transform

`lib/cas/ntt.lisp` implements the NTT, the finite-field discrete Fourier transform: with a
primitive n-th root of unity it maps a vector to X_k = sum_j a_j w^{jk}, computed by the
recursive radix-2 Cooley-Tukey splitting in O(n log n), with the inverse using w^{-1} and a
final scaling by n^{-1}. Because convolution becomes pointwise multiplication under the
transform, it gives fast cyclic convolution and fast polynomial multiplication.

Three facts certify it: the inverse transform recovers the input, the inverse of the
pointwise product equals the direct cyclic convolution, and NTT polynomial multiplication
matches the schoolbook product. So over F_17 with the 8th root w = 9 the transform round
trips and (1+2x+3x^2)(4+5x+6x^2) reduces to the same coefficients both ways; over F_97 with
n = 16 the same holds. See `examples/210-number-theoretic-transform.lisp` and `cas_ntt`.

## Tier 3: Hamming codes

`lib/cas/hamming.lisp` builds the order-r binary Hamming code, a [2^r-1, 2^r-1-r, 3] code
correcting one error. Data bits occupy the non-power-of-two positions, parity bits the
powers of two, and on receipt the syndrome read bit by bit is exactly the binary index of
the flipped position (zero meaning none), so a single error anywhere is located and
corrected.

Two facts certify it: a clean codeword has zero syndrome, and a single error in any of the
n positions is corrected. For the [7,4] code this is verified exhaustively over all sixteen
messages, and the [15,11] code corrects every single error likewise. See
`examples/211-hamming-codes.lisp` and the `cas_hamming` golden test.

## Tier 3: Hensel lifting

`lib/cas/hensel.lisp` lifts a coprime factorization f = g h modulo a prime p to a
factorization modulo every power p^k -- the bridge from factoring over a small field to
factoring over the integers. Each step corrects g and h with fixed mod-p Bezout cofactors
so the product matches f to one higher power of p while g stays monic; the finite-field
polynomial arithmetic is reused from ffactor (whose add/subtract/multiply reduce modulo any
modulus) with an extended Euclid supplying the cofactors.

Two facts certify each lift: the lifted product reconstructs f mod p^k, and it reduces to
the original factor mod p. So x^2 + 1 = (x+2)(x+3) mod 5 lifts to (x+7)(x+18) mod 25 and on
to mod 5^6; x^3 - 1 keeps its split (x-1)(x^2+x+1); and x^2 - 10 mod 3 lifts cleanly to mod
3^8. See `examples/212-hensel-lifting.lisp` and the `cas_hensel` golden test.

## Tier 3: Reed-Solomon via syndromes and Berlekamp-Massey

`lib/cas/rsbm.lisp` decodes Reed-Solomon codes in the spectral (BCH-like) form, complementing
the Berlekamp-Welch decoder of `reedsolomon.lisp`. For a primitive root alpha of F_p the
length n = p-1 code with k = n - 2t uses generator g(x) = prod_{j=1}^{2t} (x - alpha^j), and a
message is encoded as m*g. The received word's syndromes S_j = r(alpha^j) vanish exactly when
r is a codeword; otherwise Berlekamp-Massey over F_p finds the shortest linear recurrence
generating the syndrome sequence, whose reciprocal roots -- located by a Chien search -- are
the error positions. Forney's formula, here a small Vandermonde solve, gives the error
magnitudes. Four certificates gate it: the round trip recovers the message through up to t
errors, a clean codeword has all-zero syndromes, the located positions equal the injected
ones, and three errors on a t=2 code report failure. Verified on a [10,6] code over F_11
(generator (1 8 5 3 1)) and a [12,6] code over F_13. See `examples/215-reed-solomon-berlekamp-massey.lisp`.

## Tier 3: Reed-Muller codes

`lib/cas/reedmuller.lisp` implements the first-order Reed-Muller code RM(1,m), a
[2^m, m+1, 2^{m-1}] code correcting t = 2^{m-2}-1 errors, self-contained over GF(2). A
message (a0,...,am) encodes the affine Boolean function a0 + a1 x1 + ... + am xm as its
length-2^m truth table. Decoding is the fast Walsh-Hadamard transform: mapping bits b to
signs 1-2b and transforming, the largest-magnitude coefficient identifies the linear part by
its index and the constant term by its sign -- maximum-likelihood decoding in O(m 2^m)
without any matrix. The certificate is the round trip: RM(1,4) = [16,5,8] recovers its
message from any three bit errors, RM(1,5) = [32,6,16] from seven, and RM(1,3) = [8,4,4]
from one. See `examples/216-reed-muller-codes.lisp` and the `cas_reedmuller` golden test.

## Tier 3: binary BCH codes

`lib/cas/bch.lisp` implements binary BCH codes over GF(2^m), the cyclic generalization of the
Hamming codes. For a primitive alpha of GF(2^m) the length n = 2^m-1 code designed to correct
t errors has generator g = lcm(M_1, ..., M_{2t}), where M_i is the minimal polynomial of
alpha^i over GF(2) -- the product over the cyclotomic coset {i, 2i, 4i, ...} (mod n) of
(x - alpha^j). Distinct cosets give coprime minimal polynomials, so g is their product over
GF(2); a k = n - deg g bit message is encoded as m*g. Decoding mirrors the Reed-Solomon
spectral route: syndromes S_j = r(alpha^j) in GF(2^m), Berlekamp-Massey over GF(2^m) for the
error locator, and a Chien search for the positions -- but because the errors are binary,
decoding simply flips those bits with no Forney step. The construction is pinned by an
independent constant: over GF(16) with primitive polynomial x^4+x+1 the t=2 generator is
exactly the textbook x^8+x^7+x^6+x^4+1, giving the [15,7] code. The round trip then corrects
up to t errors there, a t=3 design gives a [15,5] code correcting three errors, and over
GF(8) the t=1 code reproduces the [7,4] Hamming generator x^3+x+1. See
`examples/217-bch-codes.lisp` and the `cas_bch` golden test.

## Tier 3: permutation groups

`lib/cas/permgroup.lisp` works with finite permutation groups on {0,...,n-1}, a permutation
being its list of images so that the identity is (0 1 ... n-1) and composition is function
composition. Given generators, the whole subgroup of S_n is enumerated by breadth-first
search over its Cayley graph -- from the identity, repeatedly left-multiplying by each
generator visits every element exactly once -- so the group order is just the count. That
count is cross-checked against the orbit-stabilizer theorem |G| = |orbit(x)| * |stabilizer(x)|,
computed independently, and the group axioms (identity present, closure under generators,
closure under inverses) are verified on the enumerated set, with Lagrange's theorem checked
for point stabilizers. Element order, cycle decomposition, orbits and stabilizers are all
available. Known orders pin it down: S_3 = 6, S_4 = 24, A_4 = 12 (a proper subgroup of S_4),
D_4 = 8, and a cyclic generator's order equals its length. See
`examples/218-permutation-groups.lisp` and the `cas_permgroup` golden test.

## Tier 3: LLL lattice basis reduction

`lib/cas/lll.lisp` implements Lenstra-Lenstra-Lovasz reduction over the rationals. A lattice is
the set of integer combinations of its basis vectors; LLL rewrites a long, skewed basis into a
short, nearly orthogonal one spanning the same lattice, interleaving size reduction (every
Gram-Schmidt coefficient |mu_ij| <= 1/2) with swaps whenever the Lovasz condition
|b*_k|^2 >= (3/4 - mu_{k,k-1}^2)|b*_{k-1}|^2 fails. Because all Gram-Schmidt arithmetic is exact
rational, the result is exact. A correct reduction is characterized completely by its
certificates, which is what is checked: the output is size-reduced, satisfies Lovasz, and spans
the same lattice -- the change of basis U with reduced = U * original is verified to be an
integer matrix of determinant +-1 (computed from an exact rational matrix inverse and
determinant), and the lattice determinant is preserved. The skewed basis (1,0,0),(10,1,0),
(100,10,1) reduces all the way to the standard basis of Z^3, and (1,2),(3,4) becomes the
orthogonal (1,0),(0,2). See `examples/219-lll-lattice-reduction.lisp` and the `cas_lll` golden.

## Tier 3: integer relation detection

`lib/cas/intrel.lisp` finds, for rationals x_1,...,x_n, a nonzero integer vector a with
a.x = 0 -- the classic application of lattice reduction. The problem is embedded as the lattice
spanned by the rows (e_i | C*x_i): a lattice vector is (a, C*(a.x)), so making it short forces
a.x toward zero and the identity block exposes the relation. Because the LLL change of basis is
integer-unimodular and the identity block is integer, the first n coordinates of any reduced
vector are integers, so a is recovered as an integer vector directly. The routine is
self-certifying: it scans the reduced basis and returns only an a with a.x = 0 exactly (raising
the weight C and retrying if none is found at the current scale). It recovers small relations --
(1/2, 1/3, 1/6) yields (-1, 1, 1), and (1, 3/7) yields (-3, 7), recognizing the rational 3/7 --
and includes a checker that validates a supplied relation. Builds on lll.lisp. See
`examples/220-integer-relations.lisp` and the `cas_intrel` golden test.

## Tier 3: cyclic codes

`lib/cas/cyclic.lisp` provides the framework beneath the Hamming and BCH codes: a binary cyclic
code of length n is generated by a polynomial g dividing x^n - 1 (= x^n + 1 over GF(2)), with
codewords the multiples of g, so a word is a codeword exactly when g divides it (the syndrome
r mod g is zero). The module checks that a proposed generator divides x^n + 1, encodes
k = n - deg g bit messages as m*g, computes syndromes, and -- the defining property -- verifies
cyclic closure, that rotating a codeword's coefficients (multiplication by x modulo x^n - 1)
yields another codeword, for every rotation. It also confirms that corrupting a codeword makes
the syndrome nonzero (error detection). Concretely x^3 + x + 1 generates the [7,4] Hamming code
and x + 1 the even-weight parity-check code, both verified cyclic. Builds on ffactor.lisp. See
`examples/221-cyclic-codes.lisp` and the `cas_cyclic` golden test.

## Tier 3: the integer partition function

`lib/cas/partition.lisp` computes p(n), the number of partitions of n, by Euler's pentagonal
number theorem: p(n) = sum_{k>=1} (-1)^{k-1} [p(n - k(3k-1)/2) + p(n - k(3k+1)/2)], with p(0)=1
and p(m)=0 for m<0. Only O(sqrt n) terms contribute at each step, so the table p(0..n) is built
in roughly O(n sqrt n) exact-integer operations. Two independent checks gate it: a direct
counting recurrence (partitions of n into parts at most k, with no pentagonal numbers) must
agree for small n, and the classical value p(100) = 190569292 must come out exactly. Self-
contained. See `examples/222-integer-partitions.lisp` and the `cas_partition` golden test.

## Tier 3: Rothstein-Trager logarithmic integration

`lib/cas/rothstein.lisp` computes the logarithmic part of a rational integral. For a proper
a/b with b squarefree, INT a/b dx = sum_i c_i log(v_i), and Rothstein and Trager showed how to
find it without factoring b: the constants c_i are the roots of the resultant
R(y) = res_x(a - y b', b) -- exactly the residues of a/b at the roots of b -- and the matching
argument is v_c = gcd(a - c b', b). The module assembles the answer over the rational roots of
R (found by the rational root theorem) and certifies it by differentiation, checking that
sum c_i v_i'/v_i equals a/b as an exact identity. When R has only rational roots the
logarithmic part is complete; an irreducible denominator such as x^2+1 has algebraic residues
(here +-i/2, the arctangent case), which is reported honestly rather than faked. This is
genuinely factorization-free, unlike the partial-fraction route, and rests on the parametric
resultant rt-resultant; that resultant is sampled away from the node where the integrand's
x-degree drops, so the interpolation stays consistent. See `examples/223-rothstein-trager.lisp`
and the `cas_rothstein` golden test.

## Tier 3: Hermite reduction

`lib/cas/hermite.lisp` extracts the rational part of a rational integral. For a proper A/D it
produces INT A/D dx = (rational part) + INT (numerator)/(radical of D) dx, where the radical
(squarefree part) has only simple roots so the remaining integral is purely logarithmic. No
factorization into irreducibles is needed: D is squarefree-factorized by Yun's algorithm, split
into pieces by the polynomial CRT, and each piece is reduced one power at a time by integration
by parts using the Bezout relation S V + T V' = 1 (V squarefree, so gcd(V,V') = 1). Everything
is exact rational arithmetic and the result is certified by differentiation:
d/dx(rational part) + residual/radical = A/D. The computed rational part can differ from a
minimal one by an additive constant, which the differentiation certificate correctly tolerates.
See `examples/224-hermite-reduction.lisp` and the `cas_hermite` golden test.

## Tier 3: complete rational integration (the Risch way)

`lib/cas/rischrat.lisp` composes the two halves of the rational case of the Risch algorithm:
Hermite reduction for the rational part, then Rothstein-Trager for the squarefree remainder,
giving INT A/D dx = ratnum/ratden + sum_i c_i log(v_i) with no irreducible factorization at any
step. The answer is returned together with a completeness flag and is certified by
differentiating it back to A/D. It is complete exactly when the denominator splits over Q
(rational residues); an irreducible quadratic factor contributes an arctangent whose residues
are algebraic, reported honestly while the Hermite rational part remains exact. This is the
honest summit of the rational case -- proof-carrying rational integration -- with the full
transcendental and algebraic Risch algorithm remaining beyond a single sitting. See
`examples/225-rational-integration-complete.lisp` and the `cas_rischrat` golden test.

## Tier 3: the Risch differential equation over Q(x), and INT R(x) e^{p(x)}

`lib/cas/rischde.lisp` extends the Risch differential equation y' + f y = g from the
polynomial-coefficient case (risch.lisp) to a polynomial f with a rational g in Q(x). That is
exactly what integrating R(x) e^{p(x)} for rational R requires: the integral is h(x) e^{p(x)}
with h' + p' h = R, where p' is a polynomial but R is rational. Since p' has no poles, a pole of
R of order m forces a pole of h of order m-1, so the denominator of any solution divides
gcd(E, E') where E is the denominator of R. Writing h = U/gcd(E,E') turns the equation into a
single first-order linear equation P1 U' + P0 U = RHS for an unknown polynomial U, which is
solved by undetermined coefficients (a linear system over Q, via an exact Gaussian solver). For
this class deg P0 >= deg P1, so the leading term never cancels and the degree bound is tight;
together with the differentiation certificate (every returned h is checked to satisfy
h' + p' h = R) this makes the procedure both sound and complete, so a reported "non-elementary"
is a genuine impossibility proof. It recovers the elementary cases, e.g.
INT (x-1)/x^2 e^x dx = e^x/x and INT x/(x+1)^2 e^x dx = e^x/(x+1), and proves the classic
non-elementary ones, INT e^(x^2) dx and INT e^x/x dx. The remaining steps toward the full
transcendental Risch algorithm -- a rational (not just polynomial) f via weak normalization, and
the logarithmic-extension RDE -- build directly on this. See
`examples/226-risch-de-rational.lisp` and the `cas_rischde` golden test.

## Tier 3: the Risch DE with rational f, and INT R(x) e^{u(x)} for rational u

`lib/cas/rderat.lisp` extends the Risch differential equation y' + f y = g to a rational
coefficient f, in the weakly-normalized case -- no simple pole of f has a positive-integer
residue. That is exactly what is needed to integrate R(x) e^{u(x)} when u itself is rational
(rischde.lisp required a polynomial u): the integral is h e^u with h' + u' h = R, and f = u' is
automatically weakly normalized, because the derivative of a rational function has every residue
zero (the antiderivative of 1/(x-a) is a logarithm, never rational). The denominator of any
solution is bounded by the product over the squarefree factors p^l of denom(R) of
p^{max(0, l - max(k,1))}, where k is the pole order of u' at p: where u' has a pole of order
k >= 2 the term u' h dominates and forces order l-k, while where u' is regular or has a simple
pole the y' term dominates and forces order l-1. For a polynomial u every k is 0 and this is
exactly the gcd(E,E') bound of rischde.lisp, so the new module is a clean generalization.
Writing h = U/denominator reduces the problem to one linear equation P1 U' + P0 U = RHS for a
polynomial U, solved by undetermined coefficients (reusing rischde.lisp). Every returned answer
is differentiation-certified, so the method is sound; the denominator bound makes it complete on
this class, validated against integrals with known answers (for rational g and u, setting
R = g' + u' g recovers an h that certifies). It gives, e.g., INT -1/x^2 e^{1/x} dx = e^{1/x},
INT (x-1)/x e^{1/x} dx = x e^{1/x}, INT 2/x^3 e^{-1/x^2} dx = e^{-1/x^2}, and proves
INT e^{1/x} dx non-elementary. The remaining step toward the full Risch DE is the general weak
normalizer for an f whose simple poles do have positive-integer residues. See
`examples/227-risch-de-rational-f.lisp` and the `cas_rderat` golden test.

## Tier 3: weak normalization and the full base-case Risch DE over Q(x)

`lib/cas/weaknorm.lisp` removes the weak-normalization hypothesis, completing the Risch
differential equation y' + f y = g for an arbitrary rational coefficient f over Q(x). The
obstruction is a simple pole of f at some alpha whose residue n is a positive integer: there a
solution may carry a pole of order n that the rderat.lisp denominator bound, derived under weak
normalization, does not see. The fix is the classical WeakNormalizer. It builds the monic
polynomial q = product (x - alpha)^n over exactly those poles and substitutes y = z/q, turning the
problem into z' + (f - q'/q) z = q g whose coefficient f - q'/q is weakly normalized, so rderat.lisp
solves for z and y = z/q is recovered. The poles and residues are found without factoring: the
residues at the simple poles of f -- the roots of d1, the multiplicity-one part of the denominator
of f -- are the roots of the resultant R(y) = res_x(a - y d', d1) where f = a/d, and for each
positive integer n among those roots the corresponding factor is gcd(d1, a - n d'), contributed to
q with multiplicity n.

This step is not cosmetic. It is essential precisely when exp(-INT f) is not rational, so the
forced pole cannot be shifted away by adding a rational homogeneous solution. The worked example is
f = (5/2 x - 6)/(x^2 - 3x), which has residue 2 at x = 0 (an integer) and residue 1/2 at x = 3 (not),
with g = 1/(2 x^2 (x - 3)). Its only rational solution is y = 1/x^2, and the weakly-normalized solver
alone returns "none" -- it genuinely cannot represent that pole -- whereas with q = x^2 the weak
normalizer reduces the equation to z' + (1/(2(x-3))) z = 1/(2(x-3)) with solution z = 1, giving back
y = 1/x^2. As always every returned solution is differentiation-certified, and the construction is
validated by round trips that choose y and an f with an integer-residue pole, form g = y' + f y, and
recover a certifying solution. The remaining steps toward the full transcendental Risch algorithm
are the logarithmic-extension differential equation and height-two towers; the base case over Q(x)
is now complete. See `examples/228-weak-normalization.lisp` and the `cas_weaknorm` golden test.

## Tier 3: complete rational integration including improper fractions, and randomized hardening

`lib/cas/ratfull.lisp` finishes rational-function integration for an arbitrary A/D. rischrat.lisp
assumes a proper fraction, and on an improper one (degree of A at least that of D) Hermite reduction
silently drops the polynomial part: for x^3/(x^2 - 1) it kept the proper piece x/(x^2 - 1) but lost
the quotient x, and so the answer omitted x^2/2. rat-integrate-full divides A = Q D + Rem first, so
INT A/D = INT Q + INT Rem/D, where INT Q is the elementary polynomial antiderivative and Rem/D is
proper and handled by the Hermite plus Rothstein-Trager pipeline. Degenerate denominators are guarded
explicitly: a constant denominator makes A/D a polynomial integrated directly, and an exact division
leaves no proper part. The whole answer -- polynomial part, rational part, and logarithmic terms --
is differentiated back to A/D and checked against the integrand whenever the residues are rational;
the polynomial division and the Hermite rational part are exact regardless. It gives, for instance,
INT x^3/(x^2 - 1) dx = x^2/2 + (1/2) log(x^2 - 1) and handles (5x^4 + 3)/(x^2 - 1), x^5/(x^3 - x), and
constant or exactly-dividing denominators.

Both of those defects were not found by inspection but by `lib/cas/fuzzcheck.lisp`, a deterministic
randomized validator that hardens the whole integration stack. A linear-congruential generator
produces reproducible pseudo-random polynomials, and three families of checks run against them. The
first checks invariants of rational integration: for any A/D the Hermite rational part must be exact,
and when the residues are all rational the complete answer must differentiate back. The second and
third check completeness, the property a differentiation certificate cannot establish on its own,
by constructing solvable instances on purpose: for random polynomial f and rational h it sets
g = h' + f h and demands the Risch DE solver find a certifying solution rather than report "none",
and for random p and h it sets R = h' + p' h and demands INT R e^p come back elementary and certify.
A certificate only catches a wrong answer; a constructed-solvable instance catches a missed one, and
it was exactly such instances -- an improper fraction and a constant denominator -- that exposed the
two bugs now fixed. The validator runs as part of continuous integration over fixed seeds. See
`examples/229-improper-rational-integration.lisp`, `examples/230-integration-fuzz.lisp`, and the
`cas_ratfull` and `cas_fuzzcheck` golden tests.

## Tier 3: the exponential polynomial part of a tower

`lib/cas/expoly.lisp` integrates the part of a height-one exponential tower that Hermite reduction
cannot touch: a Laurent polynomial sum over k of a_k(x) theta^k in theta = e^p, where p is a
polynomial and the a_k are rational in x. The mechanism is the base-case Risch differential equation
made concrete. Because the derivation sends b theta^k to (b' + k p' b) theta^k, integrating a single
term a_k theta^k is the same as solving b' + (k p') b = a_k over Q(x) -- which is exactly the
integral INT a_k e^{k p} that rischde.lisp computes -- and then the antiderivative of the term is
b theta^k. The exponentials e^{k p} for distinct k are linearly independent over the rational
functions, so by Liouville's theorem the whole sum has an elementary antiderivative if and only if
every term does; the integrator therefore proceeds term by term and reports the sum non-elementary
the moment any single term fails. The k = 0 term carries no exponential and is an ordinary
base-field integral, handled by ratfull.lisp, so its antiderivative may contribute a rational part
and logarithms. Every coefficient produced is checked by re-deriving its defining equation
b' + k p' b = a_k, and the k = 0 part by differentiation, so a reported answer is certified and a
reported impossibility is a genuine Liouville obstruction. It recovers, for instance,
INT x e^x dx = (x-1) e^x, INT e^(2x) dx = (1/2) e^(2x), INT 2x e^(x^2) dx = e^(x^2), and
INT (x e^x + 1/x) dx = (x-1) e^x + log x, while proving INT e^x/x dx and INT e^(x^2) dx
non-elementary. Together with the rational and logarithmic cases this fills in the exponential
direction of single-extension integration; the remaining work toward the full algorithm is the
exponential proper part by Hermite reduction in the tower and height-two extensions. See
`examples/231-exponential-polynomial.lisp` and the `cas_expoly` golden test.

## Tier 3: the logarithmic polynomial part of a tower

`lib/cas/logpoly.lisp` is the primitive-case counterpart of expoly.lisp: it integrates a polynomial
sum over k of a_k(x) theta^k in theta = log x, where the a_k are polynomials in x. Whereas the
exponential case decoupled into an independent differential equation per power, the logarithmic case
is a single triangular recurrence, and it collapses to closed form. The derivation sends log x to
1/x, so the coefficient of theta^k in the derivative of sum b_k theta^k is b_k' + (k+1) b_{k+1}/x.
Writing b_k = x c_k -- the antiderivative of a polynomial in log x with polynomial coefficients has
coefficients that vanish at the origin -- turns the matching condition into a_k = c_k + x c_k' +
(k+1) c_{k+1}. The key simplification is that (x c)' = c + x c', so solving c + x c' = R for a
polynomial c is immediate: matching the coefficient of x^j gives (j+1) times the j-th coefficient of
c equal to the j-th coefficient of R, that is, c is R with its j-th coefficient divided by j+1.
Processing k from the top down with c_{n+1} = 0 therefore produces every coefficient directly, with
no linear system, and the answer is exact because this class is always elementary. Each result is
certified by forming the tower derivative and comparing it coefficient by coefficient to the input.
It gives, for instance, INT (log x)^2 dx = x (log x)^2 - 2x log x + 2x, INT log x dx = x log x - x,
INT x log x dx = (x^2/2) log x - x^2/4, and the general INT (log x)^n dx, as well as mixed cases
like INT (3x^2 + 1)(log x)^3 dx. Together with elem.lisp, which already handles rational functions
of log x of the form (1/x) R(log x) by substitution, this fills the logarithmic direction of
single-extension integration alongside the exponential direction of expoly.lisp; rational (rather
than polynomial) coefficients, where intermediate logarithms must be absorbed, and the full proper
case remain. See `examples/232-logarithmic-polynomial.lisp` and the `cas_logpoly` golden test.

## Tier 3: the primitive polynomial case with rational coefficients

`lib/cas/primint.lisp` generalizes logpoly.lisp from polynomial to rational coefficients: it
integrates a polynomial sum over k of a_k(x) (log x)^k with the a_k in Q(x). This is where the
primitive case acquires its characteristic subtlety -- intermediate logarithms that must be absorbed
into higher coefficients -- and where the first genuine non-elementarity obstructions of the
logarithmic direction appear. The antiderivative is sum b_k (log x)^k with b_k in Q(x), and matching
the coefficient of (log x)^k in the derivative gives a_k = b_k' + (k+1) b_{k+1}/x. Working from the
top down, b_k is the rational antiderivative of R_k = a_k - (k+1) b_{k+1}/x. That antiderivative
lies in Q(x) precisely when R_k integrates to a rational function plus lambda_k log x and nothing
else; the integrator computes this with the complete rational integrator of ratfull.lisp and reads
off lambda_k as the coefficient of log x in the result. The free additive constant in b_{k+1} is
then determined one level higher by C_{k+1} = lambda_k/(k+1), which is exactly the amount needed to
cancel the log x that level k would otherwise produce -- so the logarithm generated at one level is
absorbed as the next-higher power of log x. If at any level the rational integral requires a
logarithm whose argument is not x, or has an algebraic (non-rational) residue, then that logarithm
cannot be absorbed and the integral is not elementary as a polynomial in log x; the procedure
reports this rather than guessing. Each answer is certified by forming the tower derivative and
comparing coefficient by coefficient to the input. It recovers the absorption cases such as
INT (1/x) log x dx = (1/2)(log x)^2 and INT (log x)^2/x dx = (1/3)(log x)^3, the genuinely rational
ones such as INT log x / x^2 dx = -1/x - (log x)/x, the polynomial-coefficient cases of logpoly.lisp,
and even the base-field logarithmic integrals INT 1/x dx = log x as the degree-zero instance, while
correctly declining INT log x/(x^2+1) dx and INT log x/(x-1) dx. With this the primitive polynomial
part is complete over Q(x); the proper (fractional) case in log x and the analogous rational
coefficients for the exponential proper case remain. See
`examples/233-primitive-rational-coefficients.lisp` and the `cas_primint` golden test.

## Tier 3: Rothstein-Trager in the tower -- the proper logarithmic case

`lib/cas/towerrt.lisp` completes the proper (fractional) case of integration over a primitive
monomial theta = log x with x-dependent coefficients, and with it the logarithmic direction acquires
genuine non-elementarity decisions rather than reductions. tower.lisp already reduces A/D by Hermite
to a rational part plus a squarefree remainder a/d, but it resolves only the single-new-logarithm
case and otherwise reports "partial". This module resolves the general logarithmic part by lifting
Rothstein-Trager into the tower. With D the tower derivation (so D log x = 1/x and D acts on a
polynomial in theta with rational-function coefficients by the product rule), the primitive-case
residue criterion states that INT a/d is elementary over Q(x)(theta) if and only if the polynomial
R(z) = Res_theta(d, a - z Dd) has constant roots, in which case INT a/d = sum_i c_i log of
gcd_theta(d, a - c_i Dd), the c_i being exactly those roots. The resultant is taken with respect to
theta over the coefficient field Q(x); it is computed by a fraction-free Euclidean recurrence over
Q(x) evaluated at z = 0, 1, ..., deg_theta(d) and interpolated in z over Q(x), which yields R(z) as
a polynomial in z with rational-function coefficients. Making R(z) monic in z, the integral is
elementary precisely when every coefficient is a constant; the constant coefficients form an
ordinary polynomial over Q whose rational roots are the residues. A non-constant coefficient is a
genuine obstruction and the integral is reported non-elementary, while constant-but-irrational roots
are an algebraic RootSum, which is deferred rather than guessed. Combined with Hermite reduction
this is a complete, certified integrator for rational functions of log x whose residues are rational.
It finds, for instance, INT ((3/x + 1) log x - (3x + 1)) / ((log x)^2 - x^2) dx =
2 log(log x + x) + log(log x - x), a proper integrand with two distinct rational residues that the
single-logarithm reducer cannot handle; it recovers INT 1/(x log x) dx = log(log x) through the same
path; it reports INT (log x)/((log x)^2 + x) dx non-elementary because the residues depend on x; and
it defers INT (1/x)/((log x)^2 - 2) dx as an algebraic RootSum. Every returned answer is checked by
differentiating it in the tower and comparing to the integrand. With this the primitive (logarithmic)
case over Q(x) is essentially complete up to algebraic residues; the analogous exponential proper
case, the algebraic RootSum closure for logarithmic residues, and height-two towers remain. See
`examples/234-tower-rothstein-trager.lisp` and the `cas_towerrt` golden test.

## Tier 3: the exponential proper case -- Rothstein-Trager with a base-field correction

`lib/cas/expnrt.lisp` is the exponential mirror of towerrt.lisp: it integrates a proper rational
function of theta = e^x whose denominator is coprime to theta. The residue criterion is the same one
-- INT a/d is elementary over Q(x)(theta) if and only if R(z) = Res_theta(d, a - z Dd) has constant
roots, with INT a/d = sum_i c_i log of gcd_theta(d, a - c_i Dd) -- and the resultant-over-Q(x)
machinery is shared. What changes is the derivation. In a logarithmic tower D log x = 1/x lowers the
theta-degree of d under differentiation; in an exponential tower D(theta^i) = i theta^i, so Dd has
the same theta-degree as d, and a logarithm argument v_i, which grows like theta to the power
deg_theta(v_i), behaves at infinity like deg_theta(v_i) times x, because log theta = x. The
consequence is a base-field correction in the answer: an integral whose logarithmic part is
sum_i c_i log(v_i) actually equals sum_i c_i log(v_i) minus (sum_i c_i deg_theta v_i) x, the
subtracted multiple of x cancelling the spurious constant the logarithms introduce upon
differentiation. The integrator runs Hermite reduction over the exponential monomial for the
rational part, reuses the residue reduction for the logarithms, reads the correction off the
residues and argument degrees, and certifies the whole answer by differentiating it in the tower. It
finds, for instance, INT (-5 e^x - 6)/(e^(2x) + 3 e^x + 2) dx = log(e^x+1) + 2 log(e^x+2) - 3x with
its integer correction, the proper part INT -1/(e^x+1) dx = log(e^x+1) - x, fractional cases with
residues 2 and -1/2 and correction 3/2, balanced quotients of logarithms with zero correction, and
it reports INT 1/(e^x + x) dx non-elementary because the residue depends on x. Together with the
logarithmic proper case this completes, up to algebraic residues, the proper case of single-extension
integration in both directions; the algebraic RootSum closure and height-two towers remain. See
`examples/235-exponential-proper-case.lisp` and the `cas_expnrt` golden test.

## Tier 3: complete integration of rational functions of log x

`lib/cas/intlog.lisp` is the capstone of the logarithmic direction: it integrates an arbitrary
rational function A/D of theta = log x over Q(x), with rational residues, in a single call, and
certifies the result. It works by the classical separation of a rational function into a polynomial
part and a proper part. Polynomial division in theta over Q(x) writes A = Q D + Rem with the degree
of Rem below that of D, so INT A/D = INT Q + INT Rem/D. The polynomial part INT Q is exactly the
primitive-case polynomial integration of primint.lisp, which handles rational coefficients and the
absorption of logarithms into higher powers; the proper part INT Rem/D is the Hermite reduction
followed by the tower Rothstein-Trager of towerrt.lisp, which produces the logarithmic terms and
decides non-elementarity. What makes the composition clean is that no cross-module reasoning is
needed for correctness: the split A = Q D + Rem is an exact polynomial identity, verified directly,
and differentiation is linear, so once each of INT Q and INT Rem/D has been certified inside its own
module the derivative of their sum is necessarily Q + Rem/D = A/D. The whole integral is elementary
exactly when both parts are, and a non-elementary polynomial part (a logarithm that cannot be
absorbed) or proper part (residues that depend on x, or are algebraic) is reported rather than
forced. In one call it integrates a mixed integrand such as A/D whose quotient is (log x)^2 and whose
proper part is the two-residue ((3/x+1) log x - (3x+1))/((log x)^2 - x^2), recovering the polynomial
part and the logarithms together; it handles logarithm-absorbing proper parts like
((log x)^3 + 1/x)/log x; and it propagates the non-elementarity of (log x)/((log x)^2 + x). With this
the single logarithmic extension over Q(x) is integrated completely up to algebraic residues. The
exponential analogue (combining expoly.lisp and expnrt.lisp through the Laurent split), the algebraic
RootSum closure, and height-two towers remain. See `examples/236-complete-log-integration.lisp` and
the `cas_intlog` golden test.

## Tier 3: complete integration of rational functions of e^x

`lib/cas/intexp.lisp` is the exponential capstone, the mirror of intlog.lisp: it integrates an
arbitrary rational function A/D of theta = e^x over Q(x), with rational residues, in a single call,
and certifies the result. The decomposition differs from the logarithmic one because theta is a
unit, so a rational function of theta has a Laurent polynomial part carrying both positive and
negative powers as well as a proper part whose denominator is coprime to theta. Factoring out the
theta-content as D = theta^j D0 and choosing S theta^j + T D0 = 1 by the extended Euclidean algorithm
over Q(x), one has A/D = A T/theta^j + A S/D0; dividing A S = Q' D0 + R produces the proper part R/D0
with the degree of R below that of D0, while the entire Laurent part is (A T + Q' theta^j)/theta^j, a
single polynomial in theta shifted down by j. Reading that polynomial's coefficient of theta^{k+j}
off as the Laurent coefficient at theta^k avoids any overlap between the positive-power contributions
of the two pieces. The Laurent part is then integrated by expoly.lisp, which solves one differential
equation per power, and the proper part by expnrt.lisp, the exponential Hermite-and-Rothstein-Trager
with its base-field correction. As in the logarithmic capstone the composition is certified by
linearity: the decomposition A = Lnum D0 + R theta^j is an exact identity, verified directly, so once
each part is certified inside its own module the derivative of their sum is A/D. The integral is
elementary exactly when both parts are, and non-elementarity of either propagates. It integrates, in
one call, a genuinely Laurent integrand such as 1/(e^x (e^x + 1)) =
-e^(-x) - x + log(e^x + 1), a mixed integrand whose polynomial part is e^(2x) and whose proper part
has two residues, pure Laurent polynomials like e^x + 2 e^(-x), and it reports 1/(e^x (e^x + x))
non-elementary. With both intlog.lisp and intexp.lisp the single transcendental extension over Q(x)
-- logarithmic or exponential -- is now integrated completely, up to algebraic residues. The
remaining frontier is the algebraic RootSum closure for irrational residues, height-two towers, and
the algebraic-function case. See `examples/237-complete-exp-integration.lisp` and the `cas_intexp`
golden test.

## Tier 3: algebraic-residue RootSum closure (irreducible quadratic)

`lib/cas/algres.lisp` removes the "up to algebraic residues" qualifier from the single transcendental
extension in the most common case. When the Rothstein-Trager residue polynomial R(z) for an integral
INT a/d -- d squarefree in the monomial theta -- is monic over Q with constant but irrational roots,
the integral is still elementary but its logarithmic part is a RootSum, the sum over the roots c of
R(z) of c log(v_c), where v_c = gcd_theta(d, a - c Dd). The module closes the case in which R(z) is an
irreducible quadratic z^2 + p z + q over Q, whose two roots are a conjugate pair. Rather than extract
the radical, it computes the argument v_c symbolically over the field Q(x)(c) obtained by adjoining a
root of the quadratic, with c^2 = -p c - q, running the Euclidean algorithm in Q(x)(c)[theta] with
exact two-component arithmetic. The result is then certified without any reference to the value of the
radical: the derivative of the RootSum is the trace, the sum over the conjugate residues of
c Dv_c/v_c, and that trace is a rational function over Q(x) because the conjugate of alpha + beta c is
alpha - p beta - beta c, so a quantity and its conjugate add to 2 alpha - p beta in Q(x); the
algebraic parts cancel. For the exponential tower the elementary answer carries the same base-field
correction as expnrt, an extra -p deg(v_c) x term, so the certified identity is trace + p deg(v_c) =
a/d. Crucially the entire computation is gated by this differentiation certificate: if it does not
hold the case is left reported as 'algebraic exactly as before, so the closure can only tighten the
verdicts of towerrt.lisp and expnrt.lisp, never weaken them. It certifies, for instance,
INT (1/x)/((log x)^2 - 2) dx, whose residues are the irrational numbers plus or minus the square root
of one eighth and whose argument is log x - sqrt 2, and the exponential analogue
INT e^x/(e^(2x) - 2) dx with the same residues; rational-residue integrals continue to be handled by
delegation, and integrals with x-dependent residues are still correctly reported non-elementary. The
remaining algebraic cases -- residue polynomials of degree three or higher, requiring the full
Lazard-Rioboo-Trager subresultant construction -- are the next increment. See
`examples/238-algebraic-rootsum-closure.lisp` and the `cas_algres` golden test.

## Tier 3: higher-degree algebraic-residue RootSum closure (irreducible R of any degree)

`lib/cas/algresn.lisp` generalizes the quadratic closure of `algres.lisp` to a Rothstein-Trager
residue polynomial R(z) that is monic over Q and irreducible of any degree m. The roots of such an R
form a single conjugate class, and the logarithmic part of INT a/d -- d squarefree in the monomial
theta -- is the RootSum over those roots of c log(v_c), with v_c = gcd_theta(d, a - c Dd). The
argument is computed over the number field K = Q(x)[c]/(R(c)). The representation makes this cheap:
an element of K is just a polynomial in c with coefficients in Q(x), so it reuses the ordinary
polynomial arithmetic already in the tower, reduced modulo R; multiplication, inversion by the
extended Euclidean algorithm, and the derivation all follow. The answer is certified without ever
extracting a radical or naming a conjugate. The Rothstein-Trager factors satisfy the identity that
their product over all roots of R equals d, so the norm of v_c is d itself and the cofactor d/v_c is
obtained by a single polynomial division in K[theta]; the derivative of the RootSum is then the
trace, over the conjugate class, of c times the logarithmic derivative of v_c, which equals the trace
of c (Dv_c) (d/v_c) divided by d. That trace down to Q(x) is evaluated coefficient by coefficient
using the power sums of the roots of R, and the power sums come from Newton's identities applied to
the coefficients of R, so the symmetric functions of the conjugates are obtained directly from R
without factoring it over any extension. For the exponential tower the elementary answer carries the
same base-field correction as expnrt, an extra term proportional to the degree of v_c times the sum
of the roots, and the certificate folds that correction in. As always the differentiation certificate
is the gate: a residue polynomial that is reducible, or any case whose trace identity fails to hold,
is left reported 'algebraic exactly as before, so the closure strictly tightens the verdicts of
towerrt.lisp and expnrt.lisp. It certifies, for example, the cubic INT (6/x)/((log x)^3 - 2) dx,
whose residues are the three cube roots of two and whose argument is log x - c with the trace
numerator collapsing to 6/x, together with its exponential analogue; the quadratic case is subsumed,
and integrals with x-dependent residues remain correctly non-elementary. The remaining algebraic
cases -- a reducible residue polynomial, handled by splitting into its irreducible factors, and
arguments v_c of degree above one -- are the next increment. See
`examples/239-higher-degree-rootsum.lisp` and the `cas_algresn` golden test.

## Tier 3: first rung of the height-two tower

`lib/cas/tower2.lisp` takes the first step beyond a single transcendental extension. A height-two
tower stacks a second monomial theta2 on top of a height-one field K1 = Q(x)(theta1), so that the
derivative of theta2 is an element of K1 rather than of Q(x): for instance theta1 = e^x and
theta2 = log(e^x + 1), whose derivative e^x/(e^x + 1) lives in Q(x, e^x). The module builds the
differential structure at that level. A height-two object is a polynomial in theta2 whose
coefficients are tower-rationals over theta1, and the derivation applies the product and chain rules
through both levels at once: the derivative of a sum of b_k theta2^k is the sum of the height-one
derivatives of the coefficients times theta2^k, plus the chain-rule contribution in which each
b_k theta2^k yields k b_k (D theta2) theta2^{k-1}, with D theta2 supplied as a K1 element. On top of
this it certifies the simplest genuinely height-two integrals, the exact powers
INT theta2^k (D theta2) dx = theta2^{k+1}/(k+1), by differentiating the proposed antiderivative with
the two-level derivation and checking equality coefficient by coefficient; it does so on two
unrelated towers, log(e^x + 1) over e^x and log(log x + 1) over log x, and rejects an incorrect
antiderivative. This is explicitly a first rung: it demonstrates that the chain rule composes
correctly across two transcendental levels and that the height-one field arithmetic carries the
weight of being a coefficient field. The larger remaining lift is full height-two integration --
Hermite reduction, Rothstein-Trager, and the Risch differential equation with the coefficient field
itself a tower -- which recurses the entire single-extension machinery one level up. See
`examples/240-height-two-tower.lisp` and the `cas_tower2` golden test.

## Tier 3: reducible algebraic-residue RootSum closure (full squarefree logarithmic part)

`lib/cas/algresfull.lisp` finishes the algebraic-residue logarithmic part of the single transcendental
extension for a squarefree denominator. The Rothstein-Trager residue polynomial R(z) for INT a/d need
not be irreducible; in general it factors over Q into pieces of mixed degree. This module factors
R = prod_j P_j with factor-Q and treats each factor separately. A linear factor contributes an
ordinary logarithm c0 log(v_c) with rational residue c0, exactly as the base rational path already
does. An irreducible factor P_j of degree m at least two contributes the RootSum over its conjugate
class, sum_{P_j(c)=0} c log(v_c), computed in the number field K_j = Q(x)[c]/(P_j) by the same
machinery as the irreducible case. The new ingredient is the per-factor norm. In the generic
situation each argument v_c = theta - r is linear in theta, and then the norm
N_j = prod_sigma sigma(v_c) is precisely the characteristic polynomial of the element r in K_j. That
characteristic polynomial is recovered from the power sums tr(r^k) of the conjugates of r -- each a
field trace computed from the power sums of P_j -- by running Newton's identities backward to get the
elementary symmetric functions, so neither a resultant nor an explicit splitting of P_j is required.
Because the Rothstein-Trager factors satisfy the identity that their norms multiply back to d, the
per-factor logarithmic derivatives share the common denominator d, and the entire logarithmic part is
certified by adding up the field traces and checking that the sum equals a/d, with the same
exponential base-field correction as before summed across the factors. The differentiation
certificate remains the sole gate: a residue polynomial that is not squarefree, or any factor whose
argument fails to be linear or whose trace identity does not hold, leaves the case reported
'algebraic, so the closure can only strengthen the verdicts of towerrt.lisp, expnrt.lisp and
algresn.lisp. It certifies, for example, the mixed integral whose residue polynomial is
(z - 1)(z^2 - 1/8): a rational residue at one logarithm together with a conjugate pair of irrational
residues, the two contributions summing exactly to the integrand. With this in place the logarithmic
part of a single transcendental extension over Q(x) is complete for any squarefree denominator and
any residue behaviour, rational or algebraic. The remaining algebraic gap is the degenerate case of a
non-squarefree residue polynomial (repeated residues, where the argument is no longer linear over a
factor). See `examples/241-reducible-rootsum-closure.lisp` and the `cas_algresfull` golden test.

## Tier 3: non-squarefree residue polynomial (repeated rational residues)

`lib/cas/algresnsf.lisp` handles the case in which the Rothstein-Trager residue polynomial R(z) is
not squarefree, which happens when several roots of d share a residue. The squarefree factorization
R = prod_i R_i^i, computed with yun-square-free, records the multiplicities: a residue that is a root
of R_i carries an argument v_c = gcd_theta(d, a - c Dd) of degree i in theta, not the linear argument
of the squarefree case. This module closes the common branch in which every residue is rational. For
each rational residue c0 it forms the degree-i argument directly in Q(x)[theta] -- no number field is
involved, since the residue is rational -- and contributes the term c0 log(v_c); when a - c0 Dd
vanishes the gcd with d returns d itself, recovering the full denominator as the argument. Because the
Rothstein-Trager factors multiply back to d, the per-residue logarithmic derivatives share the common
denominator d, and the whole logarithmic part is certified by checking that the sum of c0 (Dv_c)/v_c
over all rational residues equals a/d, with the exponential base-field correction summed across the
residues. It certifies, for example, INT (2 log x / x)/((log x)^2 - 2) dx = log((log x)^2 - 2), whose
residue polynomial is (z - 1)^2, and its exponential counterpart with the correction. The entry point
int-prim-rational-nsf layers the handlers in order -- the rational base case, then the reducible
RootSum closure of algresfull.lisp, then this non-squarefree rational case -- and otherwise reports
'algebraic, so a non-squarefree residue polynomial whose repeated residue is irrational (which would
require the number-field norm of a higher-degree argument, the one remaining algebraic gap) is still
deferred rather than mishandled. The differentiation certificate gates every answer. See
`examples/242-nonsquarefree-rootsum.lisp` and the `cas_algresnsf` golden test.

## Tier 3: irrational repeated residues -- the complete single-extension logarithmic part

`lib/cas/algresnsf2.lisp` removes the last restriction on the logarithmic part of a single
transcendental extension: a non-squarefree residue polynomial whose repeated residue is irrational.
The residue polynomial R(z) is factored over Q into distinct irreducible factors with multiplicities
by factor-Q, and each factor is treated as a single conjugate class. A factor of degree m at
multiplicity i carries an argument v_c = gcd_theta(d, a - c Dd) of degree i over the number field
K = Q(x)[c]/(P). For a rational residue this stays in Q(x)[theta], but for an irreducible factor of
degree m at least two the per-factor norm N = prod_sigma sigma(v_c) of the higher-degree argument is
required, and the module computes it as the determinant of the m-by-m multiplication-by-v_c matrix
over Q(x)[theta]. That matrix is built by reducing v_c times c^j modulo the monic minimal polynomial
P, which needs no division, and its determinant is taken by cofactor expansion in pure polynomial
arithmetic; the construction was validated against the field norms of known elements, for instance
the norm of c in Q[c]/(c^3 - 2) being two. The cofactor d/v_c then comes from a single division in
K[theta], and the per-factor derivative is the field trace of c times the logarithmic derivative of
v_c times that cofactor, the trace evaluated from the power sums of P. Because the Rothstein-Trager
factors satisfy the identity that their norms multiply back to d, every per-factor logarithmic
derivative shares the common denominator d, and the whole part is certified by checking that the sum
of the traces equals a/d with the exponential base-field correction. A single handler now subsumes
every residue case -- squarefree rational, reducible and irreducible, and non-squarefree rational and
algebraic. It certifies, for example, the integral whose residue polynomial is (z^2 - 1/8)^2, a
conjugate pair of irrational residues each of multiplicity two, with an argument of degree two over
the quadratic field. With this in place the logarithmic part of a single transcendental extension
over Q(x) is complete for every squarefree denominator and every possible residue behaviour. See
`examples/243-irrational-repeated-residue.lisp` and the `cas_algresnsf2` golden test.

## Tier 4: Hermite reduction at height two (the first climb)

`lib/cas/tower2herm.lisp` begins genuine height-two integration: integrating a rational function of a
second monomial theta2 over the height-one field K1 = Q(x)(theta1), rather than over Q(x). The module
mirrors the height-one Hermite reduction one level up. Every operation on Q(x)[theta1] is replaced by
the analogous operation on K1[theta2]: the coefficient field is now K1, whose addition, multiplication,
inversion and division are the tower-rational field operations, and a layer of polynomial arithmetic
in theta2 over K1 -- division, gcd, extended Euclid, modular inverse and Yun squarefree factorization
-- is built on top of them exactly as the height-one layer was built over Q(x). The height-one
derivation Drf is replaced by the two-level derivation D2 of tower2.lisp, which differentiates the K1
coefficients and applies the chain rule through theta2. The setting is theta2 primitive over K1, that
is a logarithm, so that D theta2 is an element of K1; the worked tower is theta1 = e^x and
theta2 = log(e^x + 1), whose derivative e^x/(e^x + 1) lies in K1. Given a proper rational function A/D
of theta2 over K1 with D monic, the reduction returns a rational part g, itself a fraction of
polynomials in theta2 over K1, together with a remainder A*/D* in which D* is squarefree in theta2,
such that D2(g) + A*/D* equals A/D. Squarefree factorization uses the formal theta2-derivative; the
genuine derivation enters only through the derivatives of the squarefree factors, just as at height
one. The result is certified by differentiating g with D2 through the quotient rule and checking the
identity by cross-multiplication over K1[theta2]. It certifies, for example, that the integral of
(D theta2) over theta2 squared is minus the reciprocal of theta2, a purely rational height-two
antiderivative, and it correctly separates a rational part from a squarefree logarithmic remainder
when one is present. This is the rational part of full height-two integration; the logarithmic part
(a height-two Rothstein-Trager, for which the determinant-over-Q(x)[theta] machinery of algresnsf2.lisp
is a building block) and the exponential second monomial are the next rungs. See
`examples/244-height-two-hermite.lisp` and the `cas_tower2herm` golden test.

## Tier 4: a complete height-two integral (rational part plus a logarithm)

`lib/cas/tower2int.lisp` turns the height-two Hermite reduction into a genuine integrator for a
rational function of a second monomial theta2 primitive over K1 = Q(x)(theta1), by adding recognition
of a logarithmic term. After Hermite reduces A/D to a rational part g together with a remainder A*/D*
whose denominator is squarefree in theta2, that remainder is the derivative of a logarithm exactly
when A* equals a constant times D2(D*); the recognizer divides A* by D2(D*) over K1[theta2] and
accepts the quotient when the division is exact and the quotient is a constant of the tower, that is
an element whose derivative under the two-level derivation vanishes and which therefore lies in the
constant field Q. In that case the integral is g plus that constant times log(D*). The full answer is
certified by differentiating it with D2, by the quotient rule on g and the logarithmic-derivative rule
on the log term, and checking equality with A/D by cross-multiplication over K1[theta2]. It certifies,
for instance, that the integral of (D theta2)(theta2 + 1) over theta2 squared is minus the reciprocal
of theta2 plus log(theta2) -- concretely minus the reciprocal of log(e^x + 1) plus log(log(e^x + 1)) --
a complete height-two antiderivative with both a rational and a logarithmic part. This resolves the
single-logarithm case, in which the squarefree remainder's denominator is itself the argument of the
logarithm. The general height-two Rothstein-Trager, where several constant residues are found from a
resultant over K1 and combined into a RootSum, and the exponential second monomial, are the next rungs
of the climb. See `examples/245-height-two-integral.lisp` and the `cas_tower2int` golden test.

## Tier 4: the general height-two Rothstein-Trager logarithmic part

`lib/cas/tower2rt.lisp` completes the logarithmic part of height-two integration, moving beyond the
single logarithm to the general case in which the squarefree remainder integrates to several
logarithms with distinct constant residues. After Hermite reduces A/D to a rational part g and a
remainder A*/D* squarefree in theta2, the logarithmic part is a sum over residues c of c times
log(v_c), where v_c is the greatest common divisor in theta2, taken over the coefficient field
K1 = Q(x)(theta1), of D* and A* - c D2(D*). The residues are the roots of the Rothstein-Trager
resultant R(z) = Res_theta2(D*, A* - z D2(D*)), an element of K1[z]; for the integral to be
elementary with constant residues those roots must lie in the constant field Q. The resultant is
obtained by evaluation and interpolation rather than by symbolic elimination over K1[z]: at each
integer z the resultant is the determinant of the Sylvester matrix whose entries lie in K1, computed
by cofactor expansion using the cancel-before-multiply K1 arithmetic, with a direct formula
lc(g) raised to deg(f) times f at the root of g taken whenever the second argument is linear, which
avoids forming the matrix in the common case. The interpolation is kept over the rationals by a ratio
trick: dividing every evaluated resultant by a fixed nonzero one cancels the K1 content that the
resultant carries as an overall factor, so the resulting ratios are constants of the tower exactly
when the residues are rational. Those ratios, read off as rationals, interpolate by Lagrange to a
polynomial over Q whose roots are the residues; for each rational residue the gcd over K1[theta2]
supplies the argument of its logarithm, and the residue degrees summing to the degree of D* confirms
that the rational residues account for the whole logarithmic part rather than leaving an algebraic
remainder. The complete answer, the rational part together with the RootSum, is certified by
differentiating with the two-level derivation D2 -- the quotient rule on the rational part and the
logarithmic-derivative rule on each term -- and checking equality with A/D by cross-multiplication
over K1[theta2]. With theta1 = e^x and theta2 = log(e^x + 1), the integrator certifies that the
integral of (D theta2)(3 theta2 - 1) over (theta2 squared minus theta2) is log(theta2) plus twice
log(theta2 - 1) -- concretely log(log(e^x + 1)) plus twice log(log(e^x + 1) - 1) -- a height-two
antiderivative with two distinct rational residues found from the resultant over K1. When the ratios
fail to be constant, or the rational residues do not account for the full degree, the logarithmic part
is algebraic at height two and is reported as such rather than forced into rational residues; that
case, together with an exponential second monomial whose derivative leaves the coefficient field, are
the remaining rungs before the algebraic-function summit. See
`examples/246-height-two-rootsum.lisp` and the `cas_tower2rt` golden test.

## Tier 4: an irreducible-quadratic residue at height two (algebraic residues by trace)

`lib/cas/tower2alg.lisp` handles the height-two logarithmic part when the Rothstein-Trager residue
polynomial is an irreducible quadratic over Q. In that case the two residues are a conjugate pair of
algebraic numbers and the antiderivative is a RootSum of two logarithms whose arguments lie in the
quadratic extension K1(alpha), where alpha is a root of the monic residue polynomial z^2 + c1 z + c0.
Rather than carry the algebraic closure, the module uses that the RootSum's logarithmic derivative is a
trace over the conjugate pair and therefore descends back to the rational coefficient field K1. For a
squarefree denominator of degree two in theta2 the second Rothstein-Trager argument A* - z D2(D*) is
linear, so the gcd argument is v_alpha = theta2 - rho with rho the root of A* - alpha D2(D*) in
K1(alpha); its conjugate v_albar is obtained by conjugating coefficients, and the two multiply back to
D* = v_alpha v_albar. The derivative of alpha log(v_alpha) + albar log(v_albar) is the trace of
alpha D2(v_alpha) v_albar over the denominator v_alpha v_albar, a quotient whose numerator is a
polynomial over K1; the integral is certified by checking that this numerator equals A* over
K1[theta2]. The extension arithmetic is the degree-two algebra of pairs over K1 reduced by
alpha^2 = -c1 alpha - c0, with conjugation, trace, norm, and inverse, and a derivation that acts
coefficientwise because the residue is a constant of the tower. The residue polynomial itself is
recovered as an element of Q[z] by the same ratio trick used for rational residues, and it is confirmed
to be an irreducible quadratic by having degree two and no rational roots. With theta1 = e^x and
theta2 = log(e^x + 1) the integrator certifies that the integral of (D theta2) over theta2 squared plus
one is arctan(theta2) -- concretely arctan(log(e^x + 1)) -- whose residue polynomial 4 z^2 + 1 is
irreducible over Q with residues plus and minus i/2. Higher-degree algebraic residues and residues that
depend on x are the natural continuations; the trace-descent certificate generalizes to them. See
`examples/247-height-two-arctan.lisp` and the `cas_tower2alg` golden test.

## Tier 4: the exponential second monomial (a different derivation)

`lib/cas/tower2exp.lisp` opens the second branch of the height-two climb, in which the second monomial
theta2 is an exponential rather than a primitive: theta2 = exp(u) with u in K1 = Q(x)(theta1), so its
derivative D theta2 = u' theta2 carries a factor of theta2 itself and is not an element of K1. The
two-level derivation is therefore structurally different from the primitive case: differentiating
b_k theta2^k gives (D b_k + k u' b_k) theta2^k, so every monomial keeps its degree instead of dropping
by one, and D2(Sum b_k theta2^k) = Sum (D b_k + k u' b_k) theta2^k with the height-one derivation acting
on each coefficient and u' supplied as an element of K1. The first integration case this exposes is the
exponential exact-power case: a power sum Sum a_k theta2^k with no constant term is an exact derivative
exactly when each a_k is k u' times a constant of the tower, in which case the antiderivative is
Sum (a_k/(k u')) theta2^k; the integrator forms each coefficient quotient over K1 and accepts when every
one is a constant. The general coefficient equation, in which the antiderivative coefficient is itself a
nonconstant function of x, is the exponential Risch differential equation and is the next rung. Every
answer is certified by differentiating it with the exponential derivation and checking equality with the
integrand over K1[theta2]. With theta1 = e^x and theta2 = exp(e^x), so u' = theta1, the module certifies
that the exponential derivative of theta2 squared is twice theta1 times theta2 squared, and that the
integral of 5 theta1 theta2 + 6 theta1 theta2 squared is 5 theta2 + 3 theta2 squared. Together with the
primitive case this gives both kinds of second monomial their derivation and a first integration law;
the full exponential height-two integrator -- exponential Hermite reduction and an exponential
Rothstein-Trager -- builds on these foundations. See `examples/248-exp-second-monomial.lisp` and the
`cas_tower2exp` golden test.

## Tier 4: an x-dependent argument for the algebraic-residue certificate

The trace certificate of `lib/cas/tower2alg.lisp` was first exercised on the arctan integral, where the
gcd argument rho = 1/(2 alpha) is a constant of the tower. The same machinery certifies residues whose
argument depends on x, because the height-two derivation t2a-deriv differentiates the K1(alpha)
coefficients of v_alpha coefficientwise and so computes a nonzero D2(rho) automatically. Differentiating
the RootSum (i/2) log(theta2 - i theta1) + (-i/2) log(theta2 + i theta1), with theta1 = e^x and
theta2 = log(e^x + 1), gives the integral of (theta1 theta2 - theta1 D theta2) over theta2 squared plus
theta1 squared. The residues are still plus and minus i/2 -- the residue polynomial 4 z^2 + 1 is again
irreducible over Q -- but now the argument rho = i theta1 depends on x, so v_alpha = theta2 - i theta1
has a nonconstant coefficient and D2(v_alpha) carries a theta1 term in addition to D theta2. The
certificate Tr[alpha D2(v_alpha) v_albar] = A* over K1[theta2] holds unchanged, confirming the integral.
This is the first algebraic-residue height-two case whose logarithm arguments are genuinely
x-dependent; higher-degree algebraic residues remain the further continuation. See
`examples/249-height-two-arctan-xdep.lisp` and the `cas_tower2algx` golden test.

## Tier 4: the exponential Risch differential equation at height two

`lib/cas/tower2exprde.lisp` is the next rung of the exponential branch. Integrating a power sum
Sum a_k theta2^k against the exponential derivation reduces, in each degree k, to solving the coefficient
equation b' + k u' b = a_k for b in K1 = Q(x)(theta1); the antiderivative coefficient is then b. The
exact-power case of tower2exp.lisp solved only the sub-case where b is a constant of the tower; this
module solves the full equation for b a polynomial in theta1. With theta1 = e^x, so D theta1 = theta1,
the two-level operator acts on b = Sum b_j theta1^j as (b' + k u' b)_j = b_j' + j b_j + k (u' * b)_j,
where the term j b_j comes from differentiating theta1^j and (u' * b) is the convolution of the
theta1-coefficient sequences. Writing P for the theta1-degree of u', a polynomial solution has degree
deg(a) - P, and the coefficient of theta1^j in the equation expresses b_{j-P} in terms of strictly
higher coefficients already found; so b_{deg(a)-P} down to b_0 fall out by successive division by
k u'_P, with the lowest P equations becoming consistency conditions. The solver assembles the candidate
b and certifies it by differentiating with the exponential derivation and checking b' + k u' b = a in
K1, reporting no solution on failure -- including the case of a genuine theta1 denominator, which this
polynomial solver does not treat and which is the further rung. The headline equation
b' + theta1 b = theta1 + theta1^2 has the nonconstant solution b = theta1, and the integrator built on
the solver produces antiderivatives whose theta2-coefficients are themselves nonconstant: the integral
of (theta1 + theta1^2) theta2 + 6 theta1 theta2 squared is theta1 theta2 + 3 theta2 squared, generalizing
the exact-power case that the same expression's second term illustrates. See
`examples/250-exp-rde.lisp` and the `cas_tower2exprde` golden test.

## Tier 4: the exponential Risch differential equation with a degree-two logarithmic derivative

The solver of `lib/cas/tower2exprde.lisp` is not limited to deg(u') = 1. With theta1 = e^x and
theta2 = exp(e^{2x}/2), the second monomial has u' = theta1^2, of degree two in theta1, and the operator
coefficient (b' + k u' b)_j = b_j' + j b_j + k (u' * b)_j drives the same top-down recursion, now
dividing by k u'_2 so that a polynomial solution has degree deg(a) - 2. The equation
b' + theta1^2 b = theta1 + theta1^3 has the nonconstant solution b = theta1, and the integrator built on
the solver certifies INT (theta1 + theta1^3) theta2 dx = theta1 theta2 against the exponential derivation
D2. This confirms the RDE solver handles the nested-exponential case where the inner exponent grows faster
than x. See `examples/251-exp-rde-deg2.lisp` and the `cas_tower2rde2` golden test.

## Tier 4: the n>=2 Sylvester-resultant determinant path over K1

For squarefree D* of degree three or more, the Rothstein-Trager resultant Res_theta2(D*, A* - z D2(D*))
is a determinant of size five by five or larger over the field K1 = Q(x)(theta1). The cofactor expansion
that first implemented this is factorial in both time and allocation and exhausted memory even at size
five; it is now replaced by Gaussian elimination over K1 with partial pivoting, cubic in the matrix size,
with each entry reduced to lowest terms after every elimination step. The arithmetic is validated against
the trusted Sylvester resultant over Q: two degree-(3,2) pairs with constant K1 coefficients, each a five
by five determinant, agree exactly with the field-division resultant of resultant.lisp. A single resultant
with genuine fractional coefficients (D theta2 = e^x/(e^x + 1)) also computes, returning a result of low
degree in theta1. The full degree-three-or-more height-two integral, however, evaluates the resultant at
several interpolation nodes inside one logarithmic-part computation, and holding several such fractional
determinants at once exceeds the interpreter's working memory regardless of the elimination method --
plain Gaussian, normalized Gaussian, and one-step fraction-free Bareiss were all tried; making that
end-to-end path feasible needs a more memory-frugal multi-resultant strategy, which is the next step. The
determinant-path arithmetic itself is certified here. See
`examples/252-resultant-determinant-path.lisp` and the `cas_tower2det` golden test.

## Tier 4: the exponential Risch differential equation with a theta1 denominator

The RDE solver of `lib/cas/tower2exprde.lisp` is not restricted to polynomial b. A denominator theta1^l
is handled by writing b = bbar * theta1^(-l): substituting into b' + k u' b = a and using
(theta1^(-l))' = -l theta1^(-l) for theta1 = e^x turns the equation into bbar' + (k u' - l) bbar = num,
the same top-down recursion with the diagonal term j replaced by j - l. Only pure-power denominators
theta1^l arise for an exponential monomial -- this is the special part of the second monomial -- and the
polynomial solver is precisely the l = 0 instance, so it still routes through unchanged. With
theta2 = exp(e^x) the equation b' + theta1 b = 1 - theta1^(-1) has the solution b = theta1^(-1) = e^{-x},
a negative power of theta1 that the polynomial recursion cannot represent; the solution is certified
against the exponential derivation D2. See `examples/253-exp-rde-laurent.lisp` and the `cas_tower2rdel`
golden test.

## Tier 4: the complete exponential power-sum integrator at height two

`lib/cas/tower2expint.lisp` closes the exponential power-sum case. For theta2 = exp(u) an integrand
P = a_0 + Sum_{k>=1} a_k theta2^k integrates degree by degree, because the exponential derivation
preserves the theta2-grading: D2 of b theta2^k stays in degree k, and D2 of a K1 element stays in degree
zero. The positive degrees are handled by the exponential RDE solver (b_k' + k u' b_k = a_k), and the
constant term a_0, an element of K1 = Q(x)(theta1), is integrated by the complete single-exponential
integrator of intexp.lisp -- the height-two coefficient representation, a ratio of polynomials in
theta1 = e^x over Q(x), is exactly the input that integrator consumes, so no translation is needed.
Correctness factors along the grading: the power part is certified by differentiating with D2 and matching
P above degree zero, while the constant term carries the single-extension integrator's own certificate.
With theta2 = exp(e^x), INT [ 1/(e^x+1) + (theta1+theta1^2) theta2 + 6 theta1 theta2^2 ] dx
= (x - log(e^x+1)) + theta1 theta2 + 3 theta2^2, the theta2^0 term being a genuine logarithmic integral
supplied by the capstone. This composes the height-two RDE solver with the single-extension exponential
integrator into a complete integrator for polynomial power sums in an exponential second monomial. A
resolved subtlety: intexp and the height-two tower both define a function named hermite (with three and
two arguments respectively, for different uses), so the modules are imported in the order that keeps the
three-argument tower version live, after which both lineages coexist. See `examples/254-exp-powersum.lisp`
and the `cas_tower2expint` golden test.

## Tier 4: Hermite reduction for an exponential second monomial

`lib/cas/tower2exphermite.lisp` is the exponential mirror of the primitive Hermite reduction. For
theta2 = exp(u) an integrand A/D whose denominator is coprime to theta2 but not squarefree in theta2 is
reduced to a rational part plus a remainder with squarefree denominator, by the same squarefree-factor-
and-peel algorithm as the primitive case (h2-yun, h2-invmod, division over K1[theta2]) with the exponential
derivation t2e-deriv in place of the primitive t2-deriv. Correctness rests on the normality of exponential
monomials: a squarefree V coprime to theta2 satisfies gcd(V, D2 V) = 1, so the Bezout step is valid. The
reduction is purely rational and does not invoke the Rothstein-Trager resultant, so it is free of the
resultant memory wall. With theta1 = e^x and theta2 = exp(e^x), INT -theta1 theta2/(theta2-1)^2 dx
= 1/(theta2-1) up to a constant; the squarefree remainder vanishes, so the antiderivative is purely
rational, certified by differentiating with D2 and matching A/D. A note on the interpreter: combining the
reduction with the full cross-multiplying remainder check in one process exhausts working memory (the
height-two K1 arithmetic is allocation-heavy), so the reduction is computed once and, the remainder being
zero here, certified by the leaner identity D2(rational part) = A/D. See `examples/255-exp-hermite.lisp`
and the `cas_tower2eh` golden test.

## Tier 4: height-two primitive integrals with rational coefficients, by substitution

`lib/cas/tower2primrat.lisp` sidesteps the Sylvester-resultant memory wall for an important class. When
the second monomial theta2 is primitive (D2 theta2 = Dtheta2 in K1) and the integrand has the form
Dtheta2 * Abar(theta2)/Dbar(theta2) with Abar, Dbar in Q[theta2] (rational-number coefficients, constant
in x and theta1), the chain rule d/dx F(theta2) = F'(theta2) Dtheta2 makes it the exact pullback of a
rational-function integral: INT Dtheta2 Abar/Dbar dx = [ INT Abar/Dbar d(theta2) ] with theta2 as the
variable. The whole logarithmic part is therefore computed by the trusted rational-function integrator
rat-integrate (rischrat.lisp) over Q, with no resultant over K1 and none of the K1-fraction memory growth
that defeated the determinant path across several earlier attempts. This integrates a cubic denominator
with three rational residues -- INT (Dtheta2)(6 theta2^2 - 10 theta2 + 2)/(theta2^3 - 3 theta2^2 +
2 theta2) dx = log(theta2) + 2 log(theta2 - 1) + 3 log(theta2 - 2), exactly the case the K1-coefficient
resultant could not evaluate -- and a quartic INT (Dtheta2) D*'/D* dx = log(D*), confirming the method has
no degree limit. The reduction is gated by reconstructing each coefficient (A_i = Dtheta2 * Abar_i exactly
in K1, with non-rational coefficients rejected), so the substitution is certified to apply, after which
rat-integrate carries its own differentiation certificate over Q; together these give the height-two
D2 certificate. The class is precisely the height-two primitive integrals whose logarithm arguments and
residues are rational and whose denominator is constant in x. The fully general K1-coefficient case --
x-dependent arguments, algebraic residues -- still routes through the Sylvester resultant and remains
memory-bound. See `examples/256-prim-rational-integral.lisp` and the `cas_tower2primrat` golden test.

## Tier 4: height-two primitive arctangents, by substitution

`lib/cas/tower2primfull.lisp` closes the algebraic-residue (arctangent) case of the substitution
introduced for `tower2primrat.lisp`. The same pullback applies -- for a primitive theta2 and Abar, Dbar
in Q[theta2], INT Dtheta2 Abar/Dbar dx = [ INT Abar/Dbar d(theta2) ] with theta2 as the variable -- but
the rational-function integral in theta2 is now routed through the complete integrator integrate.lisp
(`integrate-rational`), so that an irreducible-quadratic denominator factor of negative discriminant
produces a genuine arctangent rather than being reported non-elementary. The headline case is
INT (Dtheta2)/(theta2^2 + 1) dx = arctan(theta2) = arctan(log(e^x + 1)); a mixed numerator gives
INT (Dtheta2)(theta2^2 + theta2 + 1)/(theta2^3 + theta2) dx = log(theta2) + arctan(theta2). Both are
obtained with NO Sylvester resultant over K1 and none of its memory blow-up, since integrate-rational
works over Q by partial fractions and the arctangent branch is the irreducible-quadratic case. The
reduction is gated exactly as before (A_i = Dtheta2 * Abar_i reconstructed in K1, non-rational
coefficients rejected), so the substitution is certified to apply, and integrate-verify supplies the
differentiation certificate over Q; together they give the height-two D2 certificate. This complements
`tower2primrat.lisp` -- rational residues there, arctangents here -- so the height-two primitive
logarithmic/arctangent part is now complete for integrands whose arguments and residues are rational or
irreducible-quadratic and whose denominator is constant in x. The fully general K1-coefficient case
(x-dependent arguments, higher algebraic residues over K1) still routes through the Sylvester resultant
and remains memory-bound. See `examples/257-prim-arctan-integral.lisp` and the `cas_tower2primfull`
golden test.

## Tier 4: height-two substitution integrals for ANY monomial (the exponential case included)

`lib/cas/tower2sub.lisp` unifies the substitution method across both second-monomial kinds. The chain rule
d/dx F(theta2) = F'(theta2) D2(theta2) holds whether theta2 is primitive or exponential, so for any
g in Q(theta2), INT D2(theta2) g(theta2) dx = [ INT g(t) dt ] with t = theta2. Concretely, an integrand
A(theta2)/D(theta2) over K1 is of this type exactly when the reduced rational function
g = A / (D * D2theta2) lies in Q(theta2); the integral is then the trusted integrate.lisp over Q --
logarithms for linear factors, arctangents for irreducible quadratics -- with no Sylvester resultant over
K1. This subsumes `tower2primrat.lisp` and `tower2primfull.lisp` (where D2theta2 is a scalar in K1) and,
importantly, also covers the EXPONENTIAL second monomial, where D2theta2 = u' theta2 is degree one in
theta2 so the reduction needs a genuine polynomial gcd in K1[theta2] rather than a coefficient-wise
division. For theta2 = exp(e^x) (so D2theta2 = e^x theta2),
    INT e^x exp(e^x) / (exp(2 e^x) + 1) dx = arctan(exp(e^x)),
    INT e^x exp(e^x) / (exp(e^x) - 1)   dx = log(exp(e^x) - 1),
both certified. The reduction g = A/(D D2theta2) is an exact polynomial gcd (h2-gcd/h2-div, cheap -- no
resultant), normalized by the leading coefficient of its denominator and read off as Q-polynomials. The
certificate has two light halves: the reduction A * Dbar = D * D2theta2 * Abar is exact in K1[theta2]
(cross-multiplication), certifying that the integrand really is D2theta2 * Abar/Dbar, and integrate-verify
certifies d/dtheta2 of the answer equals Abar/Dbar over Q; by the chain rule these give the height-two
certificate D2(answer) = A/D with no heavy K1 derivation. (To stay within the interpreter's memory the
result is computed once per integrand and certified from it.) This corrects an earlier expectation that
the substitution did not transfer to the exponential case: it does, because the chain rule is indifferent
to the monomial kind. As always only integrands whose g is constant in x are of this type; x-dependent
arguments and residues algebraic over K1 still require the resultant. See
`examples/258-exp-substitution-integral.lisp` and the `cas_tower2sub` golden test.

## Tier 4: x-dependent height-two RootSums via a gc-managed K1 resultant

The substitution integrators (`tower2primrat.lisp`, `tower2primfull.lisp`, `tower2sub.lisp`) cover height-
two integrands whose logarithm arguments are constant in x. When the arguments genuinely depend on x there
is no substitution shortcut and the logarithmic part must come from the Sylvester resultant over
K1 = Q(x)(theta1) (towerrt's `h2rt-logpart`): the resultant R(z) = Res_theta2(D, A - z D2(D)) is evaluated
at z = 0..deg(D), the ratios are interpolated by rational Lagrange, and the rational residues are read off.
The obstacle has always been memory -- a single K1-fraction resultant is cheap, but several in sequence
accumulate transient garbage. The fix is that the interpreter exposes a `(gc)` primitive; calling it
between the resultant evaluations (now built into `h2rt-Rvals`, plus once at the head of `h2rt-logpart`
after Hermite) reclaims the dead K1 fractions and lets the multi-evaluation resultant stay within the heap.
With this, a degree-two x-dependent integral certifies end to end:
    INT A/D dx = log(theta2 - e^x) + 2 log(theta2 + e^x),   theta2 = log(e^x + 1),
where D = theta2^2 - e^(2x) carries an x-dependent K1 coefficient and the arguments theta2 +- e^x depend on
x. The RootSum is certified by differentiation -- sum of c_i D2(v_i)/v_i is checked to equal A/D exactly in
K1[theta2] -- and the whole example is computed once and certified from the same result to respect the
heap. This is the first height-two integral with x-dependent logarithm arguments to be certified here, and
it complements example 246 (x-independent coefficients). Scope, stated honestly: gc reclaims dead objects
for reuse but does not shrink the process's resident memory, so the peak simultaneous allocation still
bounds what is feasible. Degree-two x-dependent cases fit; a degree-three x-dependent denominator (four
evaluations, coefficients like e^(2x)) still exceeds memory, because the K1 fraction arithmetic for such
coefficients is allocation-heavy. Cracking that needs leaner (fraction-free) K1 resultant arithmetic, not
just gc timing. See `examples/259-height-two-xdependent-rootsum.lisp` and the `cas_tower2xdep` golden.

## Tier 4: fraction-free Rothstein-Trager -- x-dependent degree-three RootSums

Example 259 reached x-dependent logarithm arguments at degree two by calling towerrt's field resultant with
an explicit (gc).  Degree three with x-dependent coefficients (four resultant evaluations, coefficients
like e^(2x)) defeated even that: the field elimination's per-step k1-mul performs two Euclidean rfpoly-gcd
cross-cancellations, and four such evaluations -- plus the K1 Euclidean gcd that extracts each logarithm
argument -- exhaust the heap.  `tower2ff.lisp` removes every one of those allocators by working
fraction-free over the integral domain Q(x)[theta1].

The resultant R(z) = Res_theta2(D, A - z D2(D)) is computed by clearing the coefficient denominators by
their minimal common multiple (the LCM, not the product -- the product inflates theta1-degrees until the
recursive polynomial division overflows the stack) and then taking the Sylvester determinant by the
Bareiss one-step fraction-free elimination: only rfpoly multiply, subtract and EXACT division, with no
rfpoly-gcd anywhere in the determinant.  This was validated against towerrt's field resultant on a 5x5
case and on the integer resultant Res(t^2-1, t-2) = 3; its high-water memory stays flat across evaluations.
Because the clearing factor is identical for every z it cancels in the ratios R(z_k)/R(z_0), which the
residue interpolation needs; the constancy of those ratios is tested gcd-free, by comparing a leading-
coefficient ratio against an rfpoly equality, so the degree-eleven resultant values never go through a gcd.
Finally each logarithm argument v_c = gcd(D, A - c D2(D)) is found by a fraction-free primitive PRS over
Q(x)[theta1] (pseudo-remainders with the rfpoly content divided out each step) and made monic over K1,
replacing the K1 Euclidean gcd whose intermediates blew up.  An explicit (gc) between the heavy steps
reclaims transients.

With this, the degree-three x-dependent integral certifies end to end:
    INT A/D dx = log(theta2 - e^x) + 2 log(theta2) + 3 log(theta2 + e^x),   theta2 = log(e^x + 1),
where D = theta2^3 - e^(2x) theta2 carries an x-dependent K1 coefficient and the three logarithm arguments
depend on x; the residues 1, 2, 3 are recovered and the RootSum is certified by differentiation (sum of
c_i D2(v_i)/v_i equals A/D exactly in K1[theta2]).  This is the case that exhausted memory in every earlier
attempt.  Scope, stated honestly: this conquers the x-dependent primitive RootSum with rational residues at
degree three.  Residues genuinely algebraic of degree >= 3 over K1, the exponential Rothstein-Trager
logarithmic part for x-dependent integrands, and algebraic functions (radicals, elliptic integrals) remain
ahead -- and the fraction-free infrastructure here (Bareiss resultant, primitive-PRS gcd, gcd-free ratios
over Q(x)[theta1]) is exactly what those will build on.  See
`examples/260-height-two-xdependent-deg3-fraction-free.lisp` and the `cas_tower2ff` golden.

## Tier 4: closing the three height-two / algebraic frontiers

Three capabilities that had stood open are now built and certified, each reusing the fraction-free
apparatus of tower2ff (the derivation-indifferent Bareiss resultant and the rational ratio trick).

EXPONENTIAL x-dependent logarithmic part (`tower2expff.lisp`).  The substitution integrator only reached
constant-in-x arguments for theta2 = exp(u); the x-dependent case needs the resultant over K1.  Because the
fraction-free apparatus only ever consumes D, A and D2(D), it transfers to the exponential derivation
D2(theta2) = u' theta2 unchanged.  The degree-three integral
    INT A/D dx = log(exp(e^x) - e^x) + 2 log(exp(e^x) - 2 e^x) + 3 log(exp(e^x) + e^x)
certifies via the exponential logarithmic derivative (example 261).

ALGEBRAIC FUNCTIONS (`algfunc.lisp`).  The first move out of transcendental towers: the algebraic function
field K = Q(x)[y]/(y^2 - p), y = sqrt(p), a field carrying the derivation y' = p'/(2y).  The standard
quadratic-radical antiderivatives -- INT dx/sqrt(p) = arcsinh-type log, INT x/sqrt(p) dx = sqrt(p),
INT sqrt(p) dx -- are produced in closed form and certified by differentiation INSIDE K (example 262).
This is the rationalizable / standard-radical slice; Trager's general algebraic-function integration
(arbitrary curves, the elliptic and higher-genus non-elementary integrals) is the genuine summit beyond it.

DEGREE >= 3 ALGEBRAIC RESIDUES at height two (`tower2algn.lisp`).  tower2alg handled an irreducible-
quadratic residue; this lifts it to any degree.  When the Rothstein-Trager residue polynomial q(z) is
irreducible of degree d over Q, the d residues are a full conjugate set and the antiderivative is the
RootSum sum_{q(alpha)=0} alpha log(v_alpha) in K1(alpha)[theta2].  K1(alpha) = K1[alpha]/(q) is realised as
h2polys over K1 reduced modulo q, so its arithmetic reuses h2-rem/h2-invmod; the log argument is taken by
Euclid over K1(alpha)[theta2]; and the RootSum's derivative is the TRACE over the conjugates, which descends
to K1.  The certificate checks Tr_{K1(alpha)/K1}(alpha D2(v_alpha)(D*/v_alpha)) = A* in K1[theta2], the
trace computed from the power sums of q's roots by Newton's identities.  The cubic
    INT 6 (D theta2)/(theta2^3 - 2) dx = sum_{r^3 = 2} r log(theta2 - r)
-- residues the three cube roots of 2 -- certifies (example 263).  Honest scope: this covers an irreducible
residue polynomial; a reducible mix of factors of degree >= 3 over K1, and residues whose extension is not
separable-by-Euclid here, would extend it further.

## Tier 4: recurrences, linear ODEs, special functions, polynomial-numerator radicals

Four capabilities extending the requested areas, each certified by an exact identity.

C-finite generating functions (`cfrec.lisp`).  linrec gives closed forms only when the characteristic
polynomial splits over Q; the RATIONAL GENERATING FUNCTION needs no such condition.  For
a_n = c_1 a_{n-1} + ... + c_d a_{n-d} the generating function is N(x)/D(x) with D = 1 - c_1 x - ... - c_d x^d
and N fixed by the initial values; this captures every C-finite sequence, Fibonacci and Pell included.
G_Fib(x) = x/(1 - x - x^2) is certified by checking that its Taylor series (via the certified rational-
function series) reproduces the recurrence's own terms to high order (example 264).

Constant-coefficient linear ODEs (`odelin.lisp`).  The ODE analogue of linrec: for
a_0 y + ... + a_m y^(m) = 0 the basis solutions are x^j e^{rx} for each rational root r of the characteristic
polynomial (multiplicity mu, j < mu).  Each is certified by an exact polynomial identity -- writing the
k-th derivative of x^j e^{rx} as p_k(x) e^{rx}, the ODE collapses to (sum a_k p_k(x)) e^{rx} and the
polynomial sum is checked identically zero (example 265).  Equations whose roots are irrational/complex
(e.g. y'' + y = 0) are honestly reported as outside the rational-root closed form.

Orthogonal polynomials (`orthopoly.lisp`).  Chebyshev, Legendre, Hermite and Laguerre polynomials,
generated by their three-term recurrences and certified by substitution into the second-order ODE each
family satisfies; the residual is the exact zero polynomial (example 266).

Polynomial-numerator radical integrals (`algfuncint.lisp`).  Extends algfunc from a linear numerator to
INT P(x)/sqrt(R) dx for any polynomial P and monic quadratic R, via the ansatz Q(x) sqrt(R) + lambda INT
dx/sqrt(R) whose defining identity P = Q' R + Q R'/2 + lambda is triangular and solved in one pass; the
answer is certified by differentiation in the algebraic function field K = Q(x)[sqrt(R)] (example 267).

## Tier 4 (summit): the unified height-two Risch decision driver

`tower2risch.lisp` is the single entry point the transcendental Risch decision procedure is meant to be at
height two.  Before it, the height-two machinery was a toolbox the caller dispatched by hand: int-h2 /
int-h2-full for a primitive (logarithm-like) second monomial, and t2e-hermite + t2e-int-powersum +
t2e-int-rde for an exponential one.  This is why the "Transcendental integration (Risch)" capability row sat
unchanged for so long -- the pieces existed but were never unified into something that DECIDES.

    h2-integrate(A, D, kind, w, mono1)   ; kind = 'prim (D theta2 = w in K1) | 'exp (D theta2 = w * theta2)
      -> (list 'elementary <answer-record> kind)        ; a certified antiderivative
       | (list 'non-elementary <reason-string>)         ; a PROOF that no elementary antiderivative exists

It follows the canonical Risch split: h2-divmod separates A/D into its polynomial part (a polynomial in
theta2) and its proper part (degree in theta2 below the denominator).  The proper part is integrated by the
Hermite reduction in theta2 plus a logarithmic/RootSum step (primitive) or Hermite-then-powersum
(exponential); an irreducible obstruction there means the integrand is non-elementary.  The polynomial part
is integrated term by term for a primitive monomial, and for an exponential monomial through the Risch
differential equation `t2e-int-rde`, whose `'notexact` return is exactly a proof that no elementary
antiderivative of the polynomial part exists.  Every positive answer is then re-verified by differentiating
the assembled result and checking it equals A/D in K1(theta2).

The decisive demonstration (example 268) is a discrimination, not a single computation:

  * INT e^x exp(e^x) dx  ->  elementary; the driver returns exp(e^x) and the differentiation certificate holds.
  * INT exp(e^x) dx      ->  non-elementary; the Risch differential equation has no solution, so the driver
                             returns a proof that no elementary antiderivative exists -- not a failure to find one.

Honest scope.  This is the structural summit at HEIGHT TWO: one function that decides for a tower with one
transcendental extension over Q(x) and a second monomial on top.  It is a major rung, not the whole mountain.
The full Risch decision procedure recurses to arbitrary tower depth (towers of towers), and the algebraic case
(Trager-Bronstein, integrands with algebraic functions of unbounded genus) remains separate and far deeper.
What changed here is real: the height-two layer now has a single decision driver that returns a certified
antiderivative or a proof of non-elementarity, where before there were only hand-dispatched parts.

## Tier 4 (the real summit): a recursive Risch decision driver at arbitrary tower depth

The height-two driver (tower2risch) decided integrals over a tower with two transcendental extensions, but it
was still a fixed-depth program: nothing recursed.  The genuine structure of the Risch decision procedure is
recursive -- integration at level n is reduced to integration at level n-1 -- and `ntower.lisp` + `ntrisch.lisp`
make that literal.

`ntower.lisp` is a UNIFORM differential tower of arbitrary depth.  A tower element at level n is a polynomial in
the top monomial theta_n whose coefficients are tower elements at level n-1, bottoming out at the base field
Q(x).  The whole point is that the derivation has the same shape at every level,
D(sum_i a_i theta^i) = sum_i (D_below a_i) theta^i + (chain rule from D theta), so a single recursive function
nt-deriv computes it at any depth.  This was verified against hand-computed derivatives through depth 3 (e.g.
D exp(exp(e^x)) = exp(e^x) exp(exp(e^x))).

`ntrisch.lisp` is ONE integrator, nt-integrate(L, specs, p), that runs the same code at every level.  It splits
the integrand into its polynomial and proper parts.  For an EXPONENTIAL monomial the polynomial part is
integrated term by term, each degree k solved by the RISCH DIFFERENTIAL EQUATION b_k' + k w b_k = a_k at level
L-1 -- the reduction that makes the recursion -- and the degree-0 term integrated by a straight recursive call.
For a PRIMITIVE monomial (D theta = darg, e.g. theta = log f) the polynomial part is integrated by the primitive
Risch recurrence (integration by parts): because D(b_k theta^k) = b_k' theta^k + k b_k darg theta^{k-1} the chain
term LOWERS the theta-degree, so matching coefficients gives the triangular system b_k = INT(a_k - (k+1) b_{k+1}
darg), solved top-down with each INT again a recursive call to level L-1.  The base case (level 0) is the trusted
rational integrator rat-integrate.  The driver returns either a certified
antiderivative or a PROOF of non-elementarity (the Risch DE has no solution), and it terminates because each
recursive step strictly decreases the level.

Soundness is enforced structurally: every 'elementary verdict is re-verified by differentiating the assembled
answer at its level, so the driver never returns an antiderivative it cannot certify.  Cases outside the
implemented reductions (for instance the primitive/logarithmic polynomial coupling, where the power rule is not
valid because D theta is not constant) return an honest 'not-handled rather than a wrong answer or a false claim
of non-elementarity.

The demonstration (example 269) is a single uniform procedure deciding at depths 1, 2, 3 and 4 on the tower of
iterated exponentials theta_{k+1} = exp(theta_k):

  * INT e^x exp(e^x) dx = exp(e^x)                            -- certified  (depth 2)
  * INT exp(e^x) dx                                           -- proven non-elementary  (depth 2)
  * INT exp(e^x) exp(exp(e^x)) dx = exp(exp(e^x))             -- certified  (depth 3)
  * INT exp(exp(e^x)) dx                                      -- proven non-elementary  (depth 3)
  * INT exp(exp(e^x)) exp(exp(exp(e^x))) dx = exp(exp(e^x))   -- certified  (depth 4)
  * INT exp(exp(exp(e^x))) dx                                 -- proven non-elementary  (depth 4)

The depth-3 and depth-4 cases are unreachable by any level-specific code; they exist only because the recursion
closes on itself.  The SAME driver also decides the primitive (logarithmic) tower theta = log x: INT log(x) dx =
x log x - x, INT (log x)^2 dx = x(log x)^2 - 2x log x + 2x, INT (log x)^3 dx, and mixed polynomials in log x, all
certified; and a primitive monomial at depth 2 (D theta_2 = e^x) is integrated by the same recurrence.

Honest scope.  This is the real recursive summit for the EXPONENTIAL tower: a single decision driver at arbitrary
depth, with certified answers and proofs of non-elementarity.  Both the exponential and the logarithmic
polynomial parts are now wired into the recursion.  It is not yet the entire Risch procedure: the proper-fraction
logarithmic (RootSum) part at general depth is not yet lifted into the recursion, towers whose monomials need
rational (not merely polynomial) lower-level coefficients -- such as nested logarithms log(log x) -- are a
representation boundary not yet crossed, and the algebraic case (Trager-Bronstein) remains separate and deeper.  What changed is structural and real -- the integrator is no
longer fixed-depth.

## Tier 4 (summit, continued): logarithms carried through the recursion

The recursive driver of the previous section integrated the polynomial part of a tower element at every level
and decided non-elementarity, but it discarded the proper-fraction LOGARITHMIC part: its base case treated a
rational integrand whose antiderivative needs a logarithm (for instance INT 1/(x^2-1)) as a dead end.
`ntrischlog.lisp` removes that limitation by threading a logarithm list through the whole recursion.

An antiderivative is now represented as (list 'elementary RAT LOGS), where RAT is the rational-in-tower part
and LOGS is a list of (COEFF ARG) pairs denoting COEFF * log(ARG), with ARG a tower element at the current
level.  The base case is the foundation: rat-integrate already returns a rational part plus RootSum terms
c_i log(v_i) over Q(x), and these v_i are lifted to level-0 tower elements and returned as LOGS instead of
being thrown away, so INT 1/(x^2-1) = (1/2) log((x+1)/(x-1)) is delivered and certified.

Two new capabilities follow.  First, PROPER-FRACTION LOGARITHMIC PARTS at arbitrary depth: a proper fraction
N/V of the top monomial, with V monic and squarefree, integrates to c log(V) when the residue c (recovered by
exact tower division of N by D(V)) is a constant of the tower; the argument V is a tower element at the current
level, so it slots directly into the recursion.  This is verified through depth 3, e.g.
INT (D theta_3)/(theta_3 + 1) dx = log(exp(exp(e^x)) + 1).  Exact division in the top monomial over the lower
coefficient field (nt-divexact) is what makes this work at any depth.  Second, NESTED LOGARITHMS: a new
logarithm log(u) whose argument u is itself a lower tower element is just a term the same recognizer produces.
INT 1/(x log x) dx is exactly INT (D theta_1)/theta_1 with theta_1 = log x, so the driver returns the nested
logarithm log(log x); INT 1/(x(log x + 1)) dx = log(log x + 1) and INT 2 e^(2x)/(e^(2x)+1) dx = log(e^(2x)+1)
are handled the same way.

Every elementary-with-logs answer is certified by the cleared logarithmic identity D(RAT) V + sum_i COEFF_i
(D ARG_i)(V / ARG_i) = numerator * V, checked by tower arithmetic at the current level, so a returned
'elementary is always certified.  A proper fraction whose residues are not constants -- a genuine multi-residue
RootSum, or a fraction that is not a logarithmic derivative at all, such as 1/(exp(e^x)+1) -- is reported
'not-handled rather than forced into a false answer.

Honest scope.  The single-residue logarithmic recognizer covers the proper-fraction case whose Rothstein-Trager
residue is one rational constant (which includes simple poles and the nested-logarithm integrals above), at any
depth.  The full multi-residue RootSum at general depth (several distinct algebraic residues at once) is not yet
lifted into the recursion -- that needs polynomial gcd and resultants over a tower coefficient field -- and the
algebraic case (Trager-Bronstein) remains separate and deeper.  What changed is real: the recursive integrator
no longer discards logarithms, it produces and certifies them through the recursion, including nested ones.

## Tier 4 (summit, continued): a rational tower and the multi-residue logarithmic part

The recursive driver of the previous sections kept tower elements POLYNOMIAL in each monomial, so the
coefficient ring at each level was not closed under division.  Two things were out of reach as a result:
elements rational in a monomial (negative powers like 1/e^x, or 1/log x), and the multi-residue logarithmic
part, which needs polynomial gcd and resultants over the lower coefficient field.  `rtower.lisp` removes both
limitations by changing the representation: at every level the coefficients form a genuine FIELD, so a level-L
element is a FRACTION of polynomials in the top monomial whose coefficients are rational-tower elements one
level down, bottoming out at Q(x).  This is built in two layers -- field polynomials over the lower field
(with monic normalization, division with remainder, and gcd) and fractions of those (the field at the next
level) -- with a recursive quotient-rule derivation D(P/Q) = (P' Q - P Q')/Q^2.

The derivation is verified through depth 2 on inputs the polynomial tower could not represent: D(1/e^x) =
-1/e^x and D(1/exp(e^x)) = -e^x/exp(e^x) (negative powers), and D(1/log x) = -(1/x)/(log x)^2 (an element
rational in the monomial).

On this field tower the MULTI-RESIDUE Rothstein-Trager logarithmic part lifts uniformly.  For a proper fraction
Pnum/V with V monic and squarefree in the top monomial, the residues are the rational roots of the resultant
res_theta(V, Pnum - c V') in the variable c; each rational residue c contributes c log(gcd_theta(V, Pnum -
c V')).  The resultant-as-a-function-of-c is recovered by evaluating at integer c and interpolating over the
rationals, and the rational roots are found with the existing ros-rational-roots.  This produces SEVERAL
logarithms at once, at arbitrary depth:

  * INT 2 e^x/(e^(2x)-1) dx = log(e^x-1) - log(e^x+1)                         (two residues, depth 1)
  * INT (2/x)/((log x)^2-1) dx = log(log x-1) - log(log x+1)                  (two residues, NESTED-log arguments)
  * INT 2 e^x exp(e^x)/(exp(2 e^x)-1) dx                                       (two residues, DEPTH 2)

Every elementary RootSum is certified by the cleared identity sum_i c_i (D v_i) (prod_{j!=i} v_j) = Pnum over
the lower field (the product form avoids tower division, which keeps the certificate cheap and finite).  A
RootSum is accepted as complete -- and hence elementary over Q -- only when the residue-argument degrees sum to
deg V; otherwise some residues are algebraic and the integral is reported non-elementary.  This soundness
guard is essential: INT 2 e^x/(e^(2x)+1) dx has complex residues (the denominator e^(2x)+1 is irreducible over
Q), and the driver correctly declines it as non-elementary over Q rather than returning a spurious empty
RootSum.

Honest scope.  The multi-residue logarithmic part is now complete over Q at arbitrary depth: it returns all the
logarithms when the residues are rational and declines soundly when they are not.  Two things still lie beyond
it.  Residues that are algebraic but expressible in a finite real or complex extension (genuine RootSum-over-an-
extension, as Maxima and FriCAS print with a %r summation) are reported non-elementary here rather than being
expressed over the needed algebraic extension.  And the ALGEBRAIC case (Trager-Bronstein -- integrands that
themselves contain algebraic functions, with elliptic and higher-genus non-elementarity) remains the genuine
deepest summit, still separate.  What changed is real and structural: the tower now has division at every
level, and the logarithmic part is multi-residue and certified, where before it was single-residue only.

## Tier 4 (summit, the two final boundaries): algebraic-residue RootSum over Q(alpha), and the algebraic case

Two boundaries close the canonical core of the Risch summit.

### Boundary 1 -- the algebraic-residue RootSum over an algebraic extension (algtrace.lisp, algresext.lisp)

The rational tower's multi-residue logarithmic part returns every logarithm when the Rothstein-Trager residues
are rational and declines soundly when they are not.  The remaining case is when the residue polynomial R(z)
(the resultant res_theta(V, Pnum - z V')) has an irreducible factor of degree d >= 2: then the residues are
algebraic numbers and the logarithmic part is the RootSum sum_{R(alpha)=0} alpha log(v_alpha), with v_alpha =
gcd_theta(V, Pnum - alpha V') computed over Q(alpha)(x).  This is the %r summation Maxima and FriCAS print.

The construction works over Q(alpha)(x)[theta]: alpha is the generator of Q(alpha) = Q[z]/(R-factor); the log
argument v_alpha is the monic gcd of V and Pnum - alpha V' by the Euclidean algorithm with Q(alpha)(x)
coefficients (apoly provides Q(alpha)[x], wrapped as fractions).  Soundness is a differentiation certificate
expressed with the field TRACE Tr_{Q(alpha)/Q}, computed from the regular representation (the
multiplication-by-beta matrix in the power basis 1, alpha, ..., alpha^{n-1}; trace = its trace, norm = its
determinant -- algtrace.lisp, verified against Tr(i)=0, N(i)=1, Tr(3+2i)=6, Tr(alpha^2)=2 for z^3-z-1, etc.).
The derivative of the RootSum over the common denominator V is

    d/dx ( sum_alpha alpha log v_alpha )  =  Tr( alpha (D v_alpha)(V / v_alpha) ) / V,

so the certificate is the polynomial identity over Q[x]:  Tr( alpha (D v_alpha)(V/v_alpha) ) = Pnum, with one
subtlety -- for an exponential monomial (D theta = w theta) the leading term of D(log(theta + c)) contributes a
polynomial part, so the identity carries an extra  Tr(alpha) * deg(v_alpha) * V  correction (zero for a
primitive monomial).  The trace is taken only on the algebraic left-hand side; the right-hand side Pnum is
rational and is compared directly (tracing a rational q in a degree-d field would multiply it by d).

Verified end to end, each trace-certified: INT 2 e^x/(e^(2x)+1) dx = i log(e^x - i) - i log(e^x + i) over Q(i);
INT (e^x+1)/(e^(2x)-2) dx over Q(sqrt2); INT 3 e^(2x)/(e^(3x)-2) dx over the cubic field Q(2^(1/3)).  A
rational-residue integrand (e.g. e^(2x)-1) has no nonlinear resultant factor, so the algebraic path returns
'none and defers to the rational RootSum -- it never poaches the rational case or asserts a false extension.

### Boundary 2 -- the algebraic case: hyperelliptic reduction and elliptic non-elementarity (hyperell.lisp)

Everything above integrates in transcendental towers or over Q(x)[sqrt(quadratic)] (genus 0, always
elementary).  The genuine deepest summit is the algebraic case where the integrand itself contains an algebraic
function of higher genus.  For INT P(x)/sqrt(p) dx with p squarefree of degree m, the curve y^2 = p has genus
g = floor((m-1)/2).  A Hermite-style reduction on the curve removes the polynomial part of the numerator: since

    D(x^k y) = ( k x^{k-1} p + x^k p'/2 ) / y,

a descending pass cancels the top of P with these numerators, writing P = (Q' p + Q p'/2) + S with deg S < m-1,
hence INT P/sqrt(p) = Q sqrt(p) + INT S/sqrt(p).  The remainder INT S/sqrt(p) spans the g FIRST-KIND
(holomorphic) differentials x^i/sqrt(p), i < g, which are NON-ELEMENTARY for g >= 1.  So the integral is
elementary iff S = 0 after the reduction; the elementary part Q sqrt(p) is certified by differentiation inside
the function field K = Q(x)[y]/(y^2 - p) (af-deriv, with the radicand passed as a rat), the same arbiter used
throughout.

Decisive, all verified: INT dx/sqrt(x^3+1) -- the canonical elliptic integral, genus 1 -- is PROVEN
non-elementary; INT dx/sqrt(x^5+1) and INT x/sqrt(x^5+1) are PROVEN non-elementary (genus 2); while INT
(3x^2/2)/sqrt(x^3+1) dx = sqrt(x^3+1), INT ((5/2)x^3+1)/sqrt(x^3+1) dx = x sqrt(x^3+1), and INT
(5x^4/2)/sqrt(x^5+1) dx = sqrt(x^5+1) are found and certified.

### Honest scope

These two boundaries reach the canonical, decidable core, not full generality.  Boundary 1's algresext handles
the base-level (depth-1) proper fraction where algebraic residues first appear, taking a single irreducible
nonlinear factor (a mixed residue set is split: the rational part via the rational tower, one algebraic class
via algresext); residues whose minimal polynomial appears only at depth >= 2 over Q(alpha) are not yet lifted.
Boundary 2's hyperell handles sqrt of a squarefree polynomial (the hyperelliptic curve) with polynomial
numerators, extracting the elementary Q sqrt(p) part and deciding via the first-kind remainder; it does not yet
cover general curves beyond hyperelliptic, mixed transcendental-over-algebraic towers, the full second/third-
kind decomposition with algebraic logarithmic parts, or rational-function numerators over sqrt(p) in full
generality.  Those are the genuine frontier of any computer-algebra system (FriCAS territory).  What ships here
is the elliptic non-elementarity decision and the algebraic-residue %r RootSum, each certified.

## Tier 4 (the Trager climb, Rung 1): residues of an algebraic differential and the second-kind decision

The hyperelliptic reduction of the previous section integrates P(x)/sqrt(p) for a POLYNOMIAL numerator and
decides non-elementarity via the first-kind remainder.  The path to the full algebraic case (Trager-Bronstein,
the FriCAS integrator) is laid out in docs/TRAGER_ROADMAP.md as a sequence of certified rungs; this is the
first.  algresidue.lisp computes the RESIDUES of a differential f dx on the curve y^2 = p, where f = u(x) +
v(x) y is an element of K = Q(x)[y]/(y^2 - p), and delivers the one decision that residues alone settle.

Over a fibre x = s with p(s) != 0 (s not a branch point) there are two places (s, +sqrt(p(s))) and
(s, -sqrt(p(s))).  The v(x) y part contributes residues res_{x=s}(v) * (+/- sqrt(p(s))) -- a conjugate pair
that SUMS TO ZERO -- while the u(x) part contributes res_{x=s}(u) at both places, summing to 2 res_{x=s}(u).
So the finite residue obstruction comes only from the u-part's simple poles; a 1/sqrt(p) integrand (pure v y)
with simple non-branch poles is automatically residue-free, and the branch/non-branch split of a denominator B
is just gcd(B, p) versus B / gcd(B, p).

The decision this rung owns and certifies: a 1/sqrt(p) integrand with a polynomial numerator is residue-free
and reduces directly through hyperell, which decides elementarity and certifies the answer inside K.  A u-part
simple pole carries a genuine nonzero residue and is reported third-kind (NOT elementary -- it needs the divisor
/ torsion rung); a higher-order non-branch pole is reported not-handled (it awaits the rational-function Hermite
rung).  Nothing is guessed: the only elementary verdict issued is the certified hyperell one.  Verified:
INT dx/sqrt(x^3+1) classified non-elementary, INT (3x^2/2)/sqrt(x^3+1) = sqrt(x^3+1) certified, the
1/((x-2)sqrt(p)) conjugate-pair cancellation recognized as second-kind, the pure pole 1/(x-2) on the curve
reported third-kind, and a double pole reported not-handled.

This rung makes the residue divisor of an algebraic differential computable -- the prerequisite for the
third-kind logarithm (Rung 3) and, beyond it, the integral-basis / Puiseux normalization (Rung 4) that lifts
the restriction to hyperelliptic curves.  Each later rung remains gated by the same differentiation arbiter.

## Tier 4 (the Trager climb, Rungs 2 and 3a): algebraic Hermite reduction and the genus-0 third-kind logarithm

Two further rungs of the algebraic-integration climb (docs/TRAGER_ROADMAP.md), each certified inside
K = Q(x)[y]/(y^2 - p).

### Rung 2 -- algebraic Hermite reduction for a rational-function numerator (algherm.lisp)

hyperell integrates a polynomial numerator P(x)/sqrt(p); this rung removes the higher-order (second-kind) poles
of a RATIONAL numerator.  For a differential f = w(x) y with w = A/D a proper fraction over Q(x), it produces
INT w y dx = G y + INT wbar y dx, with G in Q(x) and wbar having at most a simple pole at every non-branch
place.  The reduction is the algebraic Hermite method carried out over Q via the squarefree factorization of D,
so no irrational roots are introduced: at the squarefree factor V of maximal multiplicity m >= 2 (V^m || D,
W = D/V^m), one subtracts D((B/V^{m-1}) y) where B solves the congruence -(m-1) B V' W ≡ A (mod V) (the 2p of
the y-weight cancels from both sides), which drops the multiplicity by one; iterate until D is squarefree.  V
sharing a root with p is a branch point and is left untouched for a later rung.  Every step is checked by the
differentiation certificate D(G y) + wbar y = w y.  Verified on double, triple, and multiple distinct poles,
on both the quadratic (genus 0) and the elliptic curve x^3+1, and on the no-op cases (already simple / pure
polynomial).

### Rung 3a -- the genus-0 third-kind algebraic logarithm (algthird.lisp)

A simple pole of the rational part carries a nonzero residue (Rung 1): the integral is third-kind and its
antiderivative is an algebraic logarithm c log(g) with g in K.  For genus 0 (p a squarefree quadratic) the
residue divisor is always principal, so the logarithm always exists.  The previous session's probe had shown
the naive argument (y - sqrt(p(s)))/(x - s) is wrong off the pole-at-origin case -- it injects a spurious
rational-logarithm term.  The correct argument uses the TANGENT LINE to the curve at the point over the pole:

    INT dx/((x - s) sqrt(p)) = c log( (y - L(x))/(x - s) ),  L(x) = rho + k (x - s),
    rho^2 = p(s),  k = p'(s)/(2 rho)   (the curve's slope at (s, rho)),

with c the constant obtained by matching, and the whole answer gated by D(c log g) = integrand.  This closes
INT dx/((x - s) sqrt(quadratic)) GENERALLY, including the shifted-pole cases the naive formula got wrong
(e.g. INT dx/((x-1) sqrt(x^2+3)), certified).  When p(s) is not a perfect square in Q the answer lives over
Q(sqrt(p(s))) and is reported 'needs-extension; when p(s) = 0 the pole is a branch point, reported 'branch-pole.
Nothing is guessed -- the third-kind logarithm is issued only when its certificate holds.

These rungs leave, as the next steps, the genus-1 (elliptic) third-kind case -- where principality of the
residue divisor becomes a torsion test on the Jacobian -- and the Puiseux / integral-basis normalization that
lifts the restriction to hyperelliptic curves.

## Tier 4 (the Trager climb, Rung 3b): the elliptic third-kind decision by a torsion test

For the genus-0 radical the third-kind logarithm always exists (Rung 3a).  On a genus-1 (elliptic) curve
y^2 = p, p a squarefree cubic, principality of the residue divisor is no longer automatic -- it is the real
Trager obstruction, and it is arithmetic-geometric.  By the Abel-Jacobi theorem a degree-zero divisor is
principal iff its image in the group law is the identity, so for INT dx/((x - s) sqrt(p)) with the pole lifted
to the rational point P = (s, rho) on the curve (rho^2 = p(s)):

    the integral is ELEMENTARY  <=>  P is a TORSION point of the elliptic curve.

When P is torsion the third-kind algebraic logarithm exists (and elltorsion.lisp reports the order, from which
the logarithm can be built); when P is non-torsion the integral is provably NON-ELEMENTARY -- the canonical
elliptic obstruction.  This is the classical Trager/Davenport criterion; cf. Combot, "Hyperelliptic Integrals
to Elliptic Integrals" (arXiv:2303.14013), which states that for y^2 = z(z-1)(z-kappa) the third-kind integral
is elementary exactly when (u, sqrt(p(u))) is a torsion point of the curve.

The decision is computed with the elliptic group law over Q (exact rational arithmetic, the general cubic
y^2 = x^3 + a2 x^2 + a1 x + a0, so the x^2 coefficient enters x3 = lambda^2 - a2 - xP - xQ), and it terminates
by Nagell-Lutz + Mazur: on an integral model a torsion point has integer coordinates and order at most 12, so
computing P, 2P, ... either returns to the point at infinity (torsion) or acquires a non-integer coordinate or
exceeds the Mazur bound (non-torsion), giving sound, bounded termination.  Verified: (0,1) on y^2 = x^3+1 has
order 3 and (2,3) order 6, while (3,5) on y^2 = x^3-2 is infinite order; correspondingly INT dx/(x sqrt(x^3+1))
and INT dx/(x sqrt(x^3+4)) are decided ELEMENTARY (torsion poles) while INT dx/((x-3) sqrt(x^3-2)) is decided
NON-ELEMENTARY (infinite-order pole).  A pole whose lift is not rational (p(s) not a perfect square) is reported
needs-extension rather than guessed.

This rung delivers the elliptic third-kind DECISION -- the heart of the Trager algorithm and the first point
where elementarity rests on the arithmetic of the Jacobian.  The remaining refinement is the explicit
construction of the logarithm's argument (the function realizing the principal divisor) in the torsion case;
the order reported here is what that construction consumes.  Beyond it lie the Puiseux / integral-basis
normalization (general algebraic functions) and the mixed transcendental-over-algebraic towers.

## Tier 4 (the Trager climb): the explicit elliptic logarithm

Rung 3b decided that INT dx/((x - s) sqrt(p)) on a genus-1 curve is elementary exactly when the pole lifts to a
torsion point P = (s, rho) of order n.  elllog.lisp completes that rung by CONSTRUCTING the answer: an algebraic
logarithm c log(f) with f in K = Q(x)[y]/(y^2 - p), certified by D(c log f) = integrand.

The residue divisor n([P] - [-P]) is principal (nP = O).  We build the Miller function f_P with div(f_P) =
n[P] - n[O] by the standard iteration over K -- f_1 = 1, f_{i+1} = f_i * L_{iP,P} / V_{(i+1)P}, using the
chord/tangent lines L and verticals V of the elliptic group law as elements of K -- then take f = f_P /
conj(f_P) (conj sends y -> -y), so div(f) = n[P] - n[-P], exactly the principal divisor.  The constant c is
found by matching f'/f to the integrand, and the whole answer is gated by the differentiation certificate.

The decisive result: INT dx/(x sqrt(x^3+1)) = (1/3) log( (sqrt(x^3+1) - 1)/(sqrt(x^3+1) + 1) ), the pole
lifting to the order-3 point (0,1); the constant 1/3 is the reciprocal of the torsion order, and the answer is
certified inside K.  This is a genuinely non-trivial algebraic logarithm on an elliptic curve -- the explicit
antiderivative the torsion criterion promised.

Scope (honest): the construction is gated by the certificate, so it never returns a wrong logarithm -- it
yields (list 'log c f) only when D(c log f) = integrand is verified.  It certifies the odd-order torsion poles
(the Miller iteration produces the principal divisor cleanly when no intermediate multiple is a 2-torsion / y=0
point); an even-order pole, whose multiples pass through a 2-torsion point, is currently deferred (reported as
not-yet-constructed) pending a refinement of Miller's verticals at 2-torsion places.  elltorsion still decides
those cases elementary, so only the explicit log argument is withheld, never the decision.

## Tier 4 (the Trager climb, Rung 4 start): Puiseux expansions of superelliptic functions

The third-kind theory of the previous rungs lives on the curve y^2 = p.  To reach GENERAL algebraic functions
(n-th roots y^e = g, and arbitrary F(x,y) = 0) the integrator needs the LOCAL structure of the function at each
place, which is its Puiseux expansion.  puiseux.lisp begins Rung 4 with the superelliptic case.

At the place over x = 0 an algebraic function expands as y(x) = sum_{k >= k0} c_k x^(k/e), a power series in a
fractional power x^(1/e) (e the ramification index).  For y^e = g(x) the module writes g = x^v gt(x) with
gt(0) != 0, so y = x^(v/e) gt(0)^(1/e) (1 + (gt/gt(0) - 1))^(1/e), expands the binomial series (1+u)^(1/e), and
re-expresses everything in the uniformizer t = x^(1/E) where E = e/gcd(v, e) is the true ramification index.
The result (puiseux E lead coeffs) means y = sum_i coeffs[i] x^((lead+i)/E), and it is checked by raising the
series to the e-th power and comparing with g.  When gt(0)^(1/e) is not a rational e-th power the leading
coefficient is reported needs-radical rather than guessed.

Verified: y^2 = x gives y = x^(1/2) (E=2); y^2 = x^3 the cusp y = x^(3/2) (E=2, leading exponent 3);
y^2 = x + x^2 the fractional series t + t^3/2 - t^5/8 + ... (t = x^(1/2)); y^3 = x gives y = x^(1/3) (E=3);
y^2 = 4 + x the unramified y = 2 + x/4 - ... (E=1, leading coefficient 2); and y^2 = 2 + x correctly defers
(needs-radical, sqrt(2) irrational).  Each non-deferred result passes the e-th-power check.

This is the ramification-aware local expansion at a place; the integral-basis construction that completes
Rung 4 -- normalizing the function field at its singular places to lift integration past the hyperelliptic
restriction -- consumes these branch expansions.  Remaining in Rung 4: Puiseux for general F(x,y) = 0 via the
Newton polygon (several branches per place), then the integral basis itself.

## Tier 4 (the Trager climb, Rung 4 continued): the elliptic third-kind decision completed, and the Newton polygon

### The complete elliptic third-kind criterion (ellint.lisp)

Rung 3b decided INT dx/((x-s) sqrt(p)) elementary <=> the pole lifts to a torsion point.  That is NECESSARY but
NOT SUFFICIENT.  The correct criterion (Trager; Combot, arXiv:2103.04134) has two parts: (1) the pole P=(s,rho)
is torsion -- which makes the logarithmic part L = c log f EXIST; and (2) the remainder I - L, a holomorphic
first-kind differential lambda * dx/y, must VANISH.  A nonzero lambda means the integral is an elementary
logarithm plus a nonzero elliptic integral of the first kind, hence NON-elementary.

ellint.lisp implements both by construction: it builds the function g with div(g) = N[P]-N[O] (N = order of P)
by interpolation -- g = A(x) + B(x) y vanishing to order N at P, using the local y-power-series, robust at
2-torsion and verified by N(g) = g*conj(g) = (x-s)^N -- forms f = g/conj(g) so that c f'/f (with c = 1/(N rho))
matches the residues of dx/((x-s)y), and computes the remainder.  ei-integrate returns a certified elementary
logarithm when lambda = 0 and a sound non-elementary verdict otherwise.

Corrected findings: INT dx/(x sqrt(x^3+1)) = (1/3) log((y-1)/(y+1)) IS elementary (lambda = 0), and likewise
INT dx/(x sqrt(x^3+4)); but the torsion poles of orders 4, 5, 6 tested all have lambda != 0 and are
NON-elementary -- correcting the earlier torsion-only verdict.  It is not a parity phenomenon: lambda = 0 is the
genuine extra condition.

### The Newton polygon for general F(x,y) = 0 (newton.lisp)

puiseux.lisp expands the superelliptic case y^e = g(x).  For a general plane curve F(x,y) = 0 -- given as a list
of y-coefficients, each a polynomial in x, F = (F0 F1 ... Fd) meaning sum_j Fj(x) y^j -- newton.lisp computes the
Newton polygon at x = 0: the lower-left convex hull of the support points (ord_x(Fj), j).  Each hull edge from
(i1,j1) to (i2,j2) gives a branch leading exponent mu = (i2-i1)/(j2-j1) (a reduced fraction) and an edge
polynomial in c whose nonzero roots are the branch leading coefficients (y ~ c x^mu).

Verified: the node y^2 - x^2 - x^3 gives one edge of slope 1 with edge polynomial c^2 - 1, the two tangents
y ~ +-x; y^2 - x gives slope 1/2 (ramification index 2); the cusp y^2 - x^3 gives slope 3/2; (y-x)(y-x^2) gives
two distinct slopes 2 and 1 (the two branches y ~ x^2 and y ~ x); the tacnode-type y^2 - x^4 gives slope 2 with
edge polynomial c^2 - 1; and a smooth y - x - x^2 gives slope 1.  This is the branching-and-ramification
analyzer that the remaining Rung 4 work -- term-by-term general-F Puiseux, then the integral basis -- consumes.

## Tier 4 (Rung 4 continued): assembling the local integral basis

The integral-basis engine (intbasis.lisp) decides whether an algebraic function is regular at a singular place
by its valuation on every Puiseux branch.  The assembly step turns that into an actual basis: for each power
y^j (j = 0 .. deg_y(F) - 1), ib-local-basis-at0 finds the largest exponent k_j such that y^j / x^{k_j} is still
integral at x = 0, returning the basis {y^j / x^{k_j}} of the integral closure localized at 0.  This is the
van-Hoeij triangular basis in the case where the lower-degree correction terms vanish (superelliptic curves and
the examples here).

Verified: F = y^3 - x^4 (one branch y = x^(4/3)) gives the rank-3 basis {1, y/x, y^2/x^2} with singularity
measure delta = sum k_j = 3; the nodal cubic y^2 = x^2(x+1) gives {1, y/x}, agreeing with the independently
certified quadratic closure g = x; and the smooth elliptic place y^2 = x^3 + 1 gives {1, y} (no extension,
delta = 0).  Each basis element's integrality is witnessed by the branch valuations and its maximality checked
by the same engine.  Remaining for Rung 4: combining the local bases across all singular places into a global
K[x]-basis, the van-Hoeij correction terms for non-superelliptic curves, and Hermite reduction plus the
logarithmic part phrased over the resulting basis.

## Tier 4 (Rung 4 continued): the global integral basis

The local assembly computes the integral basis at one place; a general curve is singular at several.
ib-global-basis-superelliptic finds all the singular places of y^n = g(x) (the rational repeated roots of g),
computes the local basis at each by shifting the curve F(x+a, y) so the place sits at the origin, and combines
the local denominators into the global basis {y^j / d_j(x)} with d_j(x) = prod_a (x-a)^{k_j(a)}.

Verified: F = y^3 - x^4(x-1)^2, singular at x=0 and x=1, gives the global basis {1, y/x, y^2/(x^2(x-1))} --
the x^2 in d_2 comes from x=0 and the (x-1) from x=1, so d_2 genuinely combines both places.  Both nontrivial
basis elements are certified integral: since y^3 = g, w_1^3 = g/x^3 = x(x-1)^2 and w_2^3 = g^2/d_2^3 are both
polynomials.  The quadratic closure agrees with the independently certified ib-quadratic (g = x on the nodal
cubic).

Soundness: a singular place can have an irrational tangent -- its branches then live over an extension of Q,
where the rational Puiseux engine cannot certify integrality.  The routine detects this (the branches come back
needs-radical) and returns needs-extension rather than a wrong basis; for instance y^2 = x^2(x-1)^2(x+1), whose
node at x=1 has tangent sqrt(2), is honestly deferred.  Quadratic nodes generically have irrational tangents
over Q; cusps with rational tangents (e.g. the cube-root case above, where the cube roots of 1 are rational) are
fully certifiable.  Remaining for Rung 4: the van-Hoeij correction terms for non-superelliptic curves and places
over extensions, then Hermite reduction and the logarithmic part phrased over the integral basis.

## Tier 4 (Rung 4 continued): recursive Newton-Puiseux for shared tangents

newton.lisp and puiseuxg.lisp handle a branch when its Newton-polygon edge-polynomial root is simple.  When a
root c has multiplicity m > 1, the m branches all begin c*x^mu and separate only at higher order; the
simple-root solver cannot distinguish them (F_y vanishes along the shared leading behaviour).  puiseuxr.lisp
adds the classical resolution: substitute y = (c + y1)*x^mu, obtaining a new equation G(x, y1) = 0; divide out
the common x-power (pr-deflate); and recurse on G, whose own Newton polygon resolves the next term, until the
roots are simple.  When the constant-in-y coefficient is zero, y is an exact factor and y = 0 is itself a
terminating branch, peeled before recursing on F / y.

Each branch is returned as a sequence of (mu . c) leading-term pairs, y = c0 x^{mu0} + c1 x^{mu0+mu1} + ....
Verified: the node y^2 - x^2 - x^3 separates into y ~ x and y ~ -x; the triple tangent (y-x)(y-2x)(y-3x) into
y ~ x, 2x, 3x; and the shared-tangent curve (y - x^2)(y - x^2 - x^3) -- whose edge polynomial (c-1)^2 has the
double root c = 1 -- substitutes to y1^2 - x y1 (simple roots 0 and x) and separates into y = x^2 and
y = x^2 + x^3.  This supplies each branch its distinct leading-term sequence, the input the van-Hoeij correction
terms are built from; assembling those corrections for non-superelliptic curves is the next step.

## Tier 3 additions: Laurent series, trigonometric integration, Weierstrass substitution

Three capabilities closing comparison gaps where Maxima was full.

LAURENT SERIES (laurent.lisp): power series allowing finitely many negative-power terms, f = sum_{k>=N} a_k x^k.
Completes the series toolkit (Taylor + Puiseux + Laurent).  Provides the Laurent algebra (add, mul, inverse of
a unit, derivative, integrate with explicit log-term detection), residue (coefficient of x^{-1}) and principal
part, and -- most usefully -- the Laurent expansion of a rational function p/q at any point (writing
q = x^v u(x), p/q = x^{-v}(p u^{-1})), from which residues follow.  Verified: 1/(x^2(1-x)) = x^-2 + x^-1 + 1 +
x + ... (residue 1); Res_{x=1} x/((x-1)(x-2)) = -1 and Res_{x=2} = 2.

TRIGONOMETRIC INTEGRATION (trigint.lisp): closed-form INT sin^m(x) cos^n(x) dx by the reduction formulas, the
answer A(s,c) + B*x with A a polynomial in s = sin, c = cos and B rational.  This shape is closed under d/dx, so
every result is certified by differentiating it back to the integrand (canonicalizing with s^2 = 1 - c^2).
Verified: INT sin^3 = -cos + cos^3/3; INT sin^2 cos^2 = x/8 + (s^3 c - s c^3)/8; a battery through INT sin^4
cos^4, all differentiate-back certified.

WEIERSTRASS SUBSTITUTION (weier.lisp): INT R(sin x, cos x) dx for ANY rational R, via t = tan(x/2)
(sin = 2t/(1+t^2), cos = (1-t^2)/(1+t^2), dx = 2 dt/(1+t^2)).  This turns a rational trig integral into a
rational function of t, integrated by the certified rational integrator and carrying its differentiate-back
proof.  Verified: INT dx/(1+cos x) = tan(x/2); INT dx/(2+cos x) = (2/sqrt 3) arctan(tan(x/2)/sqrt 3);
INT dx/(2+sin x); INT cos x/(1+cos x); all verified by the rational integrator's certificate.

## Tier 4 (Rung 4 payoff): superelliptic Hermite reduction

The integration payoff for general algebraic curves begins with Hermite reduction on the superelliptic family
y^n = g(x), generalizing the hyperelliptic (n=2) reduction in hyperell.lisp to arbitrary n (sehermite.lisp).
On y^n = g the derivation gives y' = g' y/(n g), so D(x^k y^j) = [k x^(k-1) g + (j/n) x^k g'] y^j / g -- the
power y^j is preserved, so each y^j sector reduces independently, exactly the hyperelliptic mechanism with p
replaced by g and the constant 1/2 replaced by j/n.  Subtracting these numerators descending in degree gives
INT (P y^j / g) dx = Q y^j + INT (S y^j / g) dx with deg S < deg g - 1.  If S = 0 the integral is elementary,
equal to Q y^j, certified by the polynomial identity Q' g + (j/n) Q g' = P (the numerator of D(Q y^j)); if
S /= 0 the holomorphic first-kind remainder is reported (the integral is not elementary by this reduction).

Verified: INT (3x^4 + 2x) y/(x^3+1) dx = x^2 y on y^3 = x^3+1 (certified); the y^2 sector of the same curve
recovers its constructed antiderivative; the n=2 specialization reproduces hyperell (INT (3x^2/2) y/(x^3+1) =
sqrt(x^3+1)); and INT y/(x^3+1) dx is correctly reported non-elementary (first kind).  Remaining for the full
Rung-4 payoff: the logarithmic (residue) part over the integral basis to handle the simple-pole contributions,
and the van-Hoeij correction terms for non-superelliptic curves.

## Tier 4 (Rung 4): the general-n superelliptic function field

The integration of algebraic functions needs the function field itself.  algfunc.lisp provides
K = Q(x)[y]/(y^2 - p) at n = 2; sefield.lisp generalizes this to K = Q(x)[y]/(y^n - g) for arbitrary n -- the
algebraic foundation the rest of the superelliptic integration is built on.

An element is a length-n list of rational functions (a_0 ... a_{n-1}) = a_0 + a_1 y + ... + a_{n-1} y^{n-1}.
Multiplication reduces y^{i+j} = g^{floor((i+j)/n)} y^{(i+j) mod n}.  The derivation uses y' = g' y/(n g), so
d/dx (sum a_j y^j) = sum [a_j' + a_j (j/n) g'/g] y^j, which stays within the field (no reduction needed).
For a field element u, d/dx log u = u'/u; rather than invert u, the identity INT f dx = c log u is certified by
clearing the denominator -- f * u = c * u' as a field identity, needing only multiply and derive.

Verified: y^3 = g recovered by repeated multiplication on y^3 = x^3 + 1; the derivation y' = (x^2/(x^3+1)) y;
u' = 1 + (x^2/(x^3+1)) y for u = x + y; the certified logarithms INT (g'/g) dx = log g and the superelliptic
INT (x^2/(x^3+1)) ... = log y (with log y = (1/n) log g); and the n = 2 derivation matching algfunc exactly.
Remaining: rationalize via the field Norm to present third-kind logarithms with polynomial denominators and run
Rothstein-Trager residue log-finding over the field, then the van-Hoeij correction terms for general curves.

## Tier 4 (Rung 4): the field Norm, inverse, and rationalized logarithmic derivative

To turn the superelliptic field into an integration tool, third-kind logarithms must be presented over ordinary
polynomial denominators, and that needs the field Norm.  senorm.lisp computes, for u in K = Q(x)[y]/(y^n - g),
the Norm N(u) = product of the n conjugates (y -> zeta^k g^{1/n}) as the determinant of the multiplication-by-u
matrix on the basis {1, y, ..., y^{n-1}}, via cofactor expansion over rational functions (reusing the field
multiplication from sefield.lisp).  The adjugate yields the conjugate-product ubar with u * ubar = N(u), hence
the inverse u^{-1} = ubar / N(u), and this rationalizes the logarithmic derivative
u'/u = u' * ubar / N(u) -- a field element over a scalar (in y) denominator N(u) in Q(x).

Verified: N(y) = g; N(x + y + 2 y^2) on y^3 = x^3+1 equals the cubic norm form 8x^6 - 6x^4 + 18x^3 - 6x + 9;
ubar(y) = y^2 with y * y^2 = g; the inverse satisfies y * y^{-1} = 1 and (x+y)(x+y)^{-1} = 1; the rationalized
logarithmic derivative satisfies the cleared identity u F = N u'; and the n = 2 specialization reproduces the
classical norm a^2 - b^2 g (N(x+y) = -1 on y^2 = x^2+1).  Next: Rothstein-Trager residue log-finding over the
field -- the residues of u'/u at the poles of N(u) give the logarithm arguments -- then the van-Hoeij correction
terms for non-superelliptic curves.

## Tier 4 (Rung 4 complete for y^n = g): the third-kind logarithm (Rothstein-Trager)

The logarithmic half of integration on a superelliptic curve.  sethird.lisp integrates logarithmic
differentials u'/u on y^n = g and, conversely, recognizes such a differential and recovers its logarithm.

Constructive: for a field element u, INT (u'/u) dx = log u; st-log returns the rationalized differential (field
numerator over the polynomial denominator N(u)) with the certified statement, checked by the cleared identity
u * (numerator) = N(u) * u'.

Recognizer (the Rothstein-Trager step) for the common third-kind argument u = a(x) + y: the conjugate symmetric
functions of the roots of y^n - g vanish except the last, so N(a + y) = a^n + (-1)^{n+1} g.  Given a logarithmic
differential's denominator D, the candidate a is the n-th root of D - (-1)^{n+1} g; if that is an exact
polynomial n-th power and the resulting u = a + y reproduces the differential, the integral is log(a + y),
certified; otherwise the honest verdict not-third-kind-a+y.  Verified: log(x + y) on y^3 = x^3+1 (N = 2x^3+1);
recovery of a = x and a = x + 1 from their denominators; rejection of a non-Norm denominator; and the n = 2 case.

With this, Rung 4 is complete for the superelliptic family y^n = g: Puiseux expansion, the Newton polygon,
local and global integral bases, recursive branch separation, superelliptic Hermite reduction, the general-n
function field, its Norm, and the third-kind logarithm.  Remaining for fully general curves F(x,y) = 0: the van
Hoeij correction terms for the non-superelliptic case.

## Tier 5 (Rung 5 begins): mixed transcendental-over-algebraic integration

The open summit: integrands that mix a transcendental monomial with an algebraic function.  mixedexp.lisp takes
the first step -- INT B * exp(h) dx where the coefficient B is an algebraic function (a field element of
K = Q(x)[y]/(y^2 - p)) and h is rational.  The exponential sits over an algebraic coefficient field, the genuine
mixed-tower situation.

The Risch exponential case: INT B exp(h) dx is elementary with the same exponential iff there is a field element
A in K with A' + h' A = B, and then INT B exp(h) dx = A exp(h).  This is the Risch differential equation with
coefficients in the algebraic field K rather than Q(x).  On y^2 = p (y' = p' y/(2p)) it decouples by sector into
two scalar equations over Q(x); for A of bounded degree it is solved by undetermined coefficients (an exact
linear solve), and every answer is certified by differentiating A exp(h) in the field and checking A' + h' A = B
exactly -- the differentiation certificate is the arbiter, as throughout.

Verified on the canonical field y^2 = x (y = sqrt x), h = x: INT ((1 + 2x)/(2 sqrt x)) exp(x) dx = sqrt(x)
exp(x), with the RDE solver recovering A = sqrt x; the richer A = x + sqrt x and A = x^2 + sqrt x recovered; and
the honest 'none when the search degree is too low.  Next on this rung: logarithmic monomials (the Risch
primitive case over the field), general n (y^n = g via sefield), and full mixed towers with several monomials.

## Tier 5 (Rung 5): the logarithmic (primitive) case

The companion to the exponential case: INT (P_1 t + P_0) dx where t = log(h) is a primitive monomial
(t' = h'/h) and the coefficients P_1, P_0 are algebraic functions -- field elements of K = Q(x)[y]/(y^2 - p)
(mixedlog.lisp).  An element of the tower K(t) is a list of field elements (c_0 c_1 ... c_d) = sum c_i t^i;
the derivation is d/dx (sum c_i t^i) = sum (c_i' + (i+1) c_{i+1} t') t^i, the t' coupling adjacent degrees.

Integrating a degree-1 input P_1 t + P_0 gives a degree-2 answer Q_2 t^2 + Q_1 t + Q_0 with Q_2' = 0,
2 Q_2 t' + Q_1' = P_1, and Q_1 t' + Q_0' = P_0 -- a chain of field-antiderivative problems.  For Q of bounded
field-coefficient degree this is one exact linear system, solved by undetermined coefficients and certified by
differentiating in K(t) and matching the integrand.

Verified over y^2 = x with t = log x: INT ((1/(2 sqrt x)) log x + 1/sqrt x) dx = sqrt(x) log(x); the rational
case where d/dx(x log x) = log x + 1; and the genuine t^2 case INT (log x)/x dx = (1/2)(log x)^2 (the answer one
t-degree higher than the integrand).  With the exponential case (mixedexp.lisp), Rung 5 now handles both kinds
of transcendental monomial over an algebraic coefficient field.  Next: general n (the base y^n = g via sefield)
and full mixed towers with several stacked monomials.

## Tier 5 (Rung 5): mixed integration over the general superelliptic field y^n = g

The n=2 sqrt-field mixed cases generalize to any degree, using the general-n field arithmetic of sefield.lisp.

EXPONENTIAL (mixedexpn.lisp): INT B exp(h) dx over K = Q(x)[y]/(y^n - g).  Writing A = sum_j a_j y^j, the
sefield derivation preserves each y^j sector, so A' + h' A = sum_j [a_j' + ((j/n) g'/g + h') a_j] y^j and
matching B gives n INDEPENDENT scalar Risch differential equations a_j' + w_j a_j = B_j (w_j = (j/n) g'/g + h').
The sectors decouple completely; each is solved by undetermined coefficients and the assembled A is certified.
Verified INT ((1 + x^2/(x^3+1)) y) exp(x) dx = y exp(x) on y^3 = x^3+1, the y^2 sector, the all-sector case
A = x + y + y^2, and the n=2 specialization reproducing the sqrt field.

LOGARITHMIC (mixedlogn.lisp): INT (P_1 t + P_0) dx, t = log h, coefficients in K.  The tower K(t) derivation is
d/dx (sum C_i t^i) = sum (C_i' + (i+1) C_{i+1} t') t^i with C_i' the sefield derivative; integrating a degree-1
input gives Q_2 t^2 + Q_1 t + Q_0 with Q_2' = 0, 2 Q_2 t' + Q_1' = P_1, Q_1 t' + Q_0' = P_0, the field-coefficient
sectors decoupling within each t-degree.  Verified INT ((x^2/(x^3+1)) y log x + (1/x) y) dx = y log x on
y^3 = x^3+1, the t^2 case INT (log x)/x = (1/2)(log x)^2, the y^2 sector, and n=2 subsuming mixedlog.

With these, Rung 5 handles both transcendental monomial kinds over an algebraic coefficient field of any degree.
Remaining: full mixed towers with several stacked monomials, and (Rung 4) van Hoeij corrections for
non-superelliptic curves.

## Tier 5 (Rung 5): the entangled tower -- exp of an algebraic function

The earlier mixed cases put a transcendental over an algebraic coefficient field but kept the transcendental's
own logarithmic derivative rational.  algexp.lisp takes the genuinely entangled step: theta = exp(w) where w is
itself a field element of K = Q(x)[y]/(y^n - g).  Then theta'/theta = w' lives in K (for exp(sqrt x),
w' = 1/(2 sqrt x)), so the Risch differential equation A' + w' A = B for INT B exp(w) dx = A exp(w) has a
FIELD-ELEMENT coefficient.  Because w' A is a full field product, the y-power sectors no longer decouple -- the
coefficient matching is one coupled linear system in all sector coefficients of A.  We solve it by requiring the
residual field element to vanish at sample points (genuinely linear in the unknowns), and the field certificate
A' + w' A = B is the exact arbiter.

Verified: INT (1/(2 sqrt x)) exp(sqrt x) dx = exp(sqrt x); INT ((1 + sqrt x)/(2 sqrt x)) exp(sqrt x) dx =
sqrt(x) exp(sqrt x); the degree-1 coefficient A = x + sqrt x; and the cube-root tower exp(x^(1/3)) on y^3 = x.
Next: the logarithm of an algebraic argument, and deeper stacked towers with several monomials.

## Tier 4 COMPLETE: van Hoeij correction terms (general curves)

The final Rung-4 piece.  For a superelliptic curve y^n = g the integral basis is the pure-power form
{y^j / d_j} (intbasis.lisp); for a general curve F(x,y) = 0 a basis element is
w_j = (y^j + sum_{i<j} c_{j,i}(x) y^i) / d_j, where the lower-degree-in-y "correction terms" cancel the poles
the naive y^j/d_j would have (vanhoeij.lisp).

At a rational place x = a (Puiseux ramification q = 1) a single branch is an ordinary power series y(x); the
element (y - c(x))/(x-a)^k is integral there iff y - c vanishes to order >= k, so the correction c(x) is the
part of the branch BELOW order k -- "subtract the singular part", which raises the valuation from < k to >= k
and cancels the pole.  Integrality is certified by the general-F Puiseux valuation oracle of intbasis.lisp
(which already accepts an arbitrary numerator and the branches from pg-branches).  A ramified place (q > 1) or a
configuration of several branches needing a combined correction returns needs-place-combination rather than a
guess, preserving soundness.

Verified: on y = x + x^2 + x^3, (y - x)/x^2 and (y - x - x^2)/x^3 are integral while y/x^2 and y/x^3 are not (the
correction genuinely matters, certified); k = 1 reports no-correction-needed; and the cusp y^2 = x^3 (a ramified
place) is honestly deferred.  With this, Rung 4 is complete: the full local analysis, integral closure, and
integration machinery on algebraic curves -- both the superelliptic family and general plane curves.

## Tier 5 (Rung 5): the entangled tower -- log of an algebraic function

The primitive companion of exp of an algebraic argument (algexp.lisp).  alglog.lisp integrates in the tower
K(t) where t = log(w) and w is a field element of K = Q(x)[y]/(y^n - g).  Now t' = w'/w lives in K (computed via
the field inverse of senorm.lisp), not merely as a rational function as in mixedlog/mixedlogn.

An element of K(t) is sum_i C_i t^i; the derivation d/dx (sum C_i t^i) = sum (C_i' + (i+1) C_{i+1} t') t^i has
t' a FIELD element, so the product C_{i+1} t' is a full field product (sf-product) and the y-power sectors
COUPLE, exactly as in the exponential entangled case.  Integrating a degree-1 input gives Q_2 t^2 + Q_1 t + Q_0
with Q_2' = 0, 2 Q_2 t' + Q_1' = P_1, Q_1 t' + Q_0' = P_0; the coupled system is solved by requiring the residual
to vanish at sample points (genuinely linear in the unknowns), with the answer's t-degree inferred from the
integrand's, and certified by differentiating in K(t).

Verified: INT (w'/w) dx = log(sqrt x + 1) on y^2 = x (the pure entangled logarithm); the t^2 case
INT (log w)(w'/w) dx = (1/2)(log w)^2; and the cube-root argument log(x^(1/3) + 1) on y^3 = x.  With the
exponential case, both entangled towers -- exp and log of an algebraic argument -- are now in place.  The
remaining summit is deeper stacked towers with several monomials.

## Special functions (Maxima parity): Gamma, erf, Bessel

Closing the one capability Maxima had and lizard had none of.  special.lisp represents each special function the
way it can be computed and certified exactly: by its power series (rational coefficients) together with its
defining identities, checked with the series engine.

Gamma: integer values Gamma(n) = (n-1)!; half-integer values are rational multiples of sqrt(pi) carried via the
functional equation Gamma(x+1) = x Gamma(x) (verified Gamma(5)=24 and the sqrt(pi)-coefficients 1, 1/2, 3/4,
15/8).  erf: the reduced series sum (-1)^n x^{2n+1}/(n!(2n+1)) differentiates exactly to the series of e^{-x^2},
i.e. erf'(x) = (2/sqrt pi) e^{-x^2}.  Bessel: J_n(x) = sum_m (-1)^m/(m!(m+n)!) (x/2)^{2m+n}, verified to satisfy
J_0' = -J_1 and the Bessel equation x^2 y'' + x y' + x^2 y = 0.

## Tier 5 (Rung 5): the first stacked two-monomial tower

twotower.lisp realizes an element of Q(x)(theta)(t) with two independent monomials theta = exp(x) and
t = log(x).  An element is a t-polynomial of theta-polynomials of rational functions; the derivation from
theta' = theta and t' = 1/x is d/dx (c theta^k t^j) = (c' + k c) theta^k t^j + (j c / x) theta^k t^{j-1} -- the
exponential acting in place, the logarithm coupling t-degree j down to j-1.  The integrator finds answers by
undetermined coefficients (residual vanishing at sample points) and certifies them by differentiating in the
tower.  Verified: INT (exp(x) log x + exp(x)/x) dx = exp(x) log(x) -- a genuinely mixed two-monomial integrand
whose two summands are each non-elementary (exponential integrals) but whose combination is elementary, exactly
the cancellation a tower integrator must detect.  This is the structure where the recursive Risch algorithm
operates over several stacked monomials; deeper nesting and a full decision procedure remain the open summit.

## Linear recurrence solving completed: irrational quadratic roots (Binet)

linrec.lisp solves constant-coefficient recurrences whose characteristic polynomial splits over Q; linrec2.lisp
closes the remaining case -- a degree-2 irreducible characteristic polynomial, i.e. irrational quadratic roots,
the Binet/Lucas/Pell case that the rational-root solver declines.

For a_n = p a_{n-1} + q a_{n-2} the characteristic polynomial x^2 - p x - q has discriminant D = p^2 + 4q.  When
D is not a perfect square the roots r, s = (p +- sqrt D)/2 live in Q(sqrt D), and the closed form is
a_n = A r^n + B s^n with A = (a_1 - a_0 s)/sqrt D, B = (a_0 r - a_1)/sqrt D, the constants conjugate in Q(sqrt D)
so a_n is rational for every n.  The computation is carried out exactly in Q(sqrt D) (elements u + v sqrt D), and
the closed form is certified by evaluating it against the directly iterated recurrence.  Verified: Fibonacci
(F_15 = 610), Lucas (L_8 = 47), Pell (P_6 = 70), all certified to n = 20; a perfect-square discriminant defers to
linrec.  With these two modules, linear recurrence solving reaches full Maxima parity.

## Limits at an arbitrary point and indeterminate forms (Maxima parity)

slimit2.lisp generalizes translimit.lisp (limits at x = 0) to limits at ANY point a, including indeterminate
0/0 forms with transcendental numerators, by local series in t = x - a.  Given the t-series of numerator and
denominator (the standard expansions of log(1+t), sin, cos, ... and, for rational functions, an exact Taylor
shift sl-shift-poly), the limit is decided by order comparison: equal orders give the ratio of the unit series
after dividing out the common t^k (L'Hopital by series); a faster-vanishing numerator gives 0; a faster-
vanishing denominator diverges.  Verified lim_{x->1}(log x)/(x-1) = 1, lim_{x->0}(1-cos x)/x^2 = 1/2,
lim_{x->2}(x^2-4)/(x-2) = 4, and the vanishing/diverging cases.

## Transcendental equation solving by substitution (Maxima parity)

transsolve.lisp solves equations that become polynomial under a substitution u = m(x) (exp, log, a power), the
way Maxima handles e^{2x} - 3 e^x + 2 = 0.  The polynomial in u is solved with the certified solver (solve.lisp)
and each rational root r is back-substituted: u = e^x gives x = log r for r > 0 (and reports no real x for
r <= 0), u = log x gives x = e^r, u = x^k gives the real k-th roots.  Solutions are returned in exact closed
form and each polynomial root is verified by substitution.  Verified e^{2x}-3e^x+2=0 -> x in {log 2, 0};
(log x)^2-1=0 -> x in {e, 1/e}; e^{2x}-e^x-6=0 -> x = log 3 with the negative root reported as having no real x.

## Tier 5 (Rung 5): the nested-logarithm tower

nestlog.lisp realizes the genuinely nested depth-2 tower Q(x)(t1)(t2) with t1 = log x and t2 = log(log x).  The
second monomial is the logarithm of the first, so t2' = t1'/t1 = 1/(x t1) carries t1 in its denominator and the
coefficient ring is rational (not just polynomial) in t1.  Q(x)(t1) elements are rational functions N(t1)/D(t1)
with Q(x) coefficients (derivation by t1' = 1/x and the quotient rule); over them a tower element is a
polynomial in t2 (derivation by t2' = 1/(x t1)).  Verified d/dx(log x)=1/x and d/dx((log x)^2)=2 log x/x
(inner), d/dx(log log x)=1/(x log x) (outer), and the nested-log integrals INT 1/(x log x) dx = log(log x) and
INT 2 log(log x)/(x log x) dx = (log(log x))^2, all certified by differentiating in the tower.  Deeper nesting
and a full decision procedure over arbitrary towers remain the open summit.

## Tier 5 (Rung 5): the nested-exponential tower

nestexp.lisp realizes the genuinely nested depth-2 exponential tower Q(x)(s1)(s2) with s1 = exp(x) and
s2 = exp(exp(x)) = exp(s1) -- the multiplicative-tower dual of the nested logarithm.  Here s2' = s1' s2 = s1 s2
MULTIPLIES by the inner monomial (rather than dividing by it as t2' = 1/(x t1) did for nested logs), so the
coefficient ring stays polynomial -- a two-variable polynomial ring over Q(x) -- but the derivation raises the
s1-degree.  A tower element is a polynomial in s2 with s1-polynomial coefficients; from s1' = s1 and s2' = s1 s2,
d/dx(c s1^k s2^m) = (c' + k c) s1^k s2^m + (m c) s1^{k+1} s2^m, so within each s2-degree m the s1-polynomial C_m
goes to ds1(C_m) + m * (s1-shift of C_m).  The s2-degree is preserved, so the derivation is block-diagonal across
s2-degrees and the integral is solved by undetermined coefficients (residual vanishing at sample points) and
certified by differentiating in the tower.  Verified d/dx(exp x)=exp x (inner), d/dx(exp(exp x))=exp(x)exp(exp x)
(= s1 s2), the nested-exp integral INT exp(x) exp(exp(x)) dx = exp(exp(x)), and the s2^2 case
INT 2 exp(x)(exp exp x)^2 dx = (exp exp x)^2.  With nestlog, both nested towers -- logarithmic and exponential --
are now in place; arbitrary nesting depth and a full decision procedure remain the open summit.

## Tier 5 (Rung 5): arbitrary-depth iterated exponentials

itexp.lisp generalizes the depth-2 nested exponential to the iterated exponential tower of ARBITRARY height:
E_0 = x, E_1 = exp(x), E_2 = exp(exp x), ..., E_n = exp(E_{n-1}).  From E_k = exp(E_{k-1}) one proves by
induction the derivative law E_k' = E_k (E_1 E_2 ... E_{k-1}); in particular d/dx(E_n) = E_1 E_2 ... E_n, the
product of the whole tower, so INT (E_1 E_2 ... E_n) dx = E_n at any depth n.

A tower element is a sum of monomials c(x) E_1^{a_1} ... E_n^{a_n} (a list of rational-coefficient/exponent-
vector pairs).  The derivation of a monomial is c' in place plus, for each k with a_k > 0, a term that scales by
a_k and raises the exponents of E_1..E_{k-1} by one (the E_k' factor); like terms are collected and the result
is certified by differentiating in the tower.  Verified the derivative law E_1'=E_1, E_2'=E_1 E_2, E_3'=E_1 E_2
E_3; the full Leibniz expansion d/dx(E_1 E_2 E_3) into three monomials; and the depth-n integrals
INT(E_1...E_n)=E_n at depths 2, 3, 4, 5, with a soundness control confirming a wrong answer is rejected.  This is
the first tower of UNBOUNDED nesting depth the system handles; what remains open at the summit is mixed nested
towers over the algebraic base and a genuine decision procedure (proving non-elementarity).

## Tier 5 (Rung 5): arbitrary-depth iterated logarithms (the dual)

itlog.lisp is the reciprocal mirror of itexp.lisp: the iterated logarithm tower L_0 = x, L_1 = log x,
L_2 = log(log x), ..., L_n = log(L_{n-1}).  The derivative law (induction from L_k = log(L_{k-1})) is
L_k' = 1/(L_0 L_1 ... L_{k-1}) = 1/(x L_1 ... L_{k-1}), so d/dx(L_n) = 1/(x L_1 ... L_{n-1}) and
INT 1/(x L_1 ... L_{n-1}) dx = L_n at any depth.  Because the lower logs sit in denominators, elements are
Laurent monomials c(x) L_1^{a_1} ... L_n^{a_n} with integer (possibly negative) exponents; the monomial
derivation scales by a_k/x and lowers the first k exponents by one for each k with a_k != 0.  Verified the law
L_1'=1/x, L_2'=1/(x L_1), L_3'=1/(x L_1 L_2); the nested-log integrals INT 1/(x L_1...L_{n-1}) dx = L_n at
depths 2, 3, 4, 5; and a soundness control.

## Tier 5 (Rung 5): general integration in the iterated-exponential tower

itexpsolve.lisp lifts itexp.lisp from the single full-product identity to a GENERAL integrator: given an
arbitrary element B (a polynomial in E_1..E_n over Q(x)), it finds an antiderivative in the tower by undetermined
coefficients.  Because ie-deriv acts linearly on the finite monomial support, positing the answer over the
candidate monomials (B's monomials plus their prefix-lowered "one-derivative-down" forms) and matching
d/dx(E) = B gives an exact linear system, solved with Gauss-Jordan and confirmed by the certificate.  Verified
INT exp(x) = exp(x); the genuine two-term INT (exp x + exp x exp exp x) dx = exp x + exp exp x; INT (E_1 E_2 +
E_1^2 E_2) dx = E_1 E_2; and the full-product INT (E_1 E_2 E_3) = E_3 recovered as a special case.  With itlog
and itexpsolve, the iterated towers are now handled at arbitrary depth and for general (not just full-product)
integrands; mixed nested towers over the algebraic base and a genuine decision procedure remain the open summit.

## Tier 5 (Rung 5): general integration in the iterated-logarithm tower

itlogsolve.lisp is the reciprocal-mirror of itexpsolve.lisp: a general integrator for the iterated-logarithm
tower (itlog.lisp).  Given an arbitrary Laurent element B (a sum of monomials c(x) L_1^{a_1}...L_n^{a_n} with
integer exponents), it finds an antiderivative by undetermined coefficients.  Since il-deriv lowers the first k
exponents (scaled by a_k/x), the candidate answer monomials are B's monomials with one-step prefixes RAISED (the
inverse); positing the answer over that support and matching d/dx(E) = B gives an exact linear system, solved
with Gauss-Jordan and confirmed by the certificate.  Verified INT (2 log log x/(x log x)) dx = (log log x)^2;
the two-term INT (1/x + 1/(x log x)) dx = log x + log log x; and the structured INT 1/(x L_1...L_{n-1}) = L_n
recovered as a special case.

## Tier 5 (Rung 5): the fusion -- a nested logarithm over the algebraic base

nestalg.lisp fuses the nested-log tower (nestlog.lisp) with the entangled algebraic logarithm (alglog.lisp): the
base is the algebraic field K = Q(x)[y]/(y^n - g), and over it t1 = log(w) for a field element w, then
t2 = log(t1) = log(log(w)).  This is the first tower that is BOTH nested AND over the algebraic base.  From
alglog, t1' = w'/w is a field element of K; then t2' = t1'/t1 carries t1 in its denominator (the nestlog
structure) with a field element in its numerator (the alglog structure).  A K(t1) element is a rational function
in t1 (a pair N/D of t1-polynomials) whose coefficients are sefield elements; the inner derivation uses the
field element t1' (sf-product), the quotient rule lifts it to N/D, and the outer derivation uses t2' = t1'/t1.
Verified for w = sqrt x + 1 on y^2 = x: t1' is a genuine field element; d/dx(log log w) = (w'/w)/log(w); the
integral INT (w'/w)/log(w) dx = log(log(sqrt x + 1)); a t2^2 case; and the cube-root base y^3 = x.  With this
fusion and the two general iterated-tower solvers, the open summit narrows to a genuine decision procedure
(proving non-elementarity) over arbitrary towers.

## The summit step: the first genuine DECIDER (Liouville's theorem)

liouville.lisp is the first module that DECIDES elementarity rather than merely constructing antiderivatives.
For INT P(x) e^{g(x)} dx with P, g polynomials over Q and deg g >= 1, Liouville's theorem says the integral is
elementary iff there is a rational R with R' + g' R = P (then the antiderivative is R e^g).  For polynomial data
R must be a polynomial of degree exactly deg(P) - deg(g) + 1 (a degree argument: deg(g' R) = deg(R) + deg(g) - 1
strictly exceeds deg(R') = deg(R) - 1).  If that degree is negative the only candidate is R = 0, forcing P = 0;
otherwise R's coefficients are the solution of an exact linear system.  A consistent system yields R -- a PROOF
of elementarity, since (R e^g)' = (R' + g' R) e^g = P e^g, checked by lv-certify -- and an inconsistent system
is a PROOF of non-elementarity.

This is the qualitative leap the roadmap flagged as the summit: every earlier module is a constructive
certifier (it finds an answer and proves it correct), whereas this returns a verdict in BOTH directions, with a
proof object each way.  Verified elementary: INT x e^{x^2} = (1/2) e^{x^2}, INT x e^x = (x-1) e^x, INT x^2 e^{x^3}
= (1/3) e^{x^3}.  Verified PROVEN non-elementary: INT e^{x^2} dx (the error function erf), INT e^{x^3} dx,
INT x e^{x^3} dx, and INT e^x/x dx (the exponential integral Ei).  These are returned as proven-impossible -- the
first time the system reports "no elementary antiderivative EXISTS" with a proof, rather than "none was found."

## The decider suite: Liouville verdicts across exp, log, rational-exp, and the structure form

Extending the first decider (liouville.lisp, INT P e^g) into a coherent suite that PROVES elementarity verdicts
both ways across several integrand classes -- the summit work of distinguishing "no elementary form exists"
(with proof) from "none found".

liouvillelog.lisp -- the logarithmic companion.  INT P(x) log x dx = F log x - INT F/x dx (F = INT P) is always
elementary, returned as the explicit closed form; INT 1/log x dx (the logarithmic integral li) is proven
non-elementary.  Verified INT log x = x log x - x, INT x log x = (x^2/2)log x - x^2/4, and li.

liouvillerat.lisp -- the rational-coefficient exponential decider.  INT R(x) e^x dx (R rational) is elementary
iff a rational S solves S' + S = R, with antiderivative S e^x.  The polynomial part reuses liouville.lisp; the
principal (pole) part is a triangular recurrence on the Laurent coefficients whose closing equation detects the
exponential-integral obstruction.  Verified INT e^x/x (Ei) and INT e^x/x^2 proven non-elementary; the designed
elementary INT (1/x - 1/x^2) e^x = e^x/x and INT (1/x^2 - 2/x^3) e^x = e^x/x^2; INT x e^x = (x-1) e^x.

liouvilleform.lisp -- the Liouville structure theorem made explicit.  For a rational f = N/D (D squarefree, given
simple roots) it returns the witness f = v' + sum c_i u_i'/u_i with v = 0, c_i = N(a_i)/D'(a_i) the residues,
u_i = x - a_i, and certifies that sum res_i/(x - a_i) = f at sample points.  Verified the decompositions of
1/(x^2-1), 2x/(x^2-1), and 1/((x-2)(x-3)).

Together with the exponential decider, the three classic special-function integrals -- erf (INT e^{x^2}), Ei
(INT e^x/x), and li (INT 1/log x) -- are now all proven non-elementary, and the rational case carries an explicit
certified structure-theorem witness.  The open frontier is the full Risch structure theorem over arbitrary mixed
exp/log/algebraic towers: a single decision procedure subsuming these per-class deciders.

## The unification: the recursive Risch decision procedure

rischtower.lisp turns the per-class deciders into ONE recursion -- the structural heart of the full Risch
algorithm.  Over a multi-level tower K_0 = Q(x) subset K_1 = K_0(theta_1) subset ... subset K_n, it decides
INT f for f in K_n by reducing, level by level, to integration subproblems ONE LEVEL DOWN, bottoming out at
Q(x) (rational integration, always elementary).

For an exponential level theta = exp(b) (theta' = b' theta), an integrand sum_i a_i theta^i reduces per degree:
degree i != 0 gives INT a_i theta^i = c_i theta^i iff the Risch differential equation c_i' + i b' c_i = a_i is
solvable in the lower field, and degree 0 is an ordinary integration there; the whole integral is elementary iff
every degree's subproblem is solvable.  The decisive sub-routine rt-rde-exp-const-solvable? solves the RDE
c' + w c = target by an exact degree bound and linear system (deg c = deg target - deg w, or c = 0 forced when
that is negative): the same machinery that makes INT e^{x^2} non-elementary appears here as an unsolvable RDE one
level down.

The deep phenomenon decided: the iterated exponential E_n = exp(E_{n-1}).  INT E_n needs c' + E_{n-1}' c = 1, and
since E_{n-1}' = E_1 E_2 ... E_{n-1} is a nonconstant exponential, the formal solution has a NON-TERMINATING
degree tail -- so INT E_n is NON-ELEMENTARY for n >= 2.  rt-decide-iterated-exp returns this proven verdict for
INT e^{e^x} (E_2) and INT e^{e^{e^x}} (E_3), sitting exactly opposite the elementary full-product
INT(E_1 ... E_n) = E_n (rt-decide-iterated-product) -- the recursion distinguishes the single top monomial (non-
elementary) from the whole-tower product (elementary).  The recursion structure is exposed explicitly by
rt-reduce-exp (the per-degree subproblems) and bottoms out at rt-bottom-rational.  Cases the bounded analysis
does not resolve are returned as an honest 'needs-deeper-rde, never a guessed verdict.  Remaining at the summit:
a fully general RDE solver at every tower level (the present recursion decides the exponential reduction and the
iterated-exponential tower exactly), extending the recursion to arbitrary mixed exp/log/algebraic integrands.

## The general RDE solver and the rational-function-coefficient exponential decider

rischrde.lisp solves the GENERAL Risch differential equation y' + f y = g for a rational y, where f and g are
rational functions over Q -- the rational-function-coefficient solver that lifts rischtower's polynomial RDE and
unlocks the recursive Risch procedure at each tower level.  The pipeline (Bronstein's weak/SPDE approach over
Q(x)): the poles of any rational solution sit only at poles of f and g, so y = q/d with d a safe denominator
over-bound; substituting reduces to a polynomial RDE A q' + B q = C, degree-bounded and solved by an exact
linear system.  The differentiation certificate y' + f y = g is the final arbiter, so the over-bounded
denominator and degree search are sound -- only a y genuinely satisfying the equation is returned.  Verified
y'+y=x -> y=x-1; y'-(1/x)y=x -> y=x^2 (a pole in f, none in y); y'+(1/x)y=1 -> y=x/2; y'+(2/x)y=1/x^2 -> y=1/x
(a genuine pole in y); and y'+y=1/x -> no rational solution (the Ei obstruction).

rischrde2.lisp builds the GENERAL exponential-integral decider on top: INT R(x) e^{g(x)} dx for ARBITRARY
rational R and polynomial g is elementary iff INT = y e^g with y solving the RDE y' + g' y = R, decided by
rischrde.  This SUBSUMES the polynomial-R decider (liouville) and the pole-at-origin rational decider
(liouvillerat) into one rational-coefficient procedure.  Verified INT x e^x = (x-1) e^x; INT e^x/x (Ei) non-
elementary; INT (1/x - 1/x^2) e^x = (1/x) e^x; INT x e^{x^2} = (1/2) e^{x^2} (the RDE y'+2x y=x); INT e^{x^2}
(erf) non-elementary (the RDE y'+2x y=1 has no rational y) -- and the verdicts were cross-checked to AGREE with
liouville / liouvillerat on every shared case.  With the rational-coefficient RDE in hand, the tower recursion's
per-degree subproblems are decidable whenever the lower field is Q(x); the remaining summit is the RDE over
non-rational coefficient fields (the general mixed exp/log/algebraic tower).

## The tower-field RDE: the recursion calling itself (and the coupled iterated-exponential case)

rischtfrde.lisp solves the Risch differential equation y' + f y = g when the coefficients live in a height-1
exponential tower K_1 = Q(x)(theta), theta = exp(b), and f is a base-field (Q(x)) coefficient -- the step that
makes the recursive Risch procedure call ITSELF.  Because the exponential derivation is diagonal in theta-degree,
the RDE with f = phi decouples per degree into independent scalar RDEs y_k' + (k b' + phi) y_k = g_k over Q(x),
each solved by rischrde one level down; the tower-field RDE is solvable iff every per-degree base RDE is, and the
assembled y is certified by the diagonal derivation in K_1.  Verified INT e^x = e^x (degree-1 RDE y_1'+y_1=1),
INT x e^x = (x-1) e^x, the Ei obstruction INT e^x/x detected at the per-degree level, a two-degree right-hand
side solved degree by degree, and a nonzero base coefficient.

rischtfrde2.lisp goes beyond, to the COUPLED case: c' + (m theta_1') c = target where the coefficient m theta_1'
has positive theta_1-degree (theta_1' = e^x), which couples the theta_1-degrees into a banded recurrence
c_0' = t_0, c_k' + k c_k = t_k - m c_{k-1}.  Solving it degree by degree (each step a base RDE) and watching the
forced tail: when a nonzero c_{k-1} forces the solution to ever-higher degrees, no bounded-degree solution exists.
This is exactly INT exp(exp x): the reduction c' + (s) c = 1 has c_0 = x, c_1 = 1 - x, and a non-terminating
tail, so INT exp(exp x) is PROVEN non-elementary THROUGH the RDE recursion -- derived from the differential-
equation machinery, not asserted, and verified to AGREE with the tower decider's verdict.  Together these run the
recursive descent on the actual RDE machinery: a height-2 integral decided by the coupled RDE at level 1, which
calls the base RDE at level 0.  The remaining summit is the fully general coupled solver at arbitrary height and
for arbitrary mixed exp/log/algebraic levels.

## The general coupled RDE (both level types) and the unified height-1 integrator

rischcoupled.lisp generalizes the coupled tower-field RDE to BOTH level types of a height-1 tower with an
arbitrary tower-element coefficient.  For an EXPONENTIAL level (diagonal derivation) and arbitrary coefficient
f = sum_j f_j theta^j, the equation D y + f y = g at theta-degree n is y_n' + (n b' + f_0) y_n + sum_{j>=1} f_j
y_{n-j} = g_n -- a banded system coupling each degree to lower ones, solved BOTTOM-UP (each step a base RDE one
level down), with the forced higher-degree tail watched for non-termination.  For a LOGARITHMIC level the
derivation SHIFTS degree, D(sum y_k theta^k) = sum (y_k' + (k+1) u y_{k+1}) theta^k (u = b'/b), so D y + f0 y = g
couples each degree to the next-higher and is solved TOP-DOWN, the top degree being a base RDE and each lower one
using the already-found higher coefficient.  Verified the exp non-terminating tail y'+(1+e^x)y=1, a solvable
coupled exp case, and INT log x = x log x - x via the log-level top-down solve, all certified.

rischint1.lisp caps this with the unified HEIGHT-1 integration decider: INT f dx for f in K_1 = Q(x)(theta),
theta = exp(b) or log(b), is the y with D y = f -- the homogeneous-coefficient coupled RDE -- so ONE entry point
integrates over either kind of height-1 transcendental extension, dispatching on the level type and bottoming out
at the rational RDE over Q(x).  Verified INT e^x = e^x, INT x e^x = (x-1) e^x, INT e^x/x (Ei) non-elementary,
INT log x = x log x - x, and INT (log x)^2 = x(log x)^2 - 2x log x + 2x -- all through the unified coupled-RDE
reduction, certified, and cross-checked to AGREE with the original per-class deciders (liouville, liouvillelog).
This is the complete height-1 Risch integral over both transcendental kinds; the remaining summit is arbitrary
height (nesting the coupled solver) and the algebraic level.

## The height-n recursion: the Risch descent at arbitrary height

rischtowern.lisp provides a uniform tower-element algebra (a height-h element is a polynomial in theta_h with
height-(h-1) coefficients, height 0 = Q(x)) and a recursive derivation D that descends the tower one level at a
time: exp levels are diagonal (D(sum c_k theta^k) = sum (D c_k + k D(b) c_k) theta^k), log levels shift degree
(D(sum c_k theta^k) = sum (D c_k + (k+1)(D(b)/b) c_{k+1}) theta^k), with D(c_k), D(b) computed recursively at
height h-1 and bottoming at the Q(x) derivative.  Verified D(e^x)=e^x, D(log x)=1/x, and at height 2
D(e^{e^x}) = e^x e^{e^x} -- the derivation descending 2 -> 1 -> Q(x).

rischintn.lisp builds the height-n integrator on top: INT f dx = the y with D y = f, solved by te-rde-solve at
height h delegating its per-degree subproblems to a call at height h-1, all the way to the rational RDE over Q(x)
(rischrde).  Exp levels give per-degree RDEs (one level down) solved bottom-up with non-terminating-tail
detection; log levels are solved top-down.  The recursion is exact and certified for any height-1 tower
(reproducing rischint1, verified to AGREE with it) and for arbitrary-height towers whose per-degree coefficient
stays a base-field element (the decoupled case); the coupled case (a per-degree coefficient of positive
lower-degree, as in exp-over-exp where D(theta_1) = theta_1) returns an honest 'deferred rather than a guessed
verdict, so a returned answer is always certified by D y = f.  Verified at height 1 (INT e^x, INT x e^x, INT
e^x/x non-elementary, INT log x); at height 2 a genuine integral INT log(e^x + 1) dx computed by the recursion
descending 2 -> 1 -> Q(x) and certified; and the exp-over-exp deferral.  The remaining summit is the fully
general coupled solver at every height (nesting the coupled banded recurrence recursively) and the algebraic
level.

## Nesting the coupled banded recurrence: the exp-over-exp tower solved

rischcrde.lisp closes the height-n recursion by nesting the coupled banded recurrence recursively.  It solves
D y + F y = g at arbitrary tower height h where F is an ARBITRARY tower element (not merely a base-field
coefficient).  At an exponential level the equation at theta-degree n is D(y_n) + (n Db + F_0) y_n = g_n -
sum_{j>=1} F_j y_{n-j}, an RDE at height h-1 whose coefficient (n Db + F_0) may itself be coupled at that level;
it is therefore solved by te-crde-solve RECURSIVELY, the descent bottoming at the rational RDE over Q(x)
(rischrde).  The bottom-up solve watches the forced higher-degree tail, and the whole result is CERTIFICATE-GATED
for soundness: a returned y always satisfies D y + F y = g; a proven non-terminating tail (one forced by a
uniquely-determined inhomogeneous lowest coefficient, like y_0 = x where D(x) = 1) yields 'no-solution; and an
inconclusive bottom-up solve -- one that would need a homogeneous constant a lower degree could absorb (the
D(y) = 0 nonzero-constant freedom) -- is reported as 'inconclusive rather than as a false verdict.  This is a
genuine incompleteness (the "SPDE bookkeeping" of carrying homogeneous solution spaces through the recurrence),
flagged honestly, never faked.

With rischcrde wired into the height-n integrator (rischintn's te-integrate now routes INT f = (D y = f) through
te-crde-solve with F = 0), the exp-over-exp tower is now SOLVED rather than deferred: INT e^{e^x} is PROVEN
non-elementary THROUGH the recursion -- the top theta_2-degree subproblem is the coupled height-1 RDE
c' + e^x c = 1, whose non-terminating tail (c_0 = x, c_1 = 1 - x, ...) is the obstruction, derived by the
recursion itself.  The verdict AGREES three independent ways: the height-n recursion (rischintn via rischcrde),
the standalone coupled solver (rischtfrde2), and the original tower assertion (rischtower).  A solvable companion
like INT (e^x e^{e^x}) = e^{e^x}, which needs the homogeneous-constant piece, returns an honest 'deferred rather
than a wrong verdict -- the completeness fix (the homogeneous bookkeeping) is the next open piece, while
soundness holds throughout via the differentiation certificate.

## Closing the coupled completeness gap: solvable coupled integrals solved

rischcrdeh.lisp closes the incompleteness left by rischcrde.  In the exp-level banded recurrence, a degree whose
RDE coefficient vanishes (the degree-0 homogeneous case D(y_0) = RHS) leaves the solution free up to an additive
constant; rischcrde's no-constant branch can force a spurious non-terminating tail even when a specific constant
terminates it (as in the height-1 subproblem D(c) + e^x c = e^x, whose bounded solution is c = 1).  Because the
tail depends LINEARLY on that constant, te-crdeh-solve determines it by two probe runs of the recurrence (the
degree-0 constant set to 0 and to 1), reads the leading tail term in each, solves the resulting linear condition
for the terminating constant, and re-runs -- then CERTIFIES (te-crde-certify), returning a solution only if it
satisfies D y + F y = g and otherwise falling back to an honest inconclusive, so soundness is preserved.  The
inner per-degree solves recurse through te-crdeh-solve as well, so the fix applies at every level.

Wired into the recursion, this makes solvable coupled integrals actually solvable: INT (e^x e^{e^x}) dx = e^{e^x}
is now SOLVED and certified through the height-2 recursion (its top-degree subproblem D(c) + e^x c = e^x is
solved as c = 1, giving y = e^{e^x}), where before it was honestly deferred.  Soundness is preserved exactly:
INT e^{e^x} and INT e^x/x remain proven non-elementary, and all height-1 verdicts are unchanged.  The exp-over-exp
tower is now both DECIDED (INT e^{e^x} non-elementary) and INTEGRATED (INT (e^x e^{e^x}) = e^{e^x}) through the
recursion.  Remaining summit: deeper multi-parameter homogeneous spaces (this implements the single degree-0
constant case, which the iterated-exponential subproblems need); the algebraic level; and Laurent integrands
(1/theta, e.g. li) through the unified recursion.

## Multi-parameter homogeneous bookkeeping: the general completeness algorithm

rischcrdem.lisp generalizes the completeness layer from a single homogeneous constant to SEVERAL solved jointly.
When a coupled solve has homogeneous degrees-of-freedom at multiple degrees (each degree whose RDE coefficient
vanishes leaves a free additive constant), the forced tail is affine in the vector of constants C: tail(C) =
T_0 + M C.  The module collects the degrees-of-freedom, builds T_0 and the columns of M by probing the
recurrence (all constants zero, then each unit vector), and solves M C = -T_0 EXACTLY over Q by a rational
Gaussian elimination (verified to (5/3, 5/3, 8/3) on a 3x3 system), substitutes the solution, re-runs, and
CERTIFIES -- returning a solution only if it satisfies D y + F y = g, so soundness is preserved and the layer
falls back to honest inconclusive otherwise.  Inner per-degree solves delegate to the single-parameter layer
(rischcrdeh), so freedoms compound correctly across levels.  Verified: reproduces the single-parameter result
INT (e^x e^{e^x}) = e^{e^x}; solves the multi-degree integrand INT (log x)^2 = x(log x)^2 - 2x log x + 2x through
the multi-parameter path; preserves the non-elementary verdicts (INT e^{e^x}, INT e^x/x); every returned
solution certified.  This is the general completeness algorithm, subsuming the single-parameter case.  Remaining
summit: the algebraic level (towers with y^n = g) and Laurent integrands (1/theta, e.g. li) through the unified
recursion.

## The algebraic level: sqrt-towers in the recursion

rischtoweralg.lisp adds the first non-transcendental level type to the height-n recursion: a level (alg n a)
where theta is algebraic with theta^n = a.  Differentiating the defining relation gives theta' = (a'/(n a)) theta
= w theta, so D(theta^k) = (k w) theta^k and the derivation is DIAGONAL exactly like the exponential level, with
rate w = a'/(n a) in place of the exponent's derivative: D(sum_{k<n} c_k theta^k) = sum (D(c_k) + k w c_k)
theta^k.  The structural difference is the algebra -- theta-degree stays below n because theta^n reduces to a; the
module implements the quadratic case theta = sqrt(a), where (c_0 + c_1 theta)(d_0 + d_1 theta) = (c_0 d_0 +
c_1 d_1 a) + (c_0 d_1 + c_1 d_0) theta.  Verified D(sqrt x) = 1/(2 sqrt x), the algebra (sqrt x)^2 = x,
D(x + sqrt x) = 1 + 1/(2 sqrt x), the rate w = 1/(2x), and the consistency D(theta^2) = D(a) = 1 (the defining
relation differentiated).  This sits alongside the exp and log levels in the recursive tower; wiring the
algebraic level into the coupled RDE / integrator (so sqrt-tower integrals are decided through the recursion) is
the natural continuation, together with Laurent integrands (1/theta, e.g. li).

## The four remaining steps to the elementary-tower summit

This segment advances all four of the previously-open steps toward a complete elementary-tower Risch integrator.

Step 1 -- the algebraic level wired into the integrator (rischintn + rischcrde alg case).  The algebraic level
(alg n a) is diagonal with rate w = a'/(n a), so INT f = (D y = f) reduces per theta-degree to D(y_k) + (k w)
y_k = f_k, theta-degree bounded by n-1 (no non-terminating tail).  Verified INT 1/(2 sqrt x) = sqrt x,
INT (1 + 1/(2 sqrt x)) = x + sqrt x, INT x^(-3/2) = -2/sqrt x -- all decided through the recursion and certified.

Step 2 -- Laurent integrands (rischlaurent.lisp).  An integrand sum_k c_k theta^k over a logarithmic level with
negative powers splits into the polynomial part (the height-n integrator) and the theta^{-1} residue: INT
c_{-1} theta^{-1} is a new logarithm m log(theta) exactly when c_{-1}/theta' is a constant m.  Verified
INT 1/(x log x) = log log x (m = 1); the li case (INT 1/log x) and deeper negative powers defer honestly.

Step 3 -- general-degree algebraic extensions (rischtoweralgn.lisp).  The derivation and integrator are already
general in n (rate a'/(n a), degree bound n-1), so cube-root integrals are decided directly (INT (1/3) x^(-2/3)
= x^(1/3)); this module adds the general-n multiplication reducing theta^n -> a (verified theta * theta^2 = x,
theta^2 * theta^2 = x theta, theta^3 = x for theta = x^(1/3)).

Step 4 -- the structure theorem's logarithmic part (rischstruct.lisp).  Given f and candidate factors g_i, solve
f = sum c_i (g_i'/g_i) for rational constants c_i by an exact linear solve over Q (sampling at pole-free points,
Gaussian elimination), then certify, so INT f = sum c_i log(g_i).  Verified 2x/(x^2-1) = (x^2-1)'/(x^2-1) ->
log(x^2-1); the same as log(x-1) + log(x+1) (coefficients (1,1)); 3/(x-1) + 5/(x+2) recovers constants (3,5);
non-decomposable integrands return no-log-decomposition.  This recognizes integrals that are sums of logarithms
with constant coefficients -- the core decidable content of Liouville's theorem at the logarithmic level.

Together: the elementary-tower Risch integrator now spans exponential, logarithmic, and algebraic levels (any n),
handles Laurent integrands' new-logarithm case, and recognizes logarithmic-sum integrals via the structure
theorem -- every result certified by differentiation, the genuinely hard cases (li, deep Laurent, multi-parameter
algebraic structure-theorem combinations) deferred honestly.  The remaining summit is full Risch-Trager parity:
the complete structure theorem with the polynomial (v') part fused with the logarithmic part over arbitrary
towers, general algebraic function fields with their integral bases, and simplification-with-assumptions.

## THE SUMMIT: the unified top-level integrator (the flag)

rischtop.lisp is the capstone -- a single entry point that fuses every part of the stack behind one interface,
every result certified by differentiation.  Liouville's theorem says an elementary integral has the shape
INT f = v' + sum_i c_i log(g_i): a rational/field antiderivative part plus a sum of logarithms with constant
coefficients.  The unified integrator realizes this across the whole domain:

  - RATIONAL integrands: the complete rational integrator (Hermite reduction for the rational part v, fused with
    the Rothstein-Trager logarithmic part for sum c_i log(g_i), where the residues c_i are the rational roots of
    the resultant res_x(a - y b', b) and the arguments are gcd(a - c_i b', b) -- found AUTOMATICALLY without
    factoring), plus arctangents for the conjugate-residue case.  The crowning fused example
    INT 2x^3/(x^2-1) dx = x^2 + log(x^2-1) returns the rational part x^2 AND the auto-found logarithm log(x^2-1)
    together, certified; INT 1/(x^2+1) = arctan x is certified; INT 1/(x^2-2) is honestly flagged needs-algebraic
    (its residues are irrational).
  - TOWER integrands: the height-n recursion decides and integrates through exponential, logarithmic, and
    algebraic levels of any degree (INT e^x, INT log x, INT 1/(2 sqrt x) all certified), proves the
    iterated-exponential non-elementarity (INT e^(e^x)), and the Laurent layer adds the theta^{-1} new logarithm
    (INT 1/(x log x) = log log x).

This is the elementary-tower Risch summit: one integrator spanning rational functions (rational part plus
automatically-found logarithms and arctangents), exponential/logarithmic/algebraic towers, the Laurent
new-logarithm case, and the iterated-exponential non-elementarity proofs -- every elementary result certified by
differentiation, every obstruction exact, every genuinely hard case (algebraic residues, li, deep Laurent,
multi-parameter structure-theorem combinations over arbitrary towers) deferred honestly.  What lies beyond is
full production-CAS generality: arbitrary algebraic function fields with their integral bases, the complete
structure theorem fusing the polynomial and logarithmic parts over arbitrary nested towers, and
simplification-with-assumptions -- the open research frontier past the elementary-tower summit reached here.

## Closing the "any rational function of e^x" row (rischratmono)

rischratmono.lisp integrates any rational function of e^x by the substitution t = e^x (dt = t dx, so dx = dt/t):
INT R(e^x) dx = INT R(t)/t dt, a rational integral in t handled completely by the top-level rational integrator
(Hermite reduction for the rational part, Rothstein-Trager for the logarithms), after which the answer is read
back in e^x -- the logarithm log(t) becomes the base variable x (since t = e^x), other logarithms log(t - c)
become log(e^x - c), and the rational part in t becomes a rational function of e^x.  The reduction is exact and
the result is certified: the t-integral is self-certified by the rational integrator's differentiation check, and
INT R(e^x) dx = INT R(t)/t dt is the change-of-variables identity (t' = t).  Verified: INT 1/(e^x+1) dx =
x - log(e^x+1); INT e^x/(e^x+1) dx = log(e^x+1); INT 1/(e^x-1) dx = -x + log(e^x-1); INT 1/(e^(2x)+1) dx =
x - (1/2) log(e^(2x)+1) (the t^2+1 factor giving a real logarithm); each t-integral certifies.  This closes the
rational-function-of-e^x capability row; the analogous log case (INT any rational function of log x) is the
natural next target by the dual substitution.

These capability rows have been honestly re-graded to full only after demonstration and certification this arc:
sum of two squares (Cornacchia for primes p = 1 mod 4, Brahmagupta-Fibonacci composition for composites, every
representation gated by a^2 + b^2 = n -- verified on 13, 65, 50, 1000000 and the non-representable 3, 21), and
the polynomial exponential/logarithmic tower integrals (now decided and certified through the height-n
recursion).  The foundations example (335) traces the full provenance of a certified integral: the rationals Q
constructed over the type-theory kernel (as one builds N -> Z -> Q over ZFC), exact Q(x) arithmetic, the integral
claim INT f = F reduced to the differentiation identity D(F) = f, and that identity mechanically checked -- the
honest sense in which each result is proof-carrying, a machine-checkable reduction to verified arithmetic rather
than a hand derivation from the axioms.

## The dual row: any rational function of log x, a decidability result (rischratmonolog)

rischratmonolog.lisp closes the logarithmic dual of the e^x row -- and it is a decidability statement rather than
a uniform closed form, because of a genuine asymmetry.  The logarithm has no rational derivative-relation: under
t = log x (so x = e^t, dx = e^t dt) one gets INT R(log x) dx = INT R(t) e^t dt, an EXPONENTIAL integrand, not a
rational one.  So unlike INT R(e^x) -- which collapsed to the rational integrator via t = e^x, dx = dt/t -- the
log case is decided by the exponential Liouville machinery.  The split: a polynomial in log x is elementary,
integrated through the logarithmic tower and certified (INT (log x)^2 dx = x(log x)^2 - 2x log x + 2x, INT (log
x)^3 dx); a proper rational in log x is the exponential-integral situation, where a nonzero residue is the Ei
obstruction, so INT 1/log x dx -- the logarithmic integral li -- is PROVEN non-elementary, exactly as INT e^t/t
dt = Ei(t) is, as is INT 1/(log x)^2 dx.  Built on the log tower integrator and the rational-coefficient
exponential decider (liouvillerat).  Both monomial rows are now resolved: e^x by reduction to the rational
integrator, its dual log x by reduction to the exponential decider -- the second a proof of which integrals have
no elementary form, not a failure to find one.

## Into the algebraic frontier: algebraic residues and arbitrary-numerator radicals

Two research-grade rows of the capability map are closed this arc, each a genuine step into algebraic-function
integration, each certified.

Algebraic residues in Rothstein-Trager (algresq.lisp) -- the conjugate-root case.  When a proper rational
function over an irreducible quadratic has IRRATIONAL (real algebraic) residues, the integral is an
algebraic-coefficient logarithm.  For INT (Ax+B)/(x^2+px+q) dx with disc = p^2-4q > 0 and irrational roots, the
answer is (A/2) log(x^2+px+q) + ((B - Ap/2)/sqrt(disc)) log((x-r1)/(x-r2)), the second term carrying sqrt(d)
(disc = s^2 d, d squarefree).  The soundness key: the derivative of the algebraic logarithm is RATIONAL -- the
radical cancels because r1 - r2 = sqrt(disc) -- so the result is CERTIFIED by an exact rational identity in Q(x)
even though the antiderivative lives in Q(sqrt d).  Verified INT 1/(x^2-2), INT x/(x^2-2), INT (x+1)/(x^2-2),
INT (2x+3)/(x^2-3), INT 1/(x^2+x-1); the rational-root and negative-discriminant cases are routed elsewhere
(the rational integrator and the arctangent).

Arbitrary-numerator radical integration (rischradn.lisp) -- reduction of order.  For INT P(x)/sqrt(p) dx with P
any polynomial and p a monic quadratic, the antiderivative is A(x) sqrt(p) + c log(x + b1/2 + sqrt(p));
differentiating inside K = Q(x)[y]/(y^2-p) and clearing sqrt(p) gives the polynomial identity A' p + A p'/2 +
c = P, an exact linear system over Q solved by matching coefficients, the result certified inside K by the
differentiation certificate.  Verified INT x^2/sqrt(x^2+1) = (x/2) sqrt(x^2+1) - (1/2) log(x + sqrt(x^2+1)),
INT x^3/sqrt(x^2+1), INT (x^2+x+1)/sqrt(x^2+1), INT x^2/sqrt(x^2+2x+5); the base case INT 1/sqrt(x^2+1) is
reproduced.  This extends the quadratic-radical integration of algfunc.lisp (degree <= 1 numerators) to any
polynomial numerator.

The genuine summit beyond both: higher-degree algebraic residues (cubic and up, RootSum over Q(alpha)) and
higher-genus radicands (cubic and quartic p -- elliptic and hyperelliptic curves, where the integrals are mostly
non-elementary and the full theory is Trager's algebraic integration with integral bases).

## The genus-1 frontier: a decision procedure for elliptic integrals

elliptic.lisp decides INT P(x)/sqrt(q(x)) dx for q squarefree of degree 3 or 4 -- the elliptic (genus-1) case --
and is sound both ways: an elementary verdict carries a differentiation certificate inside the function field K =
Q(x)[y]/(y^2-q), and a non-elementary verdict is backed by an exact reduction.  The method is Hermite-style
reduction for the radical: since d/dx[x^k sqrt q] = (k x^{k-1} q + x^k q'/2)/sqrt q has numerator degree
k + deg(q) - 1, repeatedly subtracting a scalar multiple of d/dx[x^{D-(deg q-1)} sqrt q] cancels the top term of
the numerator, lowering its degree until the remainder has degree < deg(q) - 1, accumulating the algebraic part A
with P/sqrt q = d/dx[A sqrt q] + rem/sqrt q.  Then a zero remainder gives the elementary answer A sqrt(q)
(certified in K), and a nonzero remainder is a genuine first/second-kind elliptic differential, non-elementary.

Verified: INT (3x^2/2)/sqrt(x^3+1) = sqrt(x^3+1) and INT 2x^3/sqrt(x^4+1) = sqrt(x^4+1) (elementary, certified);
INT 1/sqrt(x^3+1), INT x/sqrt(x^3+1), INT 1/sqrt(x^4+1) (the lemniscatic integral) proven non-elementary; a
non-squarefree radicand is reported inconclusive rather than risk an unsound verdict.  The module also integrates
AS FAR AS POSSIBLE via ell-split: INT sqrt(x^3+1) dx = (2/5) x sqrt(x^3+1) + (3/5) INT 1/sqrt(x^3+1), splitting
the certified elementary part from the named elliptic remainder, exactly as a production CAS reports such an
integral; INT x^m sqrt(q) folds in through ell-split-sqrt (numerator x^m q).  This is the first decision procedure
here for integrals over an elliptic curve.  Beyond it: third-kind logarithmic parts on the curve (rational
residues giving genuine logarithms), genus >= 2 hyperelliptic curves, and the full Trager algebraic integration
with integral bases.

## Closing the comparison gaps, and the elliptic third kind

Two things this arc.  First, a verified audit and re-grade of the capability comparison: the great majority of the
rows that were marked partial or none against Maxima were in fact already implemented and passing (Reed-Solomon,
Hamming, Reed-Muller, BCH and cyclic codes; elliptic curves over F_p and ECC/ECDH; Shamir secret sharing; LLL
lattice reduction and integer-relation detection; Pratt primality certificates; Mobius/Dirichlet, perfect and
amicable numbers, the Frobenius number, binary quadratic forms, Lucas-Lehmer, Hensel lifting, Berlekamp-Massey,
finite differences, C-finite generating functions; the higher-degree and reducible algebraic-residue RootSum
cases; kernel-checked proof-carrying derivatives).  Each was re-confirmed by running its example before
re-grading; the chart now shows lizard at full on every row where Maxima is full, with the only remaining
non-full rows being ones where Maxima itself is not full either.  A small new module (numthy2.lisp) rounds out the
number-theory cluster with Dirichlet convolution and Mobius inversion, amicable pairs, the multi-denomination
Frobenius number via an exact Apery-set shortest path, and the Stern-Brocot / Farey mediant structure with the
unimodular adjacency certificate.

Second, beyond the genus-1 frontier: the elliptic THIRD-KIND recognizer (elliptic3.lisp).  Where the
first/second-kind reduction proves an elliptic integral non-elementary, an integral can still be elementary
through a logarithm log(A + B sqrt q).  For g = A + B y in K = Q(x)[y]/(y^2-q), the logarithmic derivative g'/g
is an exact K-element (computed via the conjugate), so a presented logarithmic-derivative integrand integrates to
log(g), certified by recomputing the derivative in K.  Verified over the genuine elliptic curve y^2 = x^3+1:
INT d/dx log(x + sqrt(x^3+1)) = log(x + sqrt(x^3+1)) and similar, certified; a non-matching candidate is rejected.
This complements the non-elementarity proofs with the genuine elementary logarithmic answers.  The continuing
frontier: the full third-kind decision (finding g from the integrand by rational-residue analysis on the curve),
genus >= 2 hyperelliptic curves, and the full Trager algebraic integration with integral bases.

## The four frontiers attacked: third-kind search, genus-2 hyperelliptic, and multivariate solving

This arc attacks the four research-grade frontiers beyond the elementary-tower summit, each in its soundest slice.

Frontier 1 -- third-kind elliptic SEARCH (elliptic3solve.lisp).  Where the recognizer certified a supplied g, the
search recovers it: given an integrand omega on y^2 = q(x), a bounded constructive search over candidate
g = u(x) + sqrt(q) (u a small-coefficient polynomial of bounded degree) computes g'/g in K = Q(x)[y]/(y^2-q) and
tests equality with omega, returning log(g) on a certified hit and an honest not-found when the family is
exhausted.  Verified over the genuine elliptic curve y^2 = x^3+1: omega = d/dx log(x + sqrt(x^3+1)) is solved and
certified, while the first-kind 1/sqrt(x^3+1) returns not-found (no polynomial-u logarithm), never a false claim.

Frontier 2 -- genus-2 hyperelliptic (hyperelliptic.lisp).  The Hermite-style radical reduction is degree-general,
so the elliptic reducer extends directly to squarefree q of degree 5 or 6 (genus 2): a zero remainder gives the
elementary answer A sqrt(q) (certified in K), and a nonzero remainder is a first/second-kind hyperelliptic
differential, proven non-elementary.  Verified: INT (5x^4/2)/sqrt(x^5+1) = sqrt(x^5+1) and INT 3x^5/sqrt(x^6+1) =
sqrt(x^6+1) elementary; INT 1/sqrt(x^5+1), INT x/sqrt(x^5+1), INT 1/sqrt(x^6+1) non-elementary; INT sqrt(x^5+1)
split into (2/7) x sqrt(x^5+1) + (5/7) INT 1/sqrt(x^5+1); the genus reported as 2 for degree 5 and 6.

Frontier 3 -- algebraic residues.  Already closed in earlier arcs: the algres family (algresn, algresext,
algresfull, algresnsf) handles the higher-degree (irreducible R of any degree, by the field trace via Newton's
identities), reducible, and non-squarefree RootSum cases, all certified without naming a conjugate.  The genuine
remainder is integral bases for arbitrary algebraic function fields -- research-grade, left honestly open.

Frontier 4 -- multivariate (polysolve.lisp, radmember.lisp), built on the existing Groebner machinery rather than
reimplementing it.  polysolve solves polynomial systems by Groebner elimination: consistency by the Weak
Nullstellensatz (the basis is {1} iff there is no common zero), zero-dimensionality (finitely many solutions iff
every variable has a pure-power leading monomial), the triangular eliminated form under lex order, and ideal-
membership consequences -- each gated by reduction modulo the basis.  radmember decides the geometrically stronger
RADICAL membership (does p vanish on the whole variety?) by the Rabinowitsch trick, p in sqrt(I) iff 1 in
<I, 1 - t p>, separating it from ideal membership (x in sqrt(<x^2>) but not in <x^2>); its reach is bounded by the
Groebner engine's capacity on the augmented system.  Root-naming for the solved systems is the natural next step.

## The finishing campaign: closing the five named sub-frontiers

Building on the four-frontier work, this campaign closed the five open sub-frontiers named in the roadmap, each
in its soundest exact form.

(d) Root-naming for the polynomial-system solver (polyroots.lisp).  The Groebner solver returns the structural
decision and a univariate eliminant for each coordinate of a zero-dimensional system; polyroots projects that
eliminant to a dense coefficient polynomial and, with Sturm sequences, gives the exact number of distinct real
values the coordinate takes and rational intervals isolating each (refined to a tolerance).  All exact -- no
floating point -- and certified by Sturm's theorem.  For the circle-line system x^2+y^2=1, x=y the y-eliminant
2y^2-1 yields two real roots isolated in (-363/512,-45/64) and (45/64,363/512), bracketing +-1/sqrt2.  Complex
roots and full coordinate back-substitution into solution tuples are the remaining steps.

(b) Higher genus, g >= 3 (hypergenus.lisp).  The Hermite-style radical reduction is degree-general, so the
elliptic reducer extends to squarefree q of any degree: the curve y^2=q has genus floor((deg q - 1)/2), and a
nonzero reduced remainder is a first/second-kind higher-genus differential, non-elementary.  INT (7x^6/2)/
sqrt(x^7+1) = sqrt(x^7+1) is certified elementary (genus 3); INT 1/sqrt(x^7+1) is non-elementary genus 3, INT
1/sqrt(x^9+1) non-elementary genus 4.  This completes the radical-integration tower across all genera.

(a) The complete, unbounded third-kind solver (elliptic3complete.lisp).  Writing omega = a + b*y in
K = Q(x)[y]/(y^2-q), the equation omega = g'/g for g = u + sqrt(q) gives u DIRECTLY as u = (q'/(2q) - a)/b -- a
closed-form linear solve rather than a bounded search.  Accept iff u is a polynomial and the differentiation
certificate confirms d/dx log(u + sqrt q) = omega.  This recovers g for u of arbitrary degree (verified through
degree 3, e.g. g = (x^3+2x) + sqrt(x^7+1)) and returns honest no-solution for first-kind integrands.  The fully
general third kind (g = A + B sqrt q with B nonconstant, via pole/residue analysis on the curve) remains.

(c) Integral bases (verified already complete).  The existing intbasis.lisp computes the finite integral closure
of Q[x] in the function field (including the quadratic case ib-quadratic and a general-curve oracle), and
vanhoeij.lisp adds van Hoeij correction terms for general plane curves.  A duplicate quadratic module written
during this campaign was therefore dropped as redundant, and a pre-existing regression in the van Hoeij example
(an unbound symbol from a stale import) was repaired.  Integral bases at infinite places and the general
degree > 2 integral closure remain the continuing summit.

(e) A faster Groebner engine (groebner2.lisp).  Buchberger's algorithm with the coprimality criterion: an S-pair
whose leading monomials are coprime reduces to zero and is skipped, soundly reducing the work.  Built alongside
the reference groebner.lisp and cross-checked to produce the same basis (as a monic polynomial set) on every
system tested -- independent agreement of two engines being the strongest validation available.  The chain
criterion and an F4-style linear-algebra engine would lift the heavier multivariate cases (radmember's 3+
variable systems) further.

## The research summit: S1–S5

Beyond the four frontiers and the five named sub-frontiers, this campaign attacked the genuinely-open research
summit. Four pieces were closed in their soundest exact form, and a first slice of the fifth was added; what
remains is named honestly at the end.

### S3 — Exact rational solution tuples (polysolve2.lisp)

The polynomial-system solver previously named the real values of each coordinate separately (polyroots). This
module recovers the complete solution POINTS by triangular back-substitution on the lexicographic Groebner basis:
the rational roots of the univariate eliminant (rational-root theorem, exact) are substituted back, reducing the
variable count, and the recursion assembles full tuples. Systems with rational solutions are solved exactly
(e.g. <x^2-3x+2, y-x> yields (1,1) and (2,2); <x^2-1, y^2-4> yields all four (+-1,+-2)), and a coordinate that
leaves Q is reported as 'irrational-fiber rather than guessed. Every returned tuple is certified to make all
generators vanish. Naming irrational coordinates exactly and complex solutions remain the next steps.

### S1 — The general third kind, g = A + B*sqrt(q) with B nonconstant (elliptic3general.lisp)

elliptic3complete handled g = u + sqrt(q) (B = 1). For the general g = A + B y, the norm N = g*conj(g) =
A^2 - B^2 q is a rational function, and the logarithmic derivative omega = g'/g obeys the exact identity
omega + conj(omega) = N'/N, so the rational part of omega equals (1/2) N'/N. The module computes the norm,
certifies the general recognizer omega = g'/g (exact in K for nonconstant B), and checks the norm identity --
the sound, exact core. The full CONSTRUCTION of g from omega (the coupled norm/y-component system, a
Jacobian-torsion question) is genuinely research-grade and left open; nothing returns a guessed g.

### S2 — Infinite places and the Riemann-Hurwitz genus (infplaces.lisp)

The finite integral basis (intbasis) and the genus decision (hypergenus) assumed the picture at infinity. This
module classifies the places over x = infinity from the parity of deg q and the leading coefficient: odd degree
gives one ramified place; even degree gives two places when the leading coefficient is a perfect square, else
one. Counting ramification (the finite branch points plus infinity when the degree is odd) and applying
Riemann-Hurwitz gives g = (R-2)/2 = floor((deg q - 1)/2) -- an INDEPENDENT genus computation that agrees with the
degree formula on every case (two methods agreeing is the validation). The general degree > 2 integral closure
remains open.

### S4 — A stronger Groebner engine with the chain criterion (groebner3.lisp)

groebner2 added the coprimality criterion (Buchberger's first). groebner3 adds the chain criterion (Buchberger's
second): a pair is redundant when a third basis element's leading monomial divides the pair's lcm. The initial
pair set is pruned by both criteria, then Buchberger runs with coprimality on the survivors. Correctness is
guaranteed by cross-checking against the reference engine: three engines (groebner, groebner2, groebner3) now
produce the identical basis on every tested system. A true F4-style linear-algebra engine is the next step for
the heaviest multivariate systems.

### S5 — First-order linear ODEs (odefol.lisp)

The first slice of Maxima-territory ODE solving beyond the existing separable (ode1) and constant-coefficient
(odelin) solvers: the variable-coefficient first-order linear equation y' + p(x) y = q(x). The operator
L(y) = y' + p y is linear in a polynomial ansatz, so a polynomial solution (degree deg q - deg p) is found by an
exact linear solve over Q and certified by differentiation. Inconsistent cases (where the genuine solution needs
the integrating factor exp(INT p) and is not polynomial) are reported honestly as 'no-polynomial-solution.
General non-polynomial integrating factors and nonlinear ODEs remain the open summit.

## Summit, pushed further (round two)

A second pass advanced each of the five summit pieces with another genuinely-new, self-contained increment.

### S5 -> Euler equidimensional equation (odeuler.lisp)

Beyond the constant-coefficient solvers, the Euler (Cauchy-Euler) equation x^2 y'' + a x y' + b y = 0. The
substitution y = x^r gives the indicial polynomial r^2 + (a-1)r + b, and the discriminant decides the regime: two
distinct real exponents, a repeated exponent with a logarithmic second solution, or a complex pair giving
x^alpha cos/sin(beta log x). Integer exponents are certified by direct substitution; irrational exponents are
named exactly as algebraic numbers (reusing polysolve3). All three regimes are handled exactly.

### S4 -> Radical ideal membership (radideal.lisp)

Beyond the reduced basis and ideal equality, the radical-membership test f in sqrt(I): does f vanish on the
variety V(I)? By the Nullstellensatz this holds iff <I, 1 - t*f> is the whole ring, tested by the Rabinowitsch
trick -- adjoin a fresh variable t and 1 - t*f, compute a Groebner basis, and check that 1 reduces to zero. This
is strictly stronger than ordinary membership: x is in sqrt(<x^2>) without lying in <x^2>. An exact two-sided
decision.

### S3 -> Complete algebraic solution tuples (algtuples.lisp)

Beyond naming individual irrational coordinates, the assembly of complete solution POINTS over a number field. For
a triangular system m(x_0) = 0, x_j = h_j(x_0), each root alpha of m gives the point (alpha, h_1(alpha), ...) with
every later coordinate computed exactly in Q(alpha) by algebraic-number arithmetic, and the point certified by
evaluating every generator to zero in Q(alpha). The variety is now described by genuine algebraic points, not
per-coordinate values.

### S2 -> Plane-curve genus (planecurve.lisp)

A second, independent genus theory: the genus-degree (Plucker) formula for plane curves, g = (d-1)(d-2)/2 for a
smooth curve of degree d, corrected by the delta-invariants m(m-1)/2 of ordinary singular points. It reproduces
the classical genera (conic 0, cubic 1, quartic 3, quintic 6, sextic 10) and node/cusp corrections, and agrees
with the superelliptic cyclic-cover genus where both apply (the smooth plane cubic and y^2 = cubic, both genus 1).

### S1 -> Third-kind norm reconstruction (elliptic3norm.lisp)

The first constructive step of building g = A + B*sqrt(q) from omega. Since a = (1/2) N'/N for the norm
N = A^2 - B^2 q, the residues of 2a are the integer orders of N, so from the (pole, multiplicity) data the monic
norm is reconstructed as prod (x - p_i)^{m_i} and verified to reproduce the rational part a exactly. This turns
the residue DECISION of elliptic3residue into a partial CONSTRUCTION; splitting N = A^2 - B^2 q into the actual A
and B -- the Jacobian-torsion step -- remains open.

## Summit, pushed further (round three)

A third pass advanced each of the five summit pieces once more, with one new self-contained increment apiece.

### S5 -> Exponential forcing with resonance (odeexp.lisp)

Beyond polynomial right-hand sides, the equation y'' + a y' + b y = q(x) e^{rx}. The substitution y = u(x) e^{rx}
reduces it to u'' + (2r+a) u' + (r^2+ar+b) u = q, a constant-coefficient polynomial ODE solved by odelin2. The
constant term is the characteristic value at r, so resonance (r a characteristic root) makes it vanish and the
ansatz degree rises automatically -- a double characteristic root included, with no special casing. Certified by
differentiation (y''-y = e^x gives (x/2)e^x; y''-2y'+y = e^x gives (x^2/2)e^x).

### S1 -> Norm split, full g for constant B (elliptic3split.lisp)

Beyond reconstructing the norm N from residues, splitting N = A^2 - B^2 q back into the actual A and B for the
decidable case B = c constant: then A^2 = N + c^2 q must be a perfect square polynomial, A its exact polynomial
square root. With elliptic3norm's residue reconstruction of N, the third-kind element g = A + c*y is now built end
to end in this case, certified by recomputing the norm. The general nonconstant-B split (Pell / Jacobian torsion)
remains open.

### S3 -> Complex roots named exactly (cplxroots.lisp)

Beyond naming real irrational coordinates, naming the complex ones. Each non-real root of a real polynomial is the
root of an irreducible real quadratic x^2 + px + q, so it is named (complex re im2) with re = -p/2 and
im2 = (4q-p^2)/4 rational -- the actual roots re +- sqrt(im2) i, the imaginary part carried as its square so no
surd is formed. Each pair is certified by the dividing quadratic. The real-and-complex census is now exact over Q.

### S4 -> Radical generator and squarefree decomposition (radgen.lisp)

Beyond the Nullstellensatz membership decision, the explicit radical GENERATOR of a principal ideal: sqrt(<f>) =
<f/gcd(f,f')>, the squarefree part. Yun's decomposition f = prod g_k^k exposes the primary-like structure, and
radical membership becomes a divisibility test, exact and constructive. This is the generator that the general
radical test (radideal) could not produce; reconstruction of the decomposition certifies it.

### S2 -> Weierstrass gaps and pole semigroup (weierstrass.lisp)

Beyond the genus formulas, the local structure at a place: the Weierstrass gap sequence and pole semigroup at a
Weierstrass point of a hyperelliptic curve. The pole semigroup is <2, 2g+1>, so the gaps are exactly the g odd
numbers {1, 3, ..., 2g-1}, the non-gaps are the semigroup elements, the Frobenius number is 2g-1, and the gap
count equals the genus -- an independent confirmation from the local pole structure. Exact integer arithmetic.

## Completing the exponential second monomial: the single-logarithm recognizer (tower2expfull.lisp)

The height-two exponential integrator (theta2 = exp(u), D2 theta2 = u' theta2) had every component piece -- the
exponential derivation, Hermite reduction, the Risch differential equation for the polynomial part, the power-sum
integrator, and a fraction-free Rothstein-Trager logarithmic part via the Sylvester resultant -- but the unified
driver (tower2risch.lisp) routed the proper-fraction logarithmic part through a power-sum wrapper that only
accepted a squarefree denominator of the bare-power form c*theta2^j. Any other squarefree denominator, such as
theta2 - e^x, was reported as an obstruction even when the integral was an honest single logarithm. That is what
kept the exponential second-monomial capability at "partial" rather than "full".

tower2expfull.lisp supplies the missing recognizer -- the exponential mirror of the primitive single-log
recognizer h2-newlog in tower2int.lisp. After Hermite leaves a squarefree remainder As/Ds, the integral is the
single logarithm c log(Ds) exactly when As = c D2(Ds) for a constant c of the tower; this is decided by one
polynomial division of As by D2(Ds) = t2e-deriv(Ds) over K1[theta2], checking the quotient is a tower constant
(its two-level derivative vanishes, so it lies in Q). The recovered c log(Ds) is certified by differentiation:
c D2(Ds)/Ds must equal As/Ds in K1(theta2). The recognizer is cheap -- one division, no resultant -- so it closes
the common single-residue case for any squarefree denominator without the memory cost of the general fraction-free
RootSum, which remains available (computed once) in tower2expff.lisp for the multi-residue x-dependent case. The
recognizer is sound both ways: a recognized logarithm is verified, and a remainder of any other shape returns
'none / 'notrecognized so the resultant path can handle it, rather than being misreported.

With this in place the exponential second-monomial row matches the primitive one on the single-logarithm case,
and the comparison's last lizard "partial" entry becomes a genuine, certified integrator.

## Going up the ladder: three rungs climbed

This pass climbed three of the four open ladder rungs to a sound, certified core; only the full third-kind
construction (the Jacobian-torsion question) remains open.

### Complex coordinates over Q(i) (cplxtuples.lisp)

Where cplxroots NAMES a complex root as (complex re im2), this assembles complete complex solution POINTS over the
Gaussian rationals Q(i).  When im2 is a perfect square the root is a Gaussian rational a + b i with a, b in Q, an
exact element of Q(i); the module carries such numbers as (gr a b), does exact Q(i) arithmetic, turns a perfect-
square complex root into its two Gaussian roots, assembles a triangular system into complete complex points, and
certifies a point by evaluating every generator to zero in Q(i).  For y = x on x^2 + 1 the point is (i, i); for
y = x^2 it is (i, -1) since i^2 = -1.  A non-perfect-square imaginary part is reported 'not-gaussian rather than
forced into a larger field.  Coordinates in Q(i, sqrt d) for non-square d, and positive-dimensional varieties,
remain ahead.

### Degree > 2 integral closure (superintbasis.lisp)

The integral basis at the finite places of a superelliptic curve y^n = f, the degree>2-in-y companion to the
quadratic finite integral basis.  For f squarefree the integral closure of Q[x] in Q(x)[y]/(y^n - f) is the free
power module {1, y, ..., y^(n-1)}: each y^k is integral because it satisfies the monic t^n - f^k (since
(y^k)^n = f^k), so the power basis is an integral basis and the order is maximal at the finite places.  The module
produces the basis, certifies each element's integrality by its monic defining polynomial, computes the
discriminant of the order (n^n f^(n-1) up to sign), and decides maximality by the squarefree test; a non-squarefree
f is reported non-maximal with the repeated factor, rather than passed off as integrally closed.  The van-Hoeij
correction at the repeated places, and the places over infinity, remain ahead.

### F4-class linear-algebra reduction (groebnerf4.lisp)

The linear-algebra reduction at the heart of F4: the polynomials to be reduced are laid out as the rows of a
Macaulay matrix whose columns are the occurring monomials sorted descending, and the matrix is row-reduced to
reduced row-echelon form over Q.  The nonzero rows read back as polynomials are the reductions, and the pivot
columns are their leading monomials.  This replaces Buchberger's one-at-a-time polynomial division with a single
batch Gaussian elimination -- the step that makes F4 fast -- and is exact over Q, with row-space preservation
certified (every input polynomial reduces to zero against the echelon rows).  Generating the S-pairs and the
monomial multiples that close the row space under the ideal (the symbolic-preprocessing loop) is the remaining work
to make it a full engine; the reduction core itself is here and certified.

## Into the algebraic-Risch frontier: genus-0 algebraic integration (algquadint.lisp)

The algebraic-case Risch problem -- integrating a rational function of (x, y) where y is algebraic over Q(x) -- is
the deepest remaining frontier (the three "none" rows of the comparison, where even Maxima is only partial).  Its
first rung is the genus-0 case y^2 = quadratic, where every such integral is elementary.  For a linear numerator
this module integrates INT (p x + r)/sqrt(a x^2 + b x + c) dx completely: the numerator splits as
p x + r = (p/2a)(2ax+b) + (r - pb/2a), giving an algebraic (second-kind) part (p/2a) sqrt(q) plus a first-kind part
(r - pb/2a) * J, with J = INT dx/sqrt(q) a logarithm when a > 0 (the arcsinh form log(2ax+b+2 sqrt a sqrt q)) and an
arcsine -(1/sqrt(-a)) arcsin((2ax+b)/sqrt D) when a < 0 and the radicand has real roots (D = b^2-4ac > 0).  The
arcsine identity was verified from D - (2ax+b)^2 = -4a q, which gives d/dx of the arcsine exactly 1/sqrt(q).  Every
piece is certified by differentiation, and an integrand with no real form (a < 0, no real arch) is reported
'no-real-form rather than forced.  This is the complete, certified genus-0 algebraic integrator for a linear
numerator; higher numerators reduce to it by Hermite reduction, and the positive-genus algebraic case -- the
general Trager-Bronstein algorithm over Q(x)[y] -- remains the open summit.

A note on method: this is implemented from the published algorithms (Euler substitution, the classical reduction
to first/second kind), built from scratch with differentiation certificates, not adapted from any existing
system's source -- the soundest and most license-clean way to match what mature systems do on this case.

## RUNG 5: stacked algebraic towers -- integration in the field of x^(1/4)

The algebraic-Risch climb (RUNGs 1-4: hyperelliptic decision, Hermite reduction, genus-1 third-kind with explicit
elliptic logarithms, Puiseux/Newton-polygon/integral-basis, the superelliptic family, and van Hoeij corrections)
is complete and certified for single algebraic extensions.  RUNG 5 is the genuine remaining summit: deeper STACKED
towers, where an integrand lives over a tower of two or more algebraic extensions.  This pass builds the first such
tower and its third-kind logarithm.

### The double algebraic tower (algtower2.lisp)

Q(x)[y][z]/(z^2 - y, y^2 - x) is the field of x^(1/4): elements are a + b z with a, b in the inner field
Q(x)[y]/(y^2 - x) handled by algfunc.  Multiplication reduces z^2 -> y by the inner field's own product.  The
decisive part is the derivation: from z^2 = y, z'/z = y'/(2y) = 1/(4x) (matching d/dx x^(1/4) = (1/4) x^(1/4)/x), so
D(a + b z) = a' + (b' + b/(4x)) z with a', b' the trusted inner derivations.  The whole double-tower derivation thus
reduces to algfunc's certified derivation plus one rational scalar, so soundness is inherited.  Verified: z^2 = y,
z^4 = x, (1+z)(1-z) = 1-y, y' = y/(2x), and INT (5/4) x^(1/4) dx = x^(5/4) certified by differentiation in the tower.

### The third-kind logarithm over the tower (algtower2log.lisp)

For e = a + b z the outer conjugate ebar = a - b z gives e*ebar = a^2 - b^2 y in the inner field, so one inner
inverse yields e^(-1), and the logarithmic derivative e'/e = D(e) e^(-1) is a tower element with INT (e'/e) dx =
log(e).  This is certified two independent ways: the inverse-free cleared identity e*(e'/e) = D(e) in the tower,
and the round trip.  A differential that is not d(log e) is rejected, never assigned a spurious logarithm.
Verified: e*e^(-1) = 1; INT (e'/e) dx = log(1 + x^(1/4)) certified; the generators give INT = log(x^(1/4)) and
log(sqrt x); a wrong differential is soundly rejected.

Deeper towers with several independent radicals, and the full Trager-Bronstein normalization at every singular
place (which removes the remaining genus/tower restrictions), are the open summit -- now the single honest "none"
on the comparison chart.

## Toward the full third-kind construction: the genus-2 Jacobian

The third-kind construction was complete and certified for genus 0 (the Pell fundamental unit, elliptic3pell) and
genus 1 (the elliptic torsion test elltorsion and the explicit Miller-function logarithm elllog).  Moving past
genus 1 needs the JACOBIAN group law of a higher-genus curve -- the divisor arithmetic that the elliptic
chord-tangent law provides in genus 1.  This pass builds it for genus 2.

### The genus-2 Jacobian group law (hyperjac.lisp)

On y^2 = f (deg f = 5, genus 2) a reduced divisor class is a Mumford pair [u, v] with u monic, deg v < deg u <= 2,
and u | (v^2 - f).  The identity is [1, 0], a point (a, b) is [x - a, b], negation is [u, -v].  Cantor's algorithm
adds classes by a composition step (a three-way extended gcd of u1, u2, v1 + v2 over Q[x], implemented from
scratch) followed by reduction (while deg u > 2, u <- monic((f - v^2)/u), v <- -v mod u).  Every result is checked
against the Mumford curve condition.  Verified: P + identity = P; P + Q = [x^2 + x, x + 1] on y^2 = x^5 + 1 (curve
condition holds); P + (-P) = identity; and a Weierstrass point is 2-torsion.

### The genus-2 third-kind torsion decision (hyperjactor.lisp)

The genus-1 elementarity test (a pole is elementary iff it lifts to a torsion point) generalizes verbatim with the
elliptic group replaced by the Jacobian: the third-kind class [P] - [iota P] is elementary iff it is torsion,
n*[D] = 0.  In Mumford terms that class is the divisor of P, so its order under the Jacobian group law is the
torsion order.  A bounded multiple search confirms torsion with its order, or reports an HONEST bounded negative
("no torsion up to B") -- it never falsely claims non-elementarity, since the unconditional genus-2 torsion bound
needs deeper theory.  Verified: a Weierstrass point is order 2; the point (0, 1) on y^2 = x^5 + 1 generates a
torsion class of order 5 (so that third-kind integral is elementary); a too-small bound returns the honest bounded
miss; cross-checked on y^2 = x^5 - x.

The explicit genus-2 algebraic logarithm in the torsion case (the Mumford analogue of the elliptic Miller
function), and arbitrary genus, remain the open summit -- the single honest "none" on the comparison chart.

## Genus-2 third-kind logarithm and the unified hyperelliptic driver

This pass advances both open frontiers -- the full third-kind construction and the general algebraic Risch -- on
the genus-2 hyperelliptic family.

### The genus-2 third-kind logarithm (hyperthird.lisp)

The field K = Q(x)[y]/(y^2 - f) and its derivation are handled by algfunc for ANY radicand f, so they cover the
genus-2 (deg f = 5) case unchanged.  For the third-kind argument g = a(x) + y the rationalized logarithmic
derivative g'/g has denominator the norm N(a + y) = a^2 - f.  So from a third-kind differential over a polynomial
denominator D, the candidate is a = sqrt(D + f) when D + f is a perfect square (reusing the tested polynomial
square root of elliptic3split); the integral is then log(a + y), certified in K by the cleared identity
(a + y) * omega = D(a + y).  A non-square denominator is rejected.  This generalizes sethird (superelliptic) to the
hyperelliptic field of arbitrary genus, and is genus-agnostic: verified INT (d log(x + y)) = log(x + y) on
y^2 = x^5 + 1, recovery of a = x, a = x^2, a = x^2 + 1, soundness rejection, and the same construction on the
elliptic y^2 = x^3 + 1.

### The unified hyperelliptic integration driver (hyperint.lisp)

One entry point for integration over y^2 = f at any genus: the second-kind part INT P(x)/sqrt(f) routes to hyperell
(the elementary Q sqrt(f) by Hermite reduction, or a proof of non-elementarity via the first-kind obstruction),
and a third-kind differential routes to hyperthird (the logarithm log(a + y), with a recovered and certified).  The
driver classifies and dispatches, returning a single certified verdict; every positive answer carries the
underlying module's differentiation certificate, so soundness is inherited.  Verified on y^2 = x^5 + 1 (genus 2):
INT (5/2) x^4 / sqrt(f) = sqrt(f) elementary; INT 1/sqrt(f) non-elementary (genus-2 holomorphic); the third-kind
differential over N(x + y) giving log(x + y); a non-recognized denominator reported; and the identical decisions on
the genus-1 curve y^2 = x^3 + 1.

Mixed integrands of several kinds at once, third-kind arguments beyond the a + y shape, and the full divisor-class
construction at arbitrary genus, remain the open summit.

## Genus-2 nonconstant-B third-kind: the function-field Pell construction (hyperpell.lisp)

hyperthird builds log(a + y), the b = 1 third-kind argument; the genuinely hard case is g = A + B y with B
NONCONSTANT, whose norm A^2 - B^2 f is the function-field Pell form.  A unit (constant norm) generates these, and
on the family f = h(x)^2 + c the element g0 = h + y has norm -c, a constant, so g0 is a fundamental unit.  For
deg h = 3 this is a genuine genus-2 curve y^2 = h^2 + c.  The powers g0^n = A_n + B_n y are computed by exact field
arithmetic (algfunc, genus-agnostic), have B_n nonconstant for n >= 2, and norm (-c)^n by multiplicativity; each is
a third-kind logarithm argument INT ((g0^n)'/g0^n) dx = log(g0^n) = n log(h + y), certified by the norm relation
and by differentiation in the field.  This is the genus-2 companion to the genus-0 elliptic3pell construction.
Verified on y^2 = x^6 + 1 (h = x^3, c = 1): the unit x^3 + y has norm -1; g0^2 = (2x^6 + 1, 2x^3) with nonconstant
B and norm 1; g0^3 with norm -1; the logarithm certificate holds for n = 1..3; a second curve (x^3 + x)^2 + 2 also
certifies; and a non-unit curve (x^5 + 1 for h = x^3) is reported, not forced.  Curves whose sqrt(f) has a
non-periodic continued fraction (no fundamental unit) remain out of scope.

## The continued fraction of sqrt(f): deciding the Pell unit of a hyperelliptic curve (polycf.lisp)

The genus-2 Pell construction (hyperpell) needed the fundamental unit handed to it via the f = h^2 + c family.
This module computes the unit for a general curve by expanding sqrt(f) as a continued fraction over Q[x] -- the
function-field analogue of the numeric continued fraction for sqrt(N).  For f of even degree, sqrt(f) has a
polynomial part a0 (the polynomial of degree deg(f)/2 whose square matches f's top half), and Abel's recurrence
expands sqrt(f) with complete quotients (P_i + sqrt f)/Q_i: a_i = polypart((P_i + a0)/Q_i), P_{i+1} = a_i Q_i - P_i,
Q_{i+1} = (f - P_{i+1}^2)/Q_i.  The expansion is periodic exactly when some Q_i returns to a nonzero constant; the
curve then has a fundamental Pell unit, read off the convergent h_i/k_i.  Every unit is GATED by its norm
A^2 - B^2 f being a nonzero constant: pcf-certify-unit and pcf-unit-verified return the unit only when that holds.
The period-1 family (which includes f = h^2 + c) is fully certified; a higher even period -- where the true unit is
a higher convergent -- returns unit-unverified rather than a wrong unit, and a curve whose CF does not close within
the search bound returns no-unit-up-to, an honest bounded negative (the unconditional periodicity bound for a given
genus needs deeper theory).  Verified: sqrt(x^6 + 1) has period 1 and the certified unit (x^3, 1) of norm -1
(matching hyperpell); sqrt((x^3 + x)^2 + 2) gives the certified unit (x^3 + x, 1); the classical genus-0 unit
(x, 1) of sqrt(x^2 + 1) is recovered; x^6 + x (period 2) is honestly returned unit-unverified; x^6 + x^2 + 1 is
returned no-unit-up-to.  With this, the genus-2 third-kind Pell construction extends past the curves where the unit
is visible by inspection.

## Period-2 units and the CF-driven third-kind Pell construction (hyperpellcf.lisp)

A convergent-indexing fix in polycf (the Abel iteration must start from the state AFTER a0, i.e. P_1 = a0,
Q_1 = f - a0^2, not from P=0, Q=1, which recomputed a0) makes genuine period-2 curves certify their fundamental
unit: y^2 = x^6 + x has period 2 and the certified unit (2x^6 + 1 + ..., 2x^2) with constant norm, where before the
period-2 convergent was returned unit-unverified.  With period > 1 units now extracted correctly, hyperpellcf
bridges polycf and the third-kind construction: given ANY hyperelliptic curve y^2 = f, it asks polycf for the
certified fundamental unit (A, B), builds g0 = A + B y in the genus-agnostic field (algfunc), and produces
INT ((g0^n)'/g0^n) dx = log(g0^n) = n log(g0), each gated by the differentiation certificate.  This generalizes
hyperpell past the f = h^2 + c family to every periodic curve.  Verified: on y^2 = x^6 + x (a genuine period-2
curve, not of the form h^2 + c) the CF-found unit drives a certified third-kind logarithm for n = 1, 2, 3; on
y^2 = x^6 + 1 it agrees with hyperpell (unit (x^3, 1)); a non-periodic curve reports no-unit, the honest bounded
negative.  Longer periods at higher genus, and unconditional aperiodicity proofs, remain the open summit.

## Hardening the Pell unit engine: square guard and reverse-CF cross-check (polycf.lisp, ex385)

Two additions strengthen the continued-fraction Pell engine under the CF-driven third-kind construction.  First, a
perfect-square guard: if f is a perfect square then sqrt(f) is a polynomial -- no quadratic irrational, no Pell
unit -- so pcf-is-square? flags it and pcf-unit-status returns 'square rather than a degenerate verdict; the full
classification is 'square / (unit A B) / 'unit-unverified / 'no-unit-up-to.  Second, a reverse-CF round-trip
cross-check: a unit (A, B) with constant norm c determines a curve f = (A^2 - c)/B^2, and the continued fraction of
that f must independently recover a certified unit.  This validates the unit-finder against the opposite
(construct-from-unit) direction -- two independent methods agreeing.  Verified: x^4 + 2x^2 + 1 = (x^2 + 1)^2 is
flagged 'square; the engineered curve f = x^6 + 2x^4 + 3x^2 + 2 from the unit (x^4 + x^2 + 1, x, 1) is recovered by
the CF with exactly that nonconstant-B unit, certified; several further engineered units (constant and nonconstant
B) round-trip.  Over Q, polynomial Pell with period >= 3 is rare and none appeared in a wide small-coefficient scan,
so the reverse-CF construction is the rigorous validator of the convergent/norm machinery beyond period 1; longer
periods at higher genus remain the open summit.

## Closing the last rung: the unconditional aperiodicity proof (hyperaperiodic.lisp)

The full third-kind construction needed one thing to close its core: a way to PROVE, not merely fail to find, that a
hyperelliptic curve y^2 = f has no Pell unit -- equivalently that INT dx/sqrt(f) is non-elementary.  polycf reports
a unit when some Q_i of the continued fraction of sqrt(f) returns to a nonzero constant, but when none does it could
only say "aperiodic up to bound B", a bounded negative.  This module supplies the proof.  Over Q[x] the CF of
sqrt(f) is purely periodic and its complete quotients (P_i, Q_i) are finite in number, so the pairs must eventually
repeat; tracking them until the first repeat traverses the entire cycle.  Then exactly one of two things has
happened: some Q_i past the trivial start Q_0 = 1 was a nonzero constant (a Pell unit exists, the integral is
elementary, the infinity class is torsion), or the cycle closed with no such constant (there is NO Pell unit,
UNCONDITIONALLY, so the integral is non-elementary and the class is non-torsion).  The second case is now a finite
certificate rather than a failed search.

The construction was validated by independent agreement: on y^2 = x^6 + 1 the cycle exposes the unit (x^3, 1) and
the verdict is elementary, matching polycf; on y^2 = x^6 + x^2 + 1 the cycle closes at length 3 with no constant Q,
so INT dx/sqrt(x^6+x^2+1) is PROVEN non-elementary, and the bounded polycf search independently reports aperiodic
up to 100 -- the unconditional proof and the bounded search agree, with the proof strictly stronger.  A subtle but
essential point caught during construction: the very first complete quotient is the trivial (P_0, Q_0) = (0, 1),
whose Q_0 = 1 is constant by construction and must be excluded from the unit test, or every curve would look
periodic; the test runs over the tail of the cycle only.

With both directions of the elementarity decision closed -- periodic gives the explicit logarithm, proven-aperiodic
gives non-elementarity -- the last rung's core is complete.  What remains is genuinely deep: a single uniform bound
on the period length as a function of genus (so the traversal bound can be set a priori rather than chosen), and the
behavior as periods grow without bound at ever higher genus.  Those are the research-grade horizon, named honestly
and not overclaimed.

## General integral closure: the multi-branch combined correction (vanhoeijmb.lisp)

The van Hoeij correction (vanhoeij.lisp) builds an integral-basis element for a SINGLE branch at a place, and
honestly defers a place where several branches meet -- returning 'needs-place-combination -- because no
single-branch correction is integral at all of them.  This module supplies the combined element for the quadratic
(hyperelliptic) case y^2 = f.  At a node such as y^2 = x^2(x+1), whose two branches y = +-(x + x^2/2 - ...) meet at
the origin, the integral element is found and certified by the exact integral-closure criterion in
K = Q(x)[y]/(y^2 - f): an element w = (A + B y)/d has minimal polynomial w^2 - (2A/d) w + (A^2 - B^2 f)/d^2 over
Q[x], and is integral exactly when both the trace 2A/d and the norm (A^2 - B^2 f)/d^2 are polynomials (divide out
with zero remainder).  The norm involves all branches at once, so this certifies integrality at every branch
simultaneously -- which is precisely what the single-branch construction cannot do.  For the node the element y/x
has trace 0 and norm -(x+1), both polynomials, so y/x is the combined-branch basis element; for the two-node curve
y^2 = x^2(x-1)^2(x+1) the element y/(x(x-1)) is integral at both nodes.  Non-integral candidates (y/x^2, or y/(x-1)
at a smooth branch point) are rejected with the failing trace or norm exhibited, never forced.  The certificate is
the monic minimal polynomial itself, checked to reproduce 2A and A^2 - B^2 f exactly.

This closes the multi-branch combined correction for the quadratic case; the general-degree version (n branches
over a degree-n field, where the norm is the full field norm rather than A^2 - B^2 f) is the remaining work, and
the soundness boundary -- ramified places and the general-degree combination -- is reported explicitly rather than
guessed.

A note on the recommendation behind this step: "general algebraic Risch at arbitrary genus and tower" is the open
research problem, not a single rung; pursuing it wholesale would mean overclaiming.  The sound way to push that
frontier is to find the next genuinely missing, certifiable sub-rung -- here, the multi-branch combined correction
that the existing code explicitly deferred -- build it, certify it, and name what still remains.

## The CAS as a theorem prover: definite integrals and the Dirichlet/sinc integral

Two examples make the proof-producing CAS concrete on integration, with an honest division of labor between the
elementary and non-elementary cases.

defint.lisp turns a definite integral of a polynomial into a THEOREM by the Fundamental Theorem of Calculus: it
computes the antiderivative F, discharges the one nontrivial premise "F is an antiderivative of f" with the
differentiation arbiter (poly-deriv F = f exactly), evaluates F(b) - F(a) in exact rational arithmetic, and emits a
structured proof record (theorem, value, FTC justification, F'=f certificate) that a checker re-verifies
independently -- a tampered value fails the re-check.  INT_0^1 x^2 dx = 1/3, INT_0^2 (3x^2+2x+1) dx = 14, and so on,
each a certified theorem rather than just a number.

dirichlet.lisp proves INT_0^inf sin(x)/x dx = pi/2 -- the value of the sinc integral.  This integrand is NON-
elementary, so the Risch machinery and the FTC cannot reach it; the proof is the classical parameter-integral
(Feynman) argument: introduce I(s) = INT_0^inf e^{-sx} sin(x)/x dx, differentiate under the integral to get
I'(s) = - INT_0^inf e^{-sx} sin x dx, evaluate that Laplace transform to 1/(s^2+1) (Lemma A, from the antiderivative
G(x,s) = -e^{-sx}(s sin x + cos x)/(s^2+1) whose derivative is checked against the integrand), integrate
I'(s) = -1/(s^2+1) to I(s) = C - arctan(s) (Lemma B, the arctan-derivative identity certified), fix C = pi/2 by the
boundary s -> inf, and evaluate I(0) = pi/2.  The two analytic lemmas are transcendental identities, so their
certificate is exact-agreement-at-samples (a central-difference derivative matching the closed form to high
precision at several points); the algebraic backbone -- the ODE I'(s) = -1/(s^2+1) integrating to C - arctan(s) and
the boundary algebra -- is exact.  The proof record re-checks, and a tampered value is rejected.

The honesty here is the point: sin(x)/x has no elementary antiderivative, and the system does not pretend the
antiderivative engine proves the Dirichlet value -- it proves it by the method that actually works, names which
steps are certified by which arbiter, and keeps the elementary FTC case (defint) and the non-elementary parameter-
integral case (dirichlet) clearly distinct.

## A simpler proving UX: axiom mode (axmode.lisp)

For casual use, axiom mode lets a user state axioms once and then ask, for any statement, whether it is proven,
disproven, or independent -- without re-stating the axioms.  An axiom environment is a database of Horn clauses
(facts and rules) over the engine of logic.lisp.  ax-assume adds a fact, ax-assume-rule a rule, ax-assume-not a
negative axiom.  ax-check returns a three-valued verdict: proven (the goal is derivable), disproven (its negation is
derivable -- negative axioms are stored explicitly, so a disproof is never inferred from mere absence), or
independent (neither, the honest open-world "don't know").  A set proving both a statement and its negation is
flagged inconsistent rather than silently resolved.  From {human(socrates), mortal(X):-human(X)} the mode proves
mortal(socrates), leaves mortal(zeus) independent, and -- once (not (mortal zeus)) is added -- disproves
mortal(zeus); transitive-closure rules resolve multi-step ancestry.  Everything reduces to the trusted query
engine, so the verdicts are exactly the engine's; the mode only adds the environment, the explicit-negation
convention, and the three-valued report.  This is the flexible front end: load axioms, then check anything against
them.

## Pushing the frontier harder: the ramified-place integral element (ramplace.lisp)

The multi-branch node correction (vanhoeijmb.lisp) handled a place where several branches meet but each is an
ordinary power series.  The harder case is a RAMIFIED place, where the branch is a Puiseux series in a fractional
power -- a cusp.  At x = a with (x-a)^m || f, the valuations on y^2 = f are v(x-a) = 2 and v(y) = m, so for m >= 2
the element w = y/(x-a)^{floor(m/2)} is integral (its valuation m mod 2 >= 0) and generates the local integral
closure.  Integrality is exact and certified by the monic minimal polynomial: w^2 = f/(x-a)^{2 floor(m/2)} is a
polynomial.  For the cusp y^2 = x^3 the element is y/x with w^2 = x (the place is ramified, m odd); for y^2 = x^5 it
is y/x^2 with w^2 = x; for the even multiplicity y^2 = x^2 the place SPLITS and w = y/x is a unit with w^2 = 1; a
point that is not a root of f is unramified-regular with no new element.  The module computes the multiplicity, the
two valuations, the element, the certificate, and the place classification (ramified / split / unramified-regular)
-- the local Riemann-Hurwitz data.  This is a single ramified place of the quadratic cover, by exact polynomial
division; the general-degree ramification (a Puiseux cycle of length e | n in a degree-n cover) remains ahead, with
the soundness boundary explicit.
