; -*- lisp -*-
; lib/cas/polysolve.lisp -- solving multivariate polynomial systems by Groebner-basis elimination, built ON TOP
; of the existing groebner.lisp (Buchberger) rather than reimplementing it (docs/CAS.md -- frontier 4,
; multivariate: an application on the Groebner machinery).
;
; For a system f_1 = ... = f_k = 0 over Q, the ideal I = <f_1,...,f_k> captures all polynomial consequences, and
; a Groebner basis G of I makes the system's structure decidable:
;   - CONSISTENCY: the system has a (possibly complex) solution iff 1 is NOT in I, i.e. iff the reduced Groebner
;     basis is not {1}; by the Weak Nullstellensatz, G = {1} exactly when the equations have no common zero.
;   - ELIMINATION / TRIANGULAR FORM: under lexicographic order the Groebner basis is triangular -- it contains a
;     polynomial in the last variable alone, then polynomials reintroducing earlier variables -- which is the
;     "solved" form of the system (back-substitution from the last variable up).
;   - ZERO-DIMENSIONALITY (finitely many solutions): I is zero-dimensional iff, for every variable x_i, some
;     basis element has leading monomial a pure power x_i^d; equivalently each variable is "algebraic over Q"
;     modulo I.  This is the finite-solution test.
;   - IDEAL MEMBERSHIP: a polynomial p vanishes on all solutions-as-forced-by-I iff p reduces to 0 modulo G
;     (in-ideal?), the certifiable consequence test.
;
; Everything here is exact over Q and gated by reduction modulo the basis (the normal form nf / in-ideal?), so a
; "consistent", "zero-dimensional", or "p is a consequence" verdict is backed by the Groebner certificate, and
; the triangular basis is returned for the caller to back-substitute.  Approximating or naming numeric roots is
; out of scope; the contribution is the exact structural decision and the eliminant.
;
; Public (polynomials and monomials in groebner.lisp's representation; nv = number of variables):
;   psys-solve F            -> the lex Groebner basis of <F> (the triangular / eliminated system)
;   psys-consistent? F      -> #t iff the system has a common zero (Groebner basis != {1})
;   psys-zero-dim? F nv     -> #t iff finitely many solutions (every variable has a pure-power leading monomial)
;   psys-eliminant F nv i   -> a basis element that is univariate in variable i (its leading monomial is a pure
;                              power of x_i), or () if none -- the eliminant for that variable
;   psys-consequence? p F   -> #t iff p vanishes by virtue of the system (p reduces to 0 modulo the basis)
;
; Verified: <x^2+y^2-1, x-y> is consistent and zero-dimensional with eliminant 2y^2-1 (the circle-meets-line
; system); <x+y-2, x+y-3> (parallel/inconsistent) has Groebner basis {1} and is reported inconsistent; the
; consequence test confirms x^2+y^2-1 reduces to 0 modulo the basis and a non-consequence does not.
;
; Builds only on groebner.lisp.

(import "cas/groebner.lisp")

(define (ps-len l) (if (null? l) 0 (+ 1 (ps-len (cdr l)))))
(define (ps-nth l k) (if (= k 0) (car l) (ps-nth (cdr l) (- k 1))))

; ----- solve = the lex Groebner basis (the triangular eliminated system) -----
(define (psys-solve F) (groebner F))

; ----- consistency: basis is {1} (a single constant polynomial) iff inconsistent -----
(define (psys-consistent? F) (ps-not-unit? (groebner F)))
(define (ps-not-unit? G) (if (ps-basis-has-constant? G) #f #t))
; a basis contains the unit ideal iff some element is a nonzero constant (leading monomial = all-zero exps)
(define (ps-basis-has-constant? G) (cond ((null? G) #f) ((ps-poly-constant? (car G)) #t) (else (ps-basis-has-constant? (cdr G)))))
(define (ps-poly-constant? p) (if (null? p) #f (ps-mono-zero? (cdr (car p)))))
(define (ps-mono-zero? m) (cond ((null? m) #t) ((= (car m) 0) (ps-mono-zero? (cdr m))) (else #f)))

; ----- zero-dimensionality: every variable i has SOME basis element whose leading monomial is x_i^d (a pure
; power: exponent positive in slot i, zero elsewhere) -----
(define (psys-zero-dim? F nv) (ps-zd-go (groebner F) nv 0))
(define (ps-zd-go G nv i) (cond ((>= i nv) #t) ((ps-has-pure-power? G i) (ps-zd-go G nv (+ i 1))) (else #f)))
(define (ps-has-pure-power? G i) (cond ((null? G) #f) ((ps-lm-pure-power? (mpoly-lm (car G)) i) #t) (else (ps-has-pure-power? (cdr G) i))))
(define (ps-lm-pure-power? m i) (if (> (ps-nth m i) 0) (ps-others-zero? m i 0) #f))
(define (ps-others-zero? m i j) (cond ((>= j (ps-len m)) #t) ((= j i) (ps-others-zero? m i (+ j 1))) ((= (ps-nth m j) 0) (ps-others-zero? m i (+ j 1))) (else #f)))

; ----- the eliminant for variable i: a basis element univariate in x_i (pure-power leading monomial) -----
(define (psys-eliminant F nv i) (ps-find-elim (groebner F) i))
(define (ps-find-elim G i) (cond ((null? G) (quote ())) ((ps-lm-pure-power? (mpoly-lm (car G)) i) (car G)) (else (ps-find-elim (cdr G) i))))

; ----- consequence: p reduces to 0 modulo the Groebner basis -----
(define (psys-consequence? p F) (in-ideal? p (groebner F)))
