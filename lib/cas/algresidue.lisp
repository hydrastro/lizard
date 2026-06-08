; -*- lisp -*-
; lib/cas/algresidue.lisp -- RUNG 1 of the Trager-Bronstein algebraic-integration climb (see docs/TRAGER_ROADMAP.md).
;
; Given a differential f dx on the hyperelliptic curve y^2 = p (p squarefree), with f = u(x) + v(x) y an element
; of K = Q(x)[y]/(y^2 - p), this module computes the RESIDUES of f dx at the finite places and delivers the
; one decision that residues alone settle: whether the differential is of the SECOND KIND (all residues zero),
; in which case it is elementary (its antiderivative is an algebraic function), versus THIRD KIND (some nonzero
; residue), which needs the divisor/torsion machinery of a later rung.  Nothing is guessed: an 'elementary
; verdict here is only issued for the residue-free sub-case that the differentiation arbiter can confirm.
;
; Residue structure over a fibre x = s (Bronstein 5.x):
;   * s NOT a branch point (p(s) != 0): the fibre has two places (s, +y0) and (s, -y0), y0 = sqrt(p(s)).
;       - the v(x) y part contributes residues  res_{x=s}(v) * (+/- y0)  -- a CONJUGATE PAIR that SUMS TO ZERO;
;       - the u(x) part contributes residue res_{x=s}(u) at BOTH places -- these ADD, total 2 res_{x=s}(u).
;     So the only finite residue obstruction from non-branch poles is  2 * (sum of residues of the u-part).
;   * s a branch point (p(s) = 0): a single place; handled only in the regular sub-case here (else 'not-handled).
;
; Hence: the obstruction divisor's finite residues vanish iff the u-part is residue-free at its non-branch poles
; (e.g. u is a polynomial, or u's proper-fraction part has only higher-order poles, which carry no residue) and
; the v y part has only simple non-branch poles (its residues auto-cancel).  This module detects exactly that.
;
; Builds on algfunc.lisp (the field K) and poly.lisp / ratfun.lisp.

(import "cas/algfunc.lisp")
(import "cas/hyperell.lisp")

; ----- small helpers -----
(define (ar-prat r) (if (null? r) (rat-zero) r))
(define (ar-polyeval-q p s) (poly-eval p s))                 ; evaluate Q[x] poly at rational s

; ----- residue of a rational function u = A/B at a SIMPLE root s of B: A(s)/B'(s) -----
(define (ar-residue-at u s)                                  ; u a rat, s a rational simple pole
  (/ (poly-eval (rat-num u) s) (poly-eval (poly-deriv (rat-den u)) s)))

; ----- the branch / non-branch split of a denominator B against p -----
(define (ar-branch-part B p) (poly-gcd B p))                 ; common roots = branch points
(define (ar-nonbranch-part B p) (car (poly-divmod B (ar-branch-part B p))))

; ----- pole-order check: does B have only SIMPLE roots away from p? (squarefree non-branch part) -----
; B's non-branch part is squarefree  <=>  gcd(nb, nb') is constant.
(define (ar-simple-nonbranch? B p)
  (let ((nb (ar-nonbranch-part B p)))
    (<= (poly-deg (poly-gcd nb (poly-deriv nb))) 0)))

; ----- the SECOND-KIND decision for a pure (1/sqrt(p)) integrand f = v(x) y -----
; The integrand r(x)/sqrt(p) is represented in K as (r/p) y, i.e. v = r/p.  Its finite poles:
;   - roots of p: branch points (the sqrt's own ramification) -- these carry the first/second-kind behaviour
;     handled by hyperell after we clear the rational denominator;
;   - roots of the rational denominator of r: non-branch poles, residues cancel in conjugate pairs.
; So a 1/sqrt(p) integrand whose rational numerator r is a POLYNOMIAL reduces directly to hyperell.  When r is a
; proper fraction with only simple non-branch poles, the conjugate-pair cancellation makes the finite residue
; sum zero, and (after the higher-pole Hermite reduction of a later rung) the object is second-kind.  This rung
; CERTIFIES the polynomial-numerator case by handing it to hyperell, and REPORTS the residue divisor otherwise.

; classify a differential given as f = u + v y (u,v rats) over p -> a verdict list:
;   (list 'second-kind 'poly-numerator <P>)         -- v y with v=P/p, P polynomial: delegate to hyperell
;   (list 'has-residues <total-u-residue-info>)      -- nonzero u-residues: third-kind, needs a later rung
;   (list 'not-handled <why>)                        -- branch-point poles / higher structure beyond this rung
(define (ar-classify u v p)
  (cond
    ((not (rat-zero? u)) (ar-classify-u u v p))
    (else (ar-classify-vy v p))))

; v y part: v = r/p.  If v*p is a polynomial (i.e. den(v) | p, the only poles are branch points), delegate to
; hyperell with that polynomial numerator.  Otherwise report the non-branch pole structure.
(define (ar-classify-vy v p)
  (let ((vp (rat-mul v (rat-from-poly p))))                  ; v * p ; integrand was v y = (r/p) y, r = v p
    (if (rat-is-poly? vp)
        (list (quote second-kind) (quote poly-numerator) (rat-num vp))
        (let ((B (rat-den v)))
          (if (ar-simple-nonbranch? B p)
              (list (quote second-kind) (quote simple-nonbranch-poles-cancel) v)   ; residues cancel; needs Rung2 Hermite
              (list (quote not-handled) (quote higher-order-nonbranch-poles)))))))

; u part (rational, no y): its simple poles carry residues 2*res(u) summed over the fibre -> third-kind obstruction.
(define (ar-classify-u u v p)
  (if (not (rat-zero? v)) (list (quote not-handled) (quote mixed-u-and-vy))
      (let ((B (rat-den u)))
        (if (rat-is-poly? u) (list (quote second-kind) (quote pure-polynomial) u)   ; no finite poles at all
            (list (quote has-residues) (quote u-simple-poles) u)))))

(define (rat-is-poly? r) (<= (poly-deg (rat-den r)) 0))

; ----- the certified entry for the case this rung OWNS: 1/sqrt(p) integrand with polynomial numerator -----
; INT (P(x)/p) y dx = INT P(x)/sqrt(p) dx  -> delegate to hyperell, which decides + certifies.
(define (ar-integrate-1oversqrt u v p)
  (let ((c (ar-classify u v p)))
    (cond ((if (equal? (car c) (quote second-kind)) (equal? (car (cdr c)) (quote poly-numerator)) #f)
           (he-integrate (car (cdr (cdr c))) p))             ; (elementary Q) | (non-elementary g S)
          ((equal? (car c) (quote second-kind)) (list (quote reducible-second-kind) (car (cdr c))))
          ((equal? (car c) (quote has-residues)) (list (quote third-kind) (car (cdr (cdr c)))))
          (else c))))
(define (ar-classify-differential u v p) (ar-classify u v p))
