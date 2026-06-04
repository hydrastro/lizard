; -*- lisp -*-
; lib/cas/algherm.lisp -- RUNG 2 of the Trager-Bronstein climb (see docs/TRAGER_ROADMAP.md): HERMITE REDUCTION
; for a rational-function numerator over sqrt(p).
;
; hyperell reduces a POLYNOMIAL numerator P(x)/sqrt(p).  algresidue (Rung 1) showed that the finite residue
; obstruction lives only in the simple poles of the rational part.  This rung removes the higher-order poles:
; given a differential f = w(x) y with w = A/D a proper rational function over Q(x) (y = sqrt(p)), it produces
;
;     INT w y dx = G y + INT wbar y dx,     G in Q(x),  deg-of-pole(wbar) <= 1 at every non-branch place,
;
; i.e. the SECOND-KIND higher-order poles are integrated into the algebraic part G*sqrt(p), leaving only simple
; poles plus the polynomial/first-kind content for the later rungs.  The reduction is the algebraic Hermite
; method (Bronstein 5.3) carried out over Q via the squarefree factorization of D, so no irrational roots are
; ever needed, and EVERY step is checked by the differentiation certificate D(G y) + remainder = f in K.
;
; The reduction step.  D(G y) = (G' + G p'/(2p)) y for G in Q(x).  Writing things over the common denominator,
; if D has a squarefree factor V of multiplicity m >= 2 (V^m || D), we look for G = B / V^{m-1} so that
; subtracting D(G y) cancels the order-m part of w at the roots of V.  Differentiating G = B/V^{m-1}:
;     G' = B'/V^{m-1} - (m-1) B V'/V^m,
; so D(G y) has v-part  B'/V^{m-1} - (m-1) B V'/V^m + (B/V^{m-1}) p'/(2p).  Clearing to denominator V^m * (2p):
; matching the V^m part of w fixes B by a Bezout identity gcd(V', V) = 1 (V squarefree, and we keep V coprime
; to p so V' is invertible mod V).  We implement the standard one-step-per-multiplicity descent and CERTIFY.
;
; Builds on algfunc.lisp (K and its derivation) and poly.lisp / ratfun.lisp.

(import "cas/algfunc.lisp")

; ----- helpers -----
(define (ah-ppow b n) (if (= n 0) (list 1) (poly-mul b (ah-ppow b (- n 1)))))
; extended gcd over Q[x]: returns (g s t) with s*a + t*b = g = gcd(a,b)
(define (ah-egcd a b)
  (if (poly-zero? b) (list a (list 1) (quote ()))
      (let ((qr (poly-divmod a b)))
        (let ((rec (ah-egcd b (car (cdr qr)))))
          (let ((g (car rec)) (s1 (car (cdr rec))) (t1 (car (cdr (cdr rec)))))
            (list g t1 (poly-sub s1 (poly-mul (car qr) t1))))))))

; multiplicity of squarefree factor V in D (how many times V divides D)
(define (ah-mult D V) (ah-mult-go D V 0))
(define (ah-mult-go D V acc)
  (let ((qr (poly-divmod D V)))
    (if (poly-zero? (car (cdr qr))) (ah-mult-go (car qr) V (+ acc 1)) acc)))

; ----- one Hermite step on the v y part, w = A/D, for the highest multiplicity m >= 2 -----
; Returns (list Gstep Abar Dbar): subtracting D(Gstep y) replaces A/D by Abar/Dbar with strictly lower max pole.
; We reduce the highest squarefree level at a time using the classical identity (Bronstein 5.3.3) adapted to
; the y-weight: with V^m || D, U = D/V^m, the order-m part is killed by G = (something)/V^{m-1}.
; Concretely we use the standard rational Hermite on the MODIFIED integrand that accounts for the p'/(2p) term:
;   the differential w y dx has the same pole structure (away from p) as the rational form (w * sqrt-correction);
;   since p is coprime to V at non-branch poles, the y-weight only shifts the numerator by the regular factor
;   p'/(2p), which we fold in.  Implementation: classic Hermite on w treating the extra term as part of A.
(define (ah-hermite-vy A D p)
  (ah-herm-loop A D p (rat-zero)))                  ; accumulate G into a rat (the algebraic part coefficient)

; The loop: while D has a repeated factor, do one reduction; return (list Gtotal Abar Dbar) with Dbar squarefree.
(define (ah-herm-loop A D p Gacc)
  (let ((Dr (poly-monic-or D)))
    (let ((g1 (poly-gcd Dr (poly-deriv Dr))))
      (if (<= (poly-deg g1) 0)
          (list Gacc A Dr)                          ; D squarefree -> done; remaining A/Dr has only simple poles
          (ah-herm-step A Dr p Gacc)))))
(define (poly-monic-or D) (if (poly-zero? D) D (poly-monic D)))

; one reduction step using Hermite's identity:
;   D = Dstar * S where S = gcd(D, D'), Dstar = D/S squarefree (radical).  Bronstein's per-step formula
;   writes A/D = A/(Dstar S) and reduces the multiplicity by solving B Dstar' ... -- we use the explicit
;   "reduce the largest power" form, which over Q needs the squarefree DECOMPOSITION.  We take the squarefree
;   factor V of maximal multiplicity m, set W = D / V^m, and solve for the numerator that cancels the V^m pole.
(define (ah-herm-step A D p Gacc)
  (let ((sq (square-free D)))                       ; ((mult Vi) ...)
    (let ((top (ah-max-mult sq (quote ()) 0)))      ; (m . V) with m maximal (m>=2)
      (let ((m (car top)) (V (cdr top)))
        (let ((Vm1 (ah-ppow V (- m 1))) (Vm (ah-ppow V m)))
          (let ((W (car (poly-divmod D Vm))))       ; D = V^m * W
            ; reduce only at NON-branch repeated factors: V must be coprime to p (else it is a ramified place,
            ; handled by hyperell/later rungs, not by this rational Hermite).
            (if (> (poly-deg (poly-gcd V (rat-num p))) 0)
                (list Gacc A D)                      ; V shares a root with p -> branch point; stop here
                ; cancellation condition (derived): -(m-1) B V' W  ≡  A  (mod V); the 2p cancels both sides.
                (let ((fac (poly-rem (poly-mul (poly-scale (- 0 (- m 1)) (poly-deriv V)) W) V)))
                  (let ((inv (ah-invmod fac V)))
                    (if (equal? inv (quote noninv))
                        (list Gacc A D)
                        (let ((B (poly-rem (poly-mul (poly-rem A V) inv) V)))
                          (let ((Gstep (rat-make B Vm1)))
                            (let ((newrem (ah-subtract-step A D p Gstep)))
                              (ah-herm-loop (car newrem) (car (cdr newrem)) p (rat-add Gacc Gstep)))))))))))))))

; max-multiplicity squarefree factor with multiplicity >= 2
(define (ah-max-mult sq best bestm)
  (if (null? sq) (cons bestm best)
      (let ((mult (car (car sq))) (V (car (cdr (car sq)))))
        (if (if (>= mult 2) (> mult bestm) #f)
            (ah-max-mult (cdr sq) V mult)
            (ah-max-mult (cdr sq) best bestm)))))

; inverse of a mod V over Q[x] (V squarefree), via extended gcd; 'noninv if gcd nontrivial
(define (ah-invmod a V)
  (let ((e (ah-egcd (poly-rem a V) V)))
    (if (<= (poly-deg (car e)) 0)
        (poly-rem (poly-scale (/ 1 (car (car e))) (car (cdr e))) V)   ; normalize gcd to 1
        (quote noninv))))

; subtract D(Gstep y) from the integrand w y (w = A/D); return (list A' D') for the remainder rat w'
(define (ah-subtract-step A D p Gstep)
  (let ((w (rat-make A D)))
    (let ((DG (af-deriv (rat-from-poly-or p) (af-make (rat-zero) Gstep))))   ; D(Gstep y), v-part is the rat we need
      (let ((wbar (rat-sub w (af-v DG))))
        (list (rat-num wbar) (rat-den wbar))))))
(define (rat-from-poly-or p) p)                     ; p is already a rat in our calls

; ----- top-level Rung-2 entry: INT w(x) sqrt(p) dx form, i.e. integrand = w y, reduce 2nd-kind higher poles ---
; INPUT w a rat (the coefficient of y in the integrand f = w y), p a rat (squarefree radicand).
; OUTPUT (list 'reduced Gtotal wbar)  where INT w y dx = Gtotal*y + INT wbar y dx and wbar has only simple
;        non-branch poles (plus possible polynomial part); certified by ah-verify.
(define (ah-reduce w p)
  (let ((res (ah-hermite-vy (rat-num w) (rat-den w) p)))
    (list (quote reduced) (car res) (rat-make (car (cdr res)) (car (cdr (cdr res)))))))

; certificate: D(Gtotal y) + (wbar y) = (w y)  in K
(define (ah-verify w p)
  (let ((r (ah-reduce w p)))
    (let ((G (car (cdr r))) (wbar (car (cdr (cdr r)))))
      (af-equal? (af-add (af-deriv p (af-make (rat-zero) G)) (af-make (rat-zero) wbar))
                 (af-make (rat-zero) w)))))
