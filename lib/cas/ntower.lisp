; -*- lisp -*-
; lib/cas/ntower.lisp -- a UNIFORM differential tower of arbitrary depth, the structural foundation for a
; recursive Risch decision procedure.
;
; The existing tower code is level-specific: Q(x) at level 0 (ratfun/rischrat), one extension at level 1
; (risch.lisp), two extensions at level 2 (the tower2* family + the h2-integrate driver).  Each has its own
; representation, so nothing recurses.  This module replaces that with ONE representation that nests to any
; depth, where the coefficients at level n are themselves tower elements at level n-1, bottoming out at the
; trusted base field Q(x) (rat).  The whole point of the Risch procedure is that the derivation -- and the
; integration that inverts it -- have the SAME shape at every level, differing only in the monomial on top:
;
;     D( sum_i a_i theta^i ) = sum_i ( D_below(a_i) ) theta^i  +  (chain rule from D theta)
;
; where D_below is the derivation of the level below.  We make that literal: a recursive derivation that
; calls itself on the coefficients and adds the monomial's chain-rule contribution.
;
; ---------- representation ----------
; A tower is described by a list of MONOMIAL SPECS, innermost first (level 1 .. level n):
;     spec = (list 'prim  darg)   ; theta is primitive: D theta = darg, with darg a tower element ONE LEVEL
;                                  ;   below this monomial (e.g. theta = log f  =>  darg = f'/f in the lower field)
;          | (list 'exp   uarg)   ; theta = exp(uarg): D theta = uarg' * theta, uarg a lower-level element;
;                                  ;   we store the precomputed logarithmic derivative w = uarg' (lower level)
; An ELEMENT at level n is a list (low-to-high) of coefficients, each an element at level n-1; level 0 is rat.
; The zero element at any level is ().  A constant c (rat) is lifted to level n as (((... c ...))) via nt-lift.
;
; This file builds the differential algebra (arithmetic + the recursive derivation) and certifies it against
; the existing level-1 and level-2 derivations.  The recursive INTEGRATION driver is built on top separately.

(import "cas/tower.lisp")

; ---------- generic level-parameterised arithmetic ----------
; Every operation takes the level L (0 = rat base).  At L=0 we delegate to rat-*; above, we recurse coefficientwise.
; A level-0 (rat) value may arrive as the universal empty zero (); coerce it to the rat zero.
(define (nt-r p) (if (null? p) (rat-zero) p))
(define (nt-z0 r) (if (rat-zero? r) (quote ()) r))   ; collapse rat zero to the universal zero ()
(define (nt-zero) (quote ()))
(define (nt-zero? L p) (if (= L 0) (rat-zero? (nt-r p)) (nt-all-zero? L p)))
(define (nt-all-zero? L p) (if (null? p) #t (if (nt-zero? (- L 1) (car p)) (nt-all-zero? L (cdr p)) #f)))
(define (nt-norm L p)                                   ; strip high-order zero coefficients
  (if (= L 0) p (reverse (nt-drop0 L (reverse p)))))
(define (nt-drop0 L p) (if (null? p) (quote ()) (if (nt-zero? (- L 1) (car p)) (nt-drop0 L (cdr p)) p)))
(define (nt-deg L p) (- (length (nt-norm L p)) 1))

(define (nt-add L p q)
  (if (= L 0) (nt-z0 (rat-add (nt-r p) (nt-r q)))
      (cond ((null? p) q) ((null? q) p)
            (else (cons (nt-add (- L 1) (car p) (car q)) (nt-add L (cdr p) (cdr q)))))))
(define (nt-neg L p) (if (= L 0) (nt-z0 (rat-neg (nt-r p))) (if (null? p) (quote ()) (cons (nt-neg (- L 1) (car p)) (nt-neg L (cdr p))))))
(define (nt-sub L p q) (nt-add L p (nt-neg L q)))

; scalar (lower-level element) times a level-L element: multiply each coefficient by e (a level L-1 element)
(define (nt-cscale L e p) (if (null? p) (quote ()) (cons (nt-mul (- L 1) e (car p)) (nt-cscale L e (cdr p)))))
(define (nt-shift L p k) (if (= k 0) p (cons (nt-zero) (nt-shift L p (- k 1)))))   ; multiply by theta^k
(define (nt-mul L p q)
  (if (= L 0) (nt-z0 (rat-mul (nt-r p) (nt-r q)))
      (if (null? p) (quote ())
          (nt-add L (nt-cscale L (car p) q) (nt-shift L (nt-mul L (cdr p) q) 1)))))

; lift a base-field (rat) constant up to level L
(define (nt-lift L c) (if (= L 0) c (list (nt-lift (- L 1) c))))
(define (nt-monomial L e k) (nt-shift L (list e) k))   ; e * theta^k, e a level L-1 element

; ---------- the recursive derivation ----------
; specs: innermost first. The element at level L lives over the tower described by the first L specs.
; D_L( sum_i a_i theta_L^i ) = sum_i D_{L-1}(a_i) theta_L^i + sum_{i>=1} i a_i (D theta_L) theta_L^{i-1}
; where D theta_L is: (prim) the stored darg (a level L-1 element); (exp) w * theta_L, contributing
;   i a_i w theta_L^i instead (the exp chain rule is diagonal: D theta = w theta).
(define (nt-dcoeffs L specs p) (if (null? p) (quote ()) (cons (nt-deriv (- L 1) specs (car p)) (nt-dcoeffs L specs (cdr p)))))
; primitive chain: sum_{i>=1} i a_i (darg) theta^{i-1}  -- darg is level L-1, multiply then drop one theta degree
(define (nt-chain-prim L darg p i)
  (if (null? p) (quote ())
      (nt-add L (nt-monomial L (nt-mul (- L 1) (nt-iscale (- L 1) i (car p)) darg) (- i 1))
                (nt-chain-prim L darg (cdr p) (+ i 1)))))
; exponential chain: sum_{i>=1} i a_i w theta^i  (diagonal)
(define (nt-chain-exp L w p i)
  (if (null? p) (quote ())
      (nt-add L (nt-monomial L (nt-mul (- L 1) (nt-iscale (- L 1) i (car p)) w) i)
                (nt-chain-exp L w (cdr p) (+ i 1)))))
(define (nt-iscale L n p)                               ; integer n times a level-L element
  (if (= L 0) (nt-z0 (rat-scale n (nt-r p))) (if (null? p) (quote ()) (cons (nt-iscale (- L 1) n (car p)) (nt-iscale L n (cdr p))))))

(define (nt-deriv L specs p)
  (if (= L 0) (nt-z0 (rat-deriv (nt-r p)))
      (let ((spec (nt-nth specs (- L 1))))             ; the monomial introduced AT this level (1-indexed -> L-1)
        (let ((base (nt-dcoeffs L specs p)))
          (if (null? p) (quote ())
              (if (equal? (car spec) (quote prim))
                  (nt-add L base (nt-chain-prim L (car (cdr spec)) (cdr p) 1))
                  (nt-add L base (nt-chain-exp L (car (cdr spec)) (cdr p) 1))))))))
(define (nt-nth lst i) (if (= i 0) (car lst) (nt-nth (cdr lst) (- i 1))))
