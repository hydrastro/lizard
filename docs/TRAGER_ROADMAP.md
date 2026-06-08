# Roadmap: the algebraic case (Trager-Bronstein + Puiseux), step by step

Goal: integrate r(x)/sqrt(p) (and ultimately r in any algebraic function field) the way FriCAS does --
deciding elementarity and producing the algebraic-logarithm answer or a proof of non-elementarity. This is a
MULTI-SESSION climb. Every rung must be certified by the differentiation arbiter and must honestly report
"not-handled" outside its proven scope. We never ship a guessed closed form (the probe in the previous session
showed the naive 3rd-kind log formula is wrong off the quadratic-at-origin special case).

## Where we are (DONE, certified)
- algfunc: the field K = Q(x)[y]/(y^2 - p), arithmetic + derivation D(u+vy)=u'+(v'+v p'/(2p))y, for ANY p.
- algfuncint: INT P(x)/sqrt(quadratic) for arbitrary polynomial numerator (genus 0), certified.
- hyperell: INT P(x)/sqrt(p), p squarefree any degree, POLYNOMIAL numerator. Hermite reduction extracts the
  elementary Q*sqrt(p) (second-kind) part; DECIDES non-elementarity via the first-kind remainder
  (INT dx/sqrt(x^3+1) proven non-elementary). Certified in K.

## The three kinds of differentials on y^2 = p (Bronstein Ch.5, Trager 1984)
- 1st kind (holomorphic): x^i/sqrt(p), i < g. NON-elementary. (hyperell decides this.)
- 2nd kind (poles, no residues): ELEMENTARY, = (algebraic part)*sqrt(p). (hyperell's Q*sqrt(p).)
- 3rd kind (simple poles WITH residues): elementary IFF the residue divisor is principal; answer has
  algebraic LOGARITHMS. THIS is the frontier.

## The rungs (each its own session, each certified)

RUNG 1 [THIS SESSION] -- residues of r(x)/sqrt(p) at finite places, and the all-residues-zero decision.
  - Extend the numerator to a proper RATIONAL function r(x) = A(x)/B(x) over sqrt(p).
  - Compute the residue of the differential r dx/sqrt(p) at each finite place (each root of B, and the
    branch behaviour). For a simple pole at x=s NOT a branch point (p(s)!=0): two places (s,+sqrt(p(s))) and
    (s,-sqrt(p(s))); residues are +/- A(s)/(B'(s) sqrt(p(s))). For a pole at a branch point (p(s)=0): one place.
  - DECISION delivered this rung: if ALL residues are zero, the differential is 2nd-kind, hence (after Hermite)
    elementary -- reduce and certify. Otherwise report ('third-kind <residue-data>) honestly (needs Rung 3).
  - Certify the zero-residue elementary answers in K by differentiation. Soundness: never claim elementary
    for a nonzero-residue case at this rung.

RUNG 2 [DONE, certified -- algherm.lisp] -- Hermite reduction for rational-function numerators (remove 2nd-kind poles of order >= 2).
  - Generalize hyperell's polynomial reduction to proper fractions: cancel higher-order poles by subtracting
    D(c(x) sqrt(p)) and D(c(x)/(x-s)^k sqrt(p)) pieces, leaving only SIMPLE poles + first-kind. Certified.

RUNG 3 -- the 3rd-kind logarithm via the residue divisor (the heart of Trager).
  - With simple poles and their residues (Rung 1) summed to a divisor D on the curve, decide whether D is
    principal: on genus 0 always; on genus >= 1 a TORSION test on the Jacobian. If principal, D = div(f) and the
    integral is sum residue_i * log(f) -- the algebraic logarithm. Certified in K. Else: NON-elementary, proven.
  - Sub-rung 3a [DONE, certified -- algthird.lisp]: genus 0 (the quadratic radical) -- always principal;
    closes INT dx/((x-s) sqrt(quadratic)) GENERALLY via the TANGENT-LINE log argument g=(y-L(x))/(x-s),
    L=rho+k(x-s), k=p'(s)/(2 rho) -- correct for shifted poles (the case the naive formula got wrong).
  - Sub-rung 3b [DONE, decision certified -- elltorsion.lisp]: genus 1 torsion test (elliptic) -- the real
    Trager divisor condition.  INT dx/((x-s)sqrt(cubic)) elementary <=> the pole lifts to a TORSION point on
    y^2=p (group law over Q, Nagell-Lutz+Mazur termination); INT dx/(x sqrt(x^3+1)) elementary (torsion order
    3), INT dx/((x-3)sqrt(x^3-2)) PROVEN non-elementary (infinite order). Decision delivered.
  - Explicit elliptic logarithm [DONE for odd-order torsion, certified -- elllog.lisp]: Miller function f_P
    (div n[P]-n[O]) over K, f=f_P/conj(f_P); INT dx/(x sqrt(x^3+1)) = (1/3) log((y-1)/(y+1)) CONSTRUCTED and
    certified. Gated by the differentiation certificate (never a wrong log). Even-order torsion poles (whose
    multiples hit a 2-torsion point) deferred -- Miller verticals at 2-torsion need refinement; decision retained.

RUNG 4 [STARTED -- puiseux.lisp: superelliptic Puiseux expansions DONE, certified] -- Puiseux expansions +
  integral basis for GENERAL algebraic functions. DONE so far: the Newton-Puiseux local expansion of y for
  y^e=g(x) at a place, with correct ramification index E=e/gcd(ord_0 g, e), power-checked (y^e=g verified);
  needs-radical reported honestly for irrational leading coeffs. DONE also: newton.lisp -- the NEWTON POLYGON for general F(x,y)=0,
  extracting every branch's leading exponent (slope) and leading coefficients (edge-polynomial roots); verified
  on nodes (two tangents), cusps, ramified and tacnode places, multiple distinct slopes, and smooth branches.
  DONE also: term-by-term general-F Puiseux (puiseuxg.lisp; the Catalan series recovered from y=x+y^2, all
  branches power-checked) and the INTEGRAL BASIS engine (intbasis.lisp): the Puiseux-valuation integrality test,
  the certified quadratic closure {1, y/g} for y^2=D, AND the local-basis assembly at a singular place
  (ib-local-basis-at0: {y^j/x^{k_j}} with each k_j maximal -- e.g. y^3=x^4 gives {1, y/x, y^2/x^2}, the nodal
  cubic gives {1, y/x} matching the quadratic g=x, a smooth place gives {1, y}). DONE also: the GLOBAL integral basis -- ib-global-basis-superelliptic finds all singular places (repeated
  roots of g), computes the local basis at each by shifting F(x+a,y), and combines into {y^j/d_j}, d_j=prod_a
  (x-a)^{k_j(a)} (e.g. y^3=x^4(x-1)^2 -> {1, y/x, y^2/(x^2(x-1))}, both basis elements certified integral since
  y^3=g). SOUNDNESS guard: a place with an irrational tangent (branches over an extension) returns
  needs-extension, never a wrong basis. DONE also: RECURSIVE Newton-Puiseux (puiseuxr.lisp) -- separates branches that SHARE a leading term, the
  multiplicity case the simple-root solver could not handle. Substitute y=(c+y1)x^mu at a root c of multiplicity
  m>1, deflate the common x-power, recurse on the new equation until roots are simple (with the F0=0 case
  peeling the exact branch y=0). Verified: the node (y~+-x), three simple tangents, and the double-tangent
  (y-x^2)(y-x^2-x^3) separating into y=x^2 and y=x^2+x^3. This supplies each branch its distinct leading-term
  sequence -- the input to the van-Hoeij correction terms. DONE also: SUPERELLIPTIC HERMITE REDUCTION (sehermite.lisp) -- the integration payoff for y^n=g. On y^n=g,
  D(x^k y^j) = (k x^(k-1) g + (j/n) x^k g_prime) y^j / g preserves the y^j power, so each sector reduces like
  the hyperelliptic case (p -> g, 1/2 -> j/n). INT (P y^j/g) dx = Q y^j + INT(S y^j/g) dx; S=0 gives an
  elementary answer Q y^j certified by Q_prime g + (j/n) Q g_prime = P, S nonzero reports the first-kind
  obstruction. Verified INT (3x^4+2x) y/(x^3+1) = x^2 y on y^3=x^3+1, the y^2 sector, the n=2 reduction
  matching hyperell, and INT y/(x^3+1) non-elementary. DONE also: the GENERAL-n SUPERELLIPTIC FIELD (sefield.lisp) -- Q(x)[y]/(y^n-g) for any n, generalizing
  algfunc (n=2). Elements are length-n rational-coefficient lists; multiplication reduces y^n->g, the
  derivation uses y_prime = g_prime y/(n g) in closed form per sector, and logarithms are certified by the
  cleared-denominator identity f*u = c*u_prime (no inverse needed). Verified y^3=g, the derivation, log g
  and log y certificates at n=3, and the n=2 derivation matching algfunc. DONE also: the field NORM, INVERSE, and rationalized logarithmic derivative (senorm.lisp). N(u) = product
  of conjugates = det of the multiplication-by-u matrix on {1,y,...,y^(n-1)} (cofactor expansion over Q(x),
  reusing field multiplication); the adjugate gives ubar with u*ubar = N(u), hence u^(-1) = ubar/N(u), and
  u_prime/u = u_prime*ubar/N(u) -- a field element over a polynomial denominator. Verified N(y)=g,
  N(x+y+2y^2) = the cubic norm form, y*y^(-1)=1, the rationalized log derivative, and n=2 matching a^2-b^2 g.
  DONE also: the SUPERELLIPTIC THIRD-KIND LOGARITHM (sethird.lisp) -- the Rothstein-Trager step over the
  field. CONSTRUCTIVE: for a field element u, INT (u_prime/u) dx = log u, with the rationalized differential
  (field numerator over the polynomial denominator N(u)) and the cleared certificate u*F = N(u)*u_prime.
  RECOGNIZER for u = a(x)+y (the common third-kind argument): N(a+y) = a^n + (-1)^(n+1) g, so from a
  differential denominator D the candidate a = nth-root(D - (-1)^(n+1) g); if an exact polynomial n-th power
  and it reproduces the differential, the integral is log(a+y), certified, else not-third-kind-a+y. Verified
  log(x+y) on y^3=x^3+1 (N=2x^3+1), recovery of a=x and a=x+1 from their denominators, soundness rejection,
  and the n=2 case. RUNG 4 is now COMPLETE for the superelliptic family y^n=g: Puiseux + Newton polygon +
  integral basis + branch separation + Hermite reduction + the field + Norm + third-kind log. DONE FINALLY: the VAN HOEIJ CORRECTION TERMS (vanhoeij.lisp) for non-superelliptic curves. A general-curve
  integral basis element is w_j = (y^j + sum_{i<j} c_{j,i}(x) y^i)/d_j, not a pure power; at a rational
  place x=a (Puiseux q=1) the branch is a power series and the correction c(x) = the part of the branch
  below order k, so (y-c)/(x-a)^k vanishes to order >=k and is integral (subtract the singular part).
  Integrality is certified by the general-F Puiseux valuation oracle; ramified places (q>1) or several
  branches needing a combined correction return needs-place-combination, never a guess. Verified (y-x)/x^2
  and (y-x-x^2)/x^3 integral on y=x+x^2+x^3 while y/x^2,y/x^3 are not, and the cusp y^2=x^3 honestly
  deferred. RUNG 4 IS NOW COMPLETE: the full local analysis, integral closure, and integration on algebraic
  curves -- superelliptic and general.

RUNG 5 (mixed transcendental-over-algebraic towers) -- NOW STARTING. The open summit: couple the recursive
  transcendental Risch driver (ntower/ntrisch) with the algebraic layer, so an integrand can be rational in
  x, an algebraic y, AND a transcendental like exp/log on top. Genuine FriCAS territory. STARTED: the exponential case (mixedexp.lisp) -- INT B exp(h)=A exp(h) iff
  A_prime+h_prime A=B in K, solved over the algebraic field; and the LOGARITHMIC primitive case
  (mixedlog.lisp) -- INT (P_1 t+P_0) dx = Q_2 t^2+Q_1 t+Q_0 in the tower K(t), t=log h, t_prime=h_prime/h,
  via the coupled system Q_2_prime=0, 2 Q_2 t_prime+Q_1_prime=P_1, Q_1 t_prime+Q_0_prime=P_0, every answer
  differentiate-certified. Verified INT ((1/(2 sqrt x)) log x + 1/sqrt x)=sqrt x log x and
  INT (log x)/x = (1/2)(log x)^2 on y^2=x. DONE also: both cases LIFTED TO GENERAL n over y^n=g (mixedexpn.lisp, mixedlogn.lisp) via sefield. The
  exponential case decouples into n independent scalar sector RDEs a_j_prime + ((j/n)g_prime/g + h_prime)
  a_j = B_j; the logarithmic case solves the tower system Q_2 t^2+Q_1 t+Q_0 with the field-coefficient
  sectors decoupling within each t-degree. Verified INT ((1+x^2/(x^3+1)) y) exp(x)=y exp(x), INT (...)=
  y log x on y^3=x^3+1, the y^2 sectors, the t^2 case, and n=2 subsuming the sqrt-field modules. DONE also: the ENTANGLED tower -- exp of an ALGEBRAIC function (algexp.lisp). theta=exp(w), w a field
  element, has logarithmic derivative w_prime in K (e.g. exp(sqrt x), w_prime=1/(2 sqrt x)); the Risch
  equation A_prime + w_prime A = B then has a FIELD-element coefficient, so w_prime A is a field product
  and the y-power sectors COUPLE into one linear system (solved by requiring the residual to vanish at
  sample points, certified in the field). Verified INT (1/(2 sqrt x)) exp(sqrt x)=exp(sqrt x),
  INT ((1+sqrt x)/(2 sqrt x)) exp(sqrt x)=sqrt x exp(sqrt x), and exp(x^(1/3)) on y^3=x. DONE also: the PRIMITIVE companion -- log of an ALGEBRAIC function (alglog.lisp). t=log(w), w a field
  element, has t_prime=w_prime/w in K (via the field inverse from senorm); the tower K(t) derivation then
  uses sf-product with the field-element t_prime, coupling the y-power sectors, and the coupled system is
  solved by the eval-point method (like algexp) with the answer t-degree inferred from the integrand.
  Verified INT (w_prime/w)=log(sqrt x+1) on y^2=x, the t^2 case INT (log w)(w_prime/w)=(1/2)(log w)^2, and
  log(x^(1/3)+1) on y^3=x. Both entangled towers (exp and log of an algebraic argument) now done. NEXT:
  deeper stacked towers with several monomials -- the genuine remaining summit (normalization at singular places) -- the part
  that actually lifts integration past the hyperelliptic restriction. Original note: Puiseux for (beyond sqrt: y^n = ..., and
  non-squarefree/branch structure). This is the Trager-Bronstein normalization at each singular place; it
  removes the "hyperelliptic only" restriction. Largest rung; multiple sessions.

RUNG 5 -- mixed transcendental-over-algebraic towers (algebraic functions of exp/log), unifying with the
  recursive Risch driver. The full summit.

## Invariants (every rung)
- Differentiation certificate is THE arbiter. 'elementary only if D(answer)=integrand verified in K.
- 'non-elementary must be a proof (a structural obstruction), never a failure-to-find.
- 'not-handled = honest don't-know outside proven scope. Never a guessed closed form.
- Inventory disk first; additive-safety + golden-integrity each session; fresh-extract verify before present.

## Even-order elliptic-log: RESOLVED (ellint.lisp) -- it was a criterion-completeness issue
BREAKTHROUGH: the "even-order" difficulty was not about constructing logs but about the CRITERION being
incomplete. The torsion test (Rung 3b) is NECESSARY but NOT SUFFICIENT. The complete criterion (Trager;
Combot arXiv:2103.04134, "I-L is of first kind ... I elementary iff I-L=0") is TWO parts:
  (1) pole P=(s,rho) torsion  -> the log part L=c log f EXISTS;
  (2) the remainder I-L = lambda*dx/y (holomorphic) must VANISH (lambda=0).
ellint.lisp (ei-integrate) implements BOTH, by:
  - building g with div=N[P]-N[O] (N=order P) via INTERPOLATION (g=A+B y vanishing to order N at P, using the
    local y-series; robust at 2-torsion; verified N(g)=g*conj(g)=(x-s)^N for N=3,4,5,6);
  - f=g/conj(g), c=1/(N rho) so c f'/f matches omega's residues (+-1/rho at P,-P);
  - computing lambda; ELEMENTARY iff lambda=0, then c log f CERTIFIED by differentiation.
CORRECTED FINDINGS (the old torsion-only verdict was over-optimistic):
  - INT dx/(x sqrt(x^3+1)) = (1/3)log((y-1)/(y+1)): order-3 pole, lambda=0, ELEMENTARY (certified). Still holds.
  - INT dx/(x sqrt(x^3+4)): order-3 pole (0,2), lambda=0, ELEMENTARY (certified).
  - INT dx/((x-2)sqrt(x^3+1)): order-6 pole (2,3), lambda=-1/3 != 0 -> NON-ELEMENTARY (was wrongly 'elementary').
  - INT dx/((x-2)sqrt(x^3+4x)): order-4 pole (2,4), lambda != 0 -> NON-ELEMENTARY.
  - order-5 pole on y^2=x^3-x^2+1/4: lambda=2/5 != 0 -> NON-ELEMENTARY.
So: among the tested torsion poles, only the order-3 ones are elementary; lambda=0 is the real condition, NOT
parity. elt-decide (elltorsion) retained as the torsion (necessary-condition) test; ei-integrate is the sound
complete decision. NOTE for later: elt-order has a Nagell-Lutz early-exit valid only for INTEGRAL models;
ei-torsion-order (in ellint) drops it and works for non-integral models too (bound 14).


MAXIMA-PARITY (beyond the Trager ladder): linear recurrence solving is now COMPLETE.  linrec.lisp solves
C-finite recurrences whose characteristic polynomial splits over Q (rational roots, including repeated roots);
linrec2.lisp closes the remaining case -- a degree-2 irreducible characteristic polynomial (irrational quadratic
roots), the Binet/Lucas/Pell case.  For a_n = p a_{n-1} + q a_{n-2} with non-square discriminant D = p^2+4q, the
closed form a_n = A r^n + B s^n (r,s = (p +- sqrt D)/2, A,B conjugate in Q(sqrt D)) is computed exactly in
Q(sqrt D) and certified against the iterated sequence.  Verified Fibonacci, Lucas, Pell, and a perfect-square
discriminant correctly deferred to linrec.

THREE FRONTIERS (parallel): (1) limits at an arbitrary point + indeterminate 0/0 (slimit2.lisp) and (2)
transcendental equation solving by substitution (transsolve.lisp) close/advance Maxima-parity gaps; (3) the
NESTED-log tower Q(x)(t1)(t2), t1=log x, t2=log(log x) (nestlog.lisp) pushes Rung 5.  The nested case is the
first where a monomial's derivative carries a lower monomial in its DENOMINATOR (t2'=1/(x t1)), forcing the
coefficient ring to be rational in t1; INT 1/(x log x) dx = log(log x) and the (log log x)^2 case are certified.
Still open at the summit: arbitrary nesting depth and a genuine decision procedure (proving non-elementarity),
not just bounded-ansatz constructive solving.

RUNG 5 (continued): the NESTED EXPONENTIAL tower (nestexp.lisp) is the multiplicative dual of the nested
logarithm.  s2 = exp(exp x) over s1 = exp x has s2' = s1 s2, so the second monomial's derivative MULTIPLIES by
the first (raising the s1-degree) rather than dividing by it; the coefficient ring stays polynomial.  Because the
derivation preserves s2-degree (block-diagonal), the integral solves by undetermined coefficients:
INT exp(x) exp(exp x) dx = exp(exp x) and the (exp exp x)^2 case are certified.  Both nested towers (log and exp)
are now done.  Still open at the summit: arbitrary nesting depth, mixed nested towers over the algebraic base,
and a genuine decision procedure (proving non-elementarity), not just bounded-ansatz constructive solving.

RUNG 5 (deeper): ARBITRARY-DEPTH iterated exponentials (itexp.lisp).  The iterated tower E_k = exp(E_{k-1}) for
any height n; the derivative law E_k' = E_k (E_1...E_{k-1}) (induction) gives INT(E_1 E_2 ... E_n) dx = E_n,
certified at depths 2-5.  This is the first UNBOUNDED-depth nesting handled (prior nested towers were depth 2).
Still open: mixed nested towers over the algebraic base, and a genuine decision procedure (proving
non-elementarity) rather than bounded-ansatz constructive solving.

RUNG 5 (deeper, two steps): (1) the iterated LOGARITHM tower (itlog.lisp), reciprocal dual of itexp -- L_k' =
1/(x L_1...L_{k-1}), so INT 1/(x L_1...L_{n-1}) dx = L_n at arbitrary depth (Laurent monomials, lower logs in
denominators).  (2) a GENERAL tower integrator (itexpsolve.lisp) beyond the full product: arbitrary elements of
the iterated-exp tower integrated by undetermined coefficients over a monomial support + certificate, e.g.
INT(exp x + exp x exp exp x) = exp x + exp exp x.  Still open at the summit: mixed nested towers over the
algebraic base, and a genuine decision procedure (proving non-elementarity).

RUNG 5 (two more steps): (1) a GENERAL Laurent solver for the iterated-LOGARITHM tower (itlogsolve.lisp),
mirroring itexpsolve -- arbitrary log-tower elements integrated by undetermined coefficients + certificate, e.g.
INT(1/x + 1/(x log x)) = log x + log log x.  (2) the FUSION step (nestalg.lisp): a nested logarithm over the
ALGEBRAIC base, t1 = log(w), t2 = log(log w) with w a field element of K = Q(x)[y]/(y^n - g); t1' = w'/w is a
field element, t2' = t1'/t1 carries t1 in the denominator with a field-element numerator, and INT (w'/w)/log(w)
dx = log(log(sqrt x + 1)) is certified (n = 2 and n = 3).  This was the open "mixed nested over the algebraic
base" item.  Still open at the summit: a genuine DECISION procedure (proving non-elementarity) over arbitrary
towers, rather than bounded-ansatz / structured constructive solving.

THE SUMMIT STEP -- the first genuine DECIDER (liouville.lisp).  For INT P e^g dx (P, g polynomials, deg g >= 1),
Liouville's theorem: elementary iff a rational R solves R' + g' R = P.  Polynomial degree bound (deg R = deg P -
deg g + 1) makes this a finite, exact linear decision: a solution R proves elementarity (INT = R e^g, certified
by R' + g' R = P), an inconsistent system proves NON-ELEMENTARITY.  erf (INT e^{x^2}) and Ei (INT e^x/x) come out
proven-impossible.  This is the first module that DECIDES rather than constructs -- it can say "no elementary
form exists" with a proof.  Remaining: extend the decider to more general integrand classes (mixed
exp/log/algebraic, the full Risch structure theorem) -- the present decider covers INT P e^g for polynomial P, g.

THE DECIDER SUITE (big push): four Liouville-based deciders now -- exponential (liouville, INT P e^g),
logarithmic (liouvillelog, INT P log x elementary + li non-elementary), rational-coefficient exponential
(liouvillerat, INT R e^x by solving S'+S=R, with the Ei obstruction proven), and the structure-theorem witness
(liouvilleform, the explicit f = v' + sum c_i u_i'/u_i for rational f).  The three classic special-function
integrals erf, Ei, li are all proven non-elementary.  Remaining summit: ONE decision procedure (the full Risch
structure theorem) over arbitrary mixed exp/log/algebraic towers, subsuming these per-class deciders.

THE UNIFICATION -- the recursive Risch decision procedure (rischtower.lisp).  ONE recursion decides INT f over a
multi-level tower by reducing each level to subproblems one level down (exponential level -> the RDE
c_i'+i b' c_i = a_i in the lower field; degree 0 -> integration there), bottoming out at Q(x).  Proves the
iterated exponentials INT e^{e^x}, INT e^{e^{e^x}} NON-elementary via a non-terminating RDE tail, opposite the
elementary full-product INT(E_1...E_n)=E_n -- distinguishing the single top monomial from the product.  The
per-class deciders (liouville/liouvillelog/liouvillerat) are now one tower-aware recursion: the structural heart
of the Risch algorithm.  Remaining: a fully general RDE solver at every level for arbitrary mixed
exp/log/algebraic integrands (the present recursion decides the exponential reduction and the iterated-exp tower
exactly; honest 'needs-deeper-rde elsewhere).

THE GENERAL RDE + RATIONAL-COEFFICIENT SOLVER (rischrde.lisp, rischrde2.lisp).  rischrde solves y'+f y=g for
rational y with rational f,g (denominator bound -> polynomial RDE A q'+B q=C -> degree-bounded linear solve,
certified by y'+f y=g): a pole in f need not force one in y (y'-(1/x)y=x -> y=x^2), poles in y found when needed
(y=1/x), Ei obstruction proven (y'+y=1/x no rational y).  rischrde2 builds the general exp-integral decider:
INT R e^g for ARBITRARY rational R via the RDE y'+g' y=R, subsuming liouville (poly R) and liouvillerat (rational
R, pole at 0) -- verified to AGREE with both on every shared case.  This is the rational-function-coefficient
solver the tower recursion needs at each level.  Remaining summit: the RDE over non-rational coefficient fields
(the general mixed exp/log/algebraic tower), completing the recursion at every level.

THE TOWER-FIELD RDE -- the recursion calling itself (rischtfrde.lisp, rischtfrde2.lisp).  rischtfrde: y'+f y=g
over K_1=Q(x)(exp b) with base-field f DECOUPLES by theta-degree into per-degree base RDEs (y_k'+(k b'+phi)y_k=g_k),
each solved by rischrde one level down -- INT e^x, INT x e^x solved, Ei detected per-degree.  rischtfrde2: the
COUPLED case c'+(m s)c=target (positive-degree coefficient) is a banded recurrence across theta-degrees; the
non-terminating tail is detected, so INT exp(exp x) is proven non-elementary THROUGH the RDE recursion (derived,
verified to AGREE with the tower decider).  The recursive descent now runs on the differential-equation machinery
underneath.  Remaining summit: the fully general coupled solver at arbitrary height and for mixed
exp/log/algebraic levels.

THE GENERAL COUPLED RDE + UNIFIED HEIGHT-1 INTEGRATOR (rischcoupled.lisp, rischint1.lisp).  rischcoupled handles
both height-1 level types with arbitrary coefficient: exponential (diagonal) -> banded system solved bottom-up
with non-terminating-tail detection (y'+(1+e^x)y=1 non-elementary); logarithmic (degree-shifting derivation) ->
solved top-down, giving INT log x = x log x - x.  rischint1 unifies: INT f = (D y = f) over either tower,
dispatching on level type, bottoming at the rational RDE -- INT e^x, INT x e^x, INT log x, INT (log x)^2 in closed
form, INT e^x/x (Ei) non-elementary, certified and verified to AGREE with the original deciders.  The complete
height-1 Risch integral over both transcendental kinds.  Remaining summit: arbitrary height (nesting the coupled
solver, the recursion calling itself at every level) and the algebraic level.

THE HEIGHT-N RECURSION (rischtowern.lisp, rischintn.lisp).  Uniform tower-element algebra + recursive derivation
D descending one level at a time (D(e^{e^x}) = e^x e^{e^x} at height 2); integrator INT f = (D y = f) whose
per-degree subproblems recurse to height h-1, bottoming at the rational RDE over Q(x).  Exact and certified for
any height-1 tower (AGREES with rischint1) and for decoupled arbitrary-height towers; a genuine height-2 integral
INT log(e^x+1) computed by the descent 2->1->Q(x); the coupled exp-over-exp case honestly DEFERS.  The Risch
descent now runs at arbitrary height, sound throughout via the differentiation certificate.  Remaining summit:
the fully general coupled solver at every height (nesting the banded recurrence) and the algebraic level.

NESTING THE COUPLED RECURRENCE (rischcrde.lisp) -- the exp-over-exp tower solved.  te-crde-solve solves
D y + F y = g at arbitrary height for an arbitrary tower-element F, reducing each theta-degree to an RDE one
level down (coefficient possibly coupled -> recurse), bottoming at the rational RDE; certificate-gated so a
returned y is genuine, a proven non-terminating tail gives 'no-solution, an inconclusive solve is flagged
honestly.  Wired into rischintn (INT f = D y = f = te-crde-solve with F = 0), so the height-n integrator now
SOLVES the exp-over-exp tower: INT e^{e^x} PROVEN non-elementary THROUGH the recursion (top-degree subproblem is
the coupled height-1 RDE c' + e^x c = 1), verdict AGREES three ways (height-n recursion, rischtfrde2, rischtower).
Remaining summit: the coupled COMPLETENESS fix (carry homogeneous solution spaces through the banded recurrence,
the "SPDE bookkeeping", so solvable coupled cases like INT(e^x e^{e^x})=e^{e^x} are solved rather than honestly
deferred); the algebraic level; and Laurent integrands (1/theta, e.g. li) through the unified recursion.

CLOSING THE COUPLED COMPLETENESS GAP (rischcrdeh.lisp).  The degree-0 homogeneous freedom (D(y_0)=RHS, solution
free up to a constant) that rischcrde left as honest 'inconclusive is now resolved: the constant is determined by
a linear two-probe solve and CERTIFIED, with the inner solves recursing through te-crdeh-solve.  So SOLVABLE
coupled integrals are now solved -- INT (e^x e^{e^x}) = e^{e^x} through the height-2 recursion (top-degree
subproblem D(c)+e^x c=e^x solved as c=1) -- while INT e^{e^x}, INT e^x/x stay proven non-elementary.  The
exp-over-exp tower is now both DECIDED and INTEGRATED through the recursion, soundness held by the differentiation
certificate.  Remaining summit: deeper multi-parameter homogeneous spaces; the algebraic level; Laurent
integrands (li) through the unified recursion.

MULTI-PARAMETER HOMOGENEOUS BOOKKEEPING (rischcrdem.lisp).  The general completeness algorithm: several
homogeneous constants solved jointly.  The tail is affine in the constant-vector C (tail = T_0 + M C); collect
the d.o.f., build T_0 and M by probing, solve M C = -T_0 exactly over Q by a rational Gaussian elimination
(verified to (5/3,5/3,8/3) on a 3x3), substitute, re-run, certify.  Inner solves delegate to the single-param
layer so freedoms compound across levels.  Reproduces INT (e^x e^{e^x}) = e^{e^x}; solves INT (log x)^2 through
the multi-parameter path; preserves the non-elementary verdicts; all certified.  Subsumes the single-parameter
case.  Remaining summit: the algebraic level (y^n = g) and Laurent integrands (li) through the unified recursion.

THE ALGEBRAIC LEVEL (rischtoweralg.lisp).  The first non-transcendental level type in the recursion: (alg n a),
theta^n = a.  theta' = (a'/(n a)) theta = w theta makes the derivation diagonal like exp (rate w = a'/(n a)), with
the algebra reducing theta^n = a (quadratic case theta = sqrt(a) implemented).  Verified D(sqrt x) = 1/(2 sqrt x),
(sqrt x)^2 = x, D(x + sqrt x) = 1 + 1/(2 sqrt x), w = 1/(2x), and D(theta^2) = D(a) consistency.  Sits alongside
exp and log in the recursive tower.  Remaining summit: wire the algebraic level into the coupled RDE/integrator
(sqrt-tower integrals decided through the recursion) and Laurent integrands (li) through the unified recursion.

THE FOUR REMAINING STEPS -- all advanced this segment.
(1) Algebraic level WIRED INTO THE INTEGRATOR (rischintn + rischcrde): INT 1/(2 sqrt x)=sqrt x, INT x^{-3/2}=
    -2/sqrt x decided through the recursion, certified (diagonal rate a'/(n a), degree bound n-1).
(2) LAURENT INTEGRANDS (rischlaurent): INT 1/(x log x)=log log x via the theta^{-1} new-log residue; li and deeper
    powers deferred honestly.
(3) GENERAL-DEGREE algebraic extensions (rischtoweralgn): cube roots and beyond, INT (1/3)x^{-2/3}=x^{1/3};
    theta^n=a multiplication reducing correctly (theta*theta^2=x, theta^2*theta^2=x theta, theta^3=x).
(4) STRUCTURE THEOREM logarithmic part (rischstruct): f=sum c_i g_i'/g_i solved for constant residues over Q
    (exact Gaussian elimination, recovers (3,5)), certified -> INT f = sum c_i log(g_i); non-cases rejected.
Remaining summit (full Risch-Trager parity): the complete structure theorem fusing the polynomial (v') and
logarithmic parts over arbitrary towers; general algebraic function fields with integral bases; simplification
with assumptions.

THE SUMMIT REACHED -- the unified top-level integrator (rischtop.lisp).  ONE entry point fuses: the rational
integrator (Hermite + Rothstein-Trager auto-found logarithms + arctangents) and the height-n tower recursion
(exp/log/algebraic any degree) and the Laurent new-logarithm case -- all certified by differentiation.  Crowning
fused case INT 2x^3/(x^2-1) = x^2 + log(x^2-1) (rational part AND auto-found log together, certified); INT
1/(x^2+1)=arctan x certified; INT 1/(x^2-2) honestly needs-algebraic; INT e^x, INT log x, INT 1/(2 sqrt x), INT
e^(e^x) (non-elem), INT 1/(x log x)=log log x all through the one integrator.  The elementary-tower Risch summit.
BEYOND (the open research frontier): arbitrary algebraic function fields with integral bases; the complete
structure theorem fusing polynomial + logarithmic parts over arbitrary nested towers; simplification with
assumptions.
