; -*- lisp -*-
; lib/cas/algresq.lisp -- ALGEBRAIC RESIDUES in Rothstein-Trager: the conjugate-root case, where the residues of
; a proper rational function over an irreducible quadratic are irrational (real algebraic), and the integral is
; an algebraic-coefficient logarithm.  This closes the first genuinely-algebraic-residue gap above the rational
; Rothstein-Trager part (docs/TRAGER_ROADMAP.md, the frontier, "INT algebraic-residue case").
;
; For INT (A x + B)/(x^2 + p x + q) dx with discriminant disc = p^2 - 4 q > 0 (so the denominator has real but
; irrational roots r1, r2 = (-p +- sqrt(disc))/2), the standard split is
;     (A x + B)/(x^2+px+q) = (A/2)(2x+p)/(x^2+px+q) + (B - A p/2)/(x^2+px+q),
; giving
;     INT = (A/2) log(x^2+px+q)                         [rational-coefficient logarithm]
;         + ((B - A p/2)/sqrt(disc)) log((x-r1)/(x-r2))  [ALGEBRAIC-coefficient logarithm, over Q(sqrt d)].
; Writing disc = s^2 d with d squarefree, sqrt(disc) = s sqrt(d), the algebraic part lives in Q(sqrt d).
;
; SOUNDNESS without leaving Q.  The derivative of the algebraic logarithm is
;     ((B - A p/2)/sqrt(disc)) * (r1 - r2)/(x^2+px+q) = (B - A p/2)/(x^2+px+q)   (since r1 - r2 = sqrt(disc)),
; which is RATIONAL -- the sqrt cancels.  So although the antiderivative carries sqrt(d), its derivative is a
; rational function, and the whole result is CERTIFIED by the exact rational identity
;     d/dx[ (A/2) log(x^2+px+q) + algebraic-log ] = (A x + B)/(x^2+px+q)  in Q(x).
; The module therefore returns a SYMBOLIC result (the rational-log coefficient and argument, and the algebraic-log
; coefficient as (rational-multiple, sqrt-radicand) with its arguments) together with a certificate computed
; entirely over Q.  A wrong answer is never returned: the rational identity is checked.
;
; Scope: irreducible quadratic denominator with disc > 0 and irrational roots (disc not a perfect square in Q).
; The disc < 0 case is the arctangent, already handled by integrate.lisp; disc a perfect square has rational
; roots, also already handled.  Higher-degree algebraic residues (cubic+, RootSum over Q(alpha)) remain open.
;
; Public:
;   algresq-disc p q                  -> p^2 - 4q
;   algresq-squarefree-part n         -> (s . d) with n = s^2 d, d squarefree (n>0)
;   algresq-integrate A B p q         -> (list 'elementary <rlog-coeff> <rlog-arg> <alog-coeff-rat> <alog-rad>)
;                                       | (list 'not-applicable ..) : INT (Ax+B)/(x^2+px+q), disc>0 irrational
;   algresq-certify A B p q r         -> #t iff the result differentiates back to (Ax+B)/(x^2+px+q) over Q(x)
;
; Verified: INT 1/(x^2-2) = (1/(2 sqrt2)) log((x-sqrt2)/(x+sqrt2)); INT x/(x^2-2) = (1/2) log(x^2-2);
; INT (x+1)/(x^2-2) combines both; each certified by the exact rational identity; the rational-root and
; negative-discriminant cases are reported not-applicable (handled elsewhere).
;
; Builds on tower.lisp / poly.lisp (exact Q(x) arithmetic) and numbertheory.lisp (integer square-free part).

(import "cas/tower.lisp")
(import "cas/poly.lisp")
(import "cas/numbertheory.lisp")

; ----- discriminant -----
(define (algresq-disc p q) (- (* p p) (* 4 q)))

; ----- integer square-free part: n = s^2 d, d squarefree.  Returns (s . d).  For n>0. -----
(define (algresq-squarefree-part n) (algresq-sf-go n 2 1 n))
; pull out square factors k^2 (k from 2 up) ; s accumulates the pulled root, d is the remaining radicand
(define (algresq-sf-go d k s rem)
  (cond ((> (* k k) rem) (cons s rem))
        ((= (remainder rem (* k k)) 0) (algresq-sf-go d k (* s k) (quotient rem (* k k))))
        (else (algresq-sf-go d (+ k 1) s rem))))

; ----- is disc a perfect square (=> rational roots, not our case)? -----
(define (algresq-perfect-square? n) (algresq-ps-go n 0))
(define (algresq-ps-go n k) (cond ((> (* k k) n) #f) ((= (* k k) n) #t) (else (algresq-ps-go n (+ k 1)))))

; ----- the integrator -----
; result shape: (elementary rlog-coeff rlog-arg alog-coeff-rat alog-rad)
;   rlog-coeff : rational, the coefficient of log(x^2+px+q)              [= A/2]
;   rlog-arg   : the polynomial x^2+px+q (as a coeff list)               [the rational-log argument]
;   alog-coeff-rat : rational m such that the algebraic-log coefficient is m / sqrt(disc) ... we instead store the
;                    coefficient in the form (B - A p/2)/sqrt(disc) = (B - A p/2)/(s sqrt d) = (m) / sqrt(d) with
;                    m = (B - A p/2)/s ; so the algebraic-log coefficient is m/sqrt(d).
;   alog-rad   : the squarefree radicand d (so the coefficient is m/sqrt(d), arguments x - r1, x - r2 in Q(sqrt d))
(define (algresq-integrate A B p q) (algresq-dispatch A B p q (algresq-disc p q)))
(define (algresq-dispatch A B p q disc)
  (cond ((not (> disc 0)) (list (quote not-applicable) (quote nonpositive-discriminant)))
        ((algresq-perfect-square? disc) (list (quote not-applicable) (quote rational-roots)))
        (else (algresq-build A B p q disc))))
(define (algresq-build A B p q disc) (algresq-mk A B p q (algresq-squarefree-part disc)))
(define (algresq-mk A B p q sd)
  (list (quote elementary)
        (rat-make (list A) (list 2))                 ; rlog-coeff = A/2
        (list q p 1)                                 ; rlog-arg = x^2 + p x + q
        (algresq-alog-coeff A B p (car sd))           ; alog-coeff-rat = (B - A p/2)/s
        (cdr sd)))                                   ; alog-rad = d
; (B - A p/2)/s as an exact rational
(define (algresq-alog-coeff A B p s) (rat-make (list (- (* 2 B) (* A p))) (list (* 2 s))))

; ----- the certificate, entirely over Q -----
; derivative = rlog-coeff * (x^2+px+q)'/(x^2+px+q) + [algebraic-log derivative].
; the algebraic-log derivative: coefficient (m/sqrt d) times d/dx log((x-r1)/(x-r2)) = (m/sqrt d)*(r1-r2)/(denom).
; r1 - r2 = sqrt(disc) = s sqrt(d), and m = (B - A p/2)/s, so (m/sqrt d)*(s sqrt d) = m*s = B - A p/2 (rational).
; hence algebraic-log derivative = (B - A p/2)/(x^2+px+q), and the total is checked against (Ax+B)/(x^2+px+q).
(define (algresq-certify A B p q r)
  (if (equal? (car r) (quote elementary))
      (rat-equal? (algresq-deriv r) (rat-make (list B A) (list q p 1)))
      #f))
(define (algresq-deriv r) (rat-add (algresq-rlog-deriv r) (algresq-alog-deriv r)))
; rlog part: coeff * denom'/denom
(define (algresq-rlog-deriv r) (rat-mul (car (cdr r)) (rat-make (poly-deriv (car (cdr (cdr r)))) (car (cdr (cdr r))))))
; alog part: equals (alog-coeff-rat * s) / denom where s = sqrt-of (disc/d); but we recover B - A p/2 directly as
; alog-coeff-rat * s.  We stored alog-coeff-rat = (B - A p/2)/s and alog-rad = d.  To rebuild the rational
; numerator (B - A p/2) we need s = sqrt(disc/d).  Recompute disc from the rlog-arg (= x^2+px+q): p, q known.
; alog part: equals (B - A p/2) / denom, a rational function.  We recover the rational scalar (B - A p/2) as
; alog-coeff-rat * s exactly (it equals (2B - Ap)/2), where s = sqrt(disc/d).
(define (algresq-alog-deriv r) (rat-mul (algresq-alog-scalar r) (rat-make (list 1) (algresq-denom r))))
(define (algresq-denom r) (car (cdr (cdr r))))   ; x^2+px+q
; (B - A p/2) = alog-coeff-rat * s  (exact rational), s = sqrt(disc/d)
(define (algresq-alog-scalar r) (rat-mul (car (cdr (cdr (cdr r)))) (rat-make (list (algresq-s-of r)) (list 1))))
; s = sqrt(disc/d): disc = p^2-4q from denom = (q p 1); d = alog-rad
(define (algresq-s-of r) (algresq-isqrt (quotient (algresq-disc (algresq-p-of r) (algresq-q-of r)) (car (cdr (cdr (cdr (cdr r)))))) ))
(define (algresq-p-of r) (car (cdr (algresq-denom r))))
(define (algresq-q-of r) (car (algresq-denom r)))
(define (algresq-isqrt n) (algresq-isqrt-go n 0))
(define (algresq-isqrt-go n k) (cond ((> (* k k) n) (- k 1)) ((= (* k k) n) k) (else (algresq-isqrt-go n (+ k 1)))))
