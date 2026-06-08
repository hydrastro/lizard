; -*- lisp -*-
; lib/cas/polysolve3.lisp -- EXACT NAMING OF IRRATIONAL COORDINATES for a zero-dimensional polynomial system: take
; the univariate eliminant in a coordinate, peel off its rational roots (rational-root theorem), and name each
; remaining REAL root exactly as an algebraic number -- a minimal polynomial together with an isolating rational
; interval (docs/CAS.md -- summit S3, naming irrational coordinates exactly, beyond polysolve2's rational tuples).
;
; polysolve2 returns rational solution tuples and flags any non-rational coordinate as 'irrational-fiber.  This
; module turns that flag into an exact NAME.  For the eliminant E(x) of a coordinate:
;   - its rational roots r_1, ..., r_k are found exactly (a reduced p/q has p | const, q | lead);
;   - E is deflated by (x - r_1) ... (x - r_k) to the cofactor C whose roots carry the irrational part;
;   - the squarefree part of C is taken, and each of its REAL roots is isolated by Sturm to a rational interval;
;   - each such root is reported as (algebraic minpoly-of-the-root interval), an exact algebraic number, where the
;     minpoly is the squarefree cofactor (the genuine minimal polynomial up to a further factorization, which is
;     not attempted here -- the cofactor is the exact defining polynomial and the interval pins the specific root).
; Everything is exact over Q: rational roots by enumeration, deflation by exact division, real roots by Sturm sign
; counts.  Complex (non-real) roots are not named -- the real roots are the decidable, isolable quantity -- and the
; count of named real values plus any complex remainder is reported honestly.
;
; Public (univariate coefficient lists low->high; "named root" = a rational, or (algebraic minpoly interval)):
;   ps3-rational-roots E       -> rational roots of E (delegates to the rational-root enumeration)
;   ps3-deflate E roots        -> E divided by the product of (x - r) over the given rational roots
;   ps3-irrational-factor E    -> the squarefree cofactor of E after removing all rational-root linear factors
;   ps3-named-real-roots E     -> all REAL roots of E named exactly: rationals as-is, irrationals as algebraic
;                                 numbers (minpoly = the irrational cofactor, plus an isolating interval)
;   ps3-num-complex E          -> (deg of squarefree E) minus (number of distinct real roots): the count of
;                                 non-real roots not named, reported honestly
;   ps3-verify-root E r        -> #t iff a NAMED root r actually satisfies E (rational: E(r)=0; algebraic: the
;                                 minpoly divides E and the interval brackets a sign change)
;
; Verified: x^2 - 2 names two algebraic roots (minpoly x^2-2) bracketed by (-3,0) and (0,3); (x-1)(x^2-2) names
; the rational 1 plus the two algebraic roots; x^2+1 names zero real roots and reports two complex; a rational
; eliminant like x^2-3x+2 names 1 and 2 as rationals.
;
; Builds on poly.lisp, sturm.lisp, polysolve2.lisp, algnum.lisp.

(import "cas/poly.lisp")
(import "cas/sturm.lisp")
(import "cas/polysolve2.lisp")
(import "cas/algnum.lisp")

(define (ps3-len l) (if (null? l) 0 (+ 1 (ps3-len (cdr l)))))

; ----- rational roots (reuse polysolve2's exact enumeration) -----
(define (ps3-rational-roots E) (ps2-rational-roots E))

; ----- deflate E by the product of (x - r) over a list of rational roots -----
(define (ps3-deflate E roots) (if (null? roots) E (ps3-deflate (ps3-div-linear E (car roots)) (cdr roots))))
; divide E by (x - r): (x - r) as a coeff list is (-r 1)
(define (ps3-div-linear E r) (car (poly-divmod E (list (- 0 r) 1))))

; ----- the irrational-bearing factor: deflate by all rational roots, then take squarefree part -----
(define (ps3-irrational-factor E) (sqfree-part (ps3-deflate E (ps3-rational-roots E))))

; ----- name all real roots: rationals as themselves, irrationals as (algebraic minpoly interval) -----
(define (ps3-named-real-roots E) (ps3-app (ps3-rational-roots E) (ps3-name-irrational (ps3-irrational-factor E))))
(define (ps3-app a b) (if (null? a) b (cons (car a) (ps3-app (cdr a) b))))
; for the irrational cofactor C: if it is constant (no irrational roots) -> none; else isolate its real roots,
; naming each (algebraic C interval).  We only name roots of the genuine (degree >= 1) cofactor.
(define (ps3-name-irrational C) (if (< (poly-deg C) 1) (quote ()) (ps3-mk-algs C (isolate-roots C))))
(define (ps3-mk-algs C ivs) (if (null? ivs) (quote ()) (cons (list (quote algebraic) C (car ivs)) (ps3-mk-algs C (cdr ivs)))))

; ----- count of non-real roots of the squarefree part (honest report of what is NOT named) -----
(define (ps3-num-complex E) (- (poly-deg (sqfree-part E)) (num-real-roots E)))

; ----- verify a named root really satisfies E -----
(define (ps3-verify-root E r) (if (pair? r) (ps3-verify-alg E r) (= (poly-eval E r) 0)))
; algebraic root (algebraic minpoly interval): minpoly must divide E AND the interval brackets a sign change of E
(define (ps3-verify-alg E r) (if (ps3-divides? (car (cdr r)) E) (ps3-sign-change? E (car (cdr (cdr r)))) #f))
(define (ps3-divides? d E) (ps3-zero? (car (cdr (poly-divmod E d)))))
(define (ps3-zero? p) (cond ((null? p) #t) ((= (car p) 0) (ps3-zero? (cdr p))) (else #f)))
(define (ps3-sign-change? E iv) (ps3-opp-sign (poly-eval E (car iv)) (poly-eval E (car (cdr iv)))))
(define (ps3-opp-sign a b) (cond ((= a 0) #t) ((= b 0) #t) ((if (> a 0) (< b 0) (> b 0)) #t) (else #f)))
