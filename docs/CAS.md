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
