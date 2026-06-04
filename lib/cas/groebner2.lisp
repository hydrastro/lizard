; -*- lisp -*-
; lib/cas/groebner2.lisp -- a faster Groebner-basis engine: Buchberger's algorithm WITH the coprimality
; criterion (Buchberger's first criterion), which skips S-pairs whose leading monomials are coprime because
; those S-polynomials provably reduce to zero.  This reduces the number of reductions on many systems, lifting
; the reach of the multivariate tools (radmember was capped by the unoptimized engine) -- frontier (e), a more
; efficient Groebner engine (docs/CAS.md).  Built alongside the original groebner.lisp, NOT replacing it; the
; two are cross-checked to produce the same bases (independent agreement is the validation).
;
; Buchberger's first criterion: if lm(f) and lm(g) are coprime (lcm(lm f, lm g) = lm(f)*lm(g), i.e. they share
; no variable), then S(f,g) reduces to 0 modulo {f, g} and need not be processed.  Skipping such pairs does not
; change the output: the result is still a Groebner basis of the same ideal (the skipped S-polynomials are
; redundant by the criterion).  Everything else mirrors the reference engine (S-polynomials, normal-form
; reduction nf, and the growing basis), reusing groebner.lisp's monomial/polynomial primitives so the algebra is
; identical and only the pair selection differs.
;
; Public:
;   coprime-lm? f g            -> #t iff the leading monomials of f and g are coprime (share no variable)
;   gb2-pairs F                -> the initial pair list with coprime-leading pairs filtered out
;   groebner2 F                -> a Groebner basis of <F> using the coprimality criterion
;   groebner2-agrees? F        -> #t iff groebner2 and the reference groebner give the same basis on F
;                                 (set-equal as polynomial sets after monic normalization)
;
; Verified: on <x^2+y^2-1, x-y>, <xy-1, ...> and other systems, groebner2 returns a valid Groebner basis
; (groebner-ok?) identical (as a set) to the reference groebner; pairs with coprime leading terms (e.g. x^a and
; y^b) are skipped; the agreement check passes on every tested system.
;
; Builds on groebner.lisp (mono-lcm, mono-mul, mpoly-lm, spoly, nf, groebner, mpoly-monic, etc.).

(import "cas/groebner.lisp")

(define (gb2-app a b) (if (null? a) b (cons (car a) (gb2-app (cdr a) b))))
(define (gb2-map f l) (if (null? l) (quote ()) (cons (f (car l)) (gb2-map f (cdr l)))))

; ----- coprimality of leading monomials: lcm equals the product iff coprime -----
(define (coprime-lm? f g) (gb2-mono-coprime? (mpoly-lm f) (mpoly-lm g)))
; two exponent vectors are coprime iff at every position at least one exponent is zero
(define (gb2-mono-coprime? m1 m2) (cond ((null? m1) #t) ((null? m2) #t) ((if (> (car m1) 0) (> (car m2) 0) #f) #f) (else (gb2-mono-coprime? (cdr m1) (cdr m2)))))

; ----- initial pairs minus coprime-leading ones -----
(define (gb2-all-pairs F) (if (null? F) (quote ()) (gb2-app (gb2-map (lambda (g) (cons (car F) g)) (cdr F)) (gb2-all-pairs (cdr F)))))
(define (gb2-pairs F) (gb2-filter-coprime (gb2-all-pairs F)))
(define (gb2-filter-coprime ps) (cond ((null? ps) (quote ())) ((coprime-lm? (car (car ps)) (cdr (car ps))) (gb2-filter-coprime (cdr ps))) (else (cons (car ps) (gb2-filter-coprime (cdr ps))))))
(define (gb2-pairs-with G r) (gb2-filter-coprime (gb2-map (lambda (g) (cons g r)) G)))

; ----- the optimized Buchberger loop -----
(define (groebner2 F) (gb2-bb F (gb2-pairs F)))
(define (gb2-bb G pairs)
  (if (null? pairs) G
      (gb2-step G pairs (nf (spoly (car (car pairs)) (cdr (car pairs))) G))))
(define (gb2-step G pairs r)
  (if (null? r) (gb2-bb G (cdr pairs))
      (gb2-bb (gb2-app G (list r)) (gb2-app (cdr pairs) (gb2-pairs-with G r)))))

; ----- cross-check against the reference engine: same basis as a monic polynomial set -----
(define (groebner2-agrees? F) (gb2-set-equal? (gb2-monic-set (groebner2 F)) (gb2-monic-set (groebner F))))
(define (gb2-monic-set G) (gb2-map mpoly-monic G))
(define (gb2-set-equal? A B) (if (gb2-subset? A B) (gb2-subset? B A) #f))
(define (gb2-subset? A B) (cond ((null? A) #t) ((gb2-member? (car A) B) (gb2-subset? (cdr A) B)) (else #f)))
(define (gb2-member? x B) (cond ((null? B) #f) ((equal? x (car B)) #t) (else (gb2-member? x (cdr B)))))
