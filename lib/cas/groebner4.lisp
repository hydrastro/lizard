; -*- lisp -*-
; lib/cas/groebner4.lisp -- the REDUCED Groebner basis: the canonical, UNIQUE basis of an ideal for a fixed
; monomial order, obtained by fully inter-reducing a Groebner basis (docs/CAS.md -- summit S4, the canonical
; normal form atop the stronger engines groebner2/groebner3).
;
; A Groebner basis G is REDUCED when (i) every element is monic (leading coefficient 1) and (ii) no monomial of
; any element is divisible by the leading term of a different element.  For a fixed ideal and order the reduced
; Groebner basis is UNIQUE -- it is the canonical form, so two ideals are equal iff their reduced bases are
; identical.  From any Groebner basis we reduce in the standard way:
;   1. MINIMALIZE: drop every g whose leading monomial is divisible by the leading monomial of another element
;      (such g are redundant), leaving a minimal basis;
;   2. INTER-REDUCE: replace each remaining g by its normal form with respect to all the OTHERS (this removes
;      every reducible non-leading monomial), and make it monic.
; Both steps reuse the trusted normal-form reduction nf and the monomial tests from groebner.lisp, so the algebra
; is identical to the engine that produced the basis; only redundancy is stripped.  The result is sound (it
; generates the same ideal and is still a Groebner basis) and canonical, and equality of reduced bases gives an
; exact ideal-equality test.
;
; Public:
;   gb4-minimalize G           -> a minimal Groebner basis (no leading term divides another's)
;   gb4-reduce G               -> the REDUCED (canonical) Groebner basis obtained from a Groebner basis G
;   gb4-reduced F              -> the reduced Groebner basis of the ideal <F> (computes a basis then reduces)
;   gb4-is-reduced? G          -> #t iff G is already reduced (monic, and no monomial reducible by another LT)
;   gb4-ideal-equal? F1 F2     -> #t iff <F1> = <F2>, tested by equality of their reduced Groebner bases (as sets)
;
; Verified: the reduced basis of <x^2+y^2-1, x-y> is canonical and re-reducing it is idempotent; <x^2-1, y^2-1>
; reduces to its canonical form; two different generating sets of the same ideal yield identical reduced bases
; (ideal equality), while a different ideal yields a different reduced basis.
;
; Builds on groebner.lisp (and groebner3.lisp for the basis computation).

(import "cas/groebner.lisp")
(import "cas/groebner3.lisp")

(define (gb4-app a b) (if (null? a) b (cons (car a) (gb4-app (cdr a) b))))
(define (gb4-map f l) (if (null? l) (quote ()) (cons (f (car l)) (gb4-map f (cdr l)))))
(define (gb4-len l) (if (null? l) 0 (+ 1 (gb4-len (cdr l)))))

; ----- everything in G except a designated element (by identity) -----
(define (gb4-others G g) (cond ((null? G) (quote ())) ((equal? (car G) g) (gb4-others (cdr G) g)) (else (cons (car G) (gb4-others (cdr G) g)))))

; ----- MINIMALIZE: drop g whose leading monomial is divisible by some OTHER element's leading monomial -----
(define (gb4-minimalize G) (gb4-min-go G G))
(define (gb4-min-go G all) (cond ((null? G) (quote ())) ((gb4-lt-divisible? (car G) (gb4-others all (car G))) (gb4-min-go (cdr G) (gb4-others all (car G)))) (else (cons (car G) (gb4-min-go (cdr G) all)))))
(define (gb4-lt-divisible? g others) (cond ((null? others) #f) ((mono-div? (mpoly-lm g) (mpoly-lm (car others))) #t) (else (gb4-lt-divisible? g (cdr others)))))

; ----- INTER-REDUCE: reduce each element to normal form w.r.t. the others, then make monic -----
(define (gb4-reduce G) (gb4-interreduce (gb4-minimalize G)))
(define (gb4-interreduce M) (gb4-map (lambda (g) (mpoly-monic (nf g (gb4-others M g)))) M))

; ----- the reduced Groebner basis of an ideal -----
(define (gb4-reduced F) (gb4-reduce (groebner3 F)))

; ----- is G already reduced? monic and no monomial of any g reducible by another's LT -----
(define (gb4-is-reduced? G) (if (gb4-all-monic? G) (gb4-no-cross-reduce? G) #f))
(define (gb4-all-monic? G) (cond ((null? G) #t) ((= (mpoly-lc (car G)) 1) (gb4-all-monic? (cdr G))) (else #f)))
(define (gb4-no-cross-reduce? G) (gb4-ncr-go G G))
(define (gb4-ncr-go G all) (cond ((null? G) #t) ((gb4-some-mono-reducible? (car G) (gb4-others all (car G))) #f) (else (gb4-ncr-go (cdr G) all))))
; some monomial of g divisible by the LT of some other element
(define (gb4-some-mono-reducible? g others) (gb4-smr g (gb4-monos g) others))
(define (gb4-monos g) (gb4-map (lambda (t) (cdr t)) g))
(define (gb4-smr g monos others) (cond ((null? monos) #f) ((gb4-mono-red-by-any? (car monos) others) #t) (else (gb4-smr g (cdr monos) others))))
(define (gb4-mono-red-by-any? m others) (cond ((null? others) #f) ((mono-div? m (mpoly-lm (car others))) #t) (else (gb4-mono-red-by-any? m (cdr others)))))

; ----- ideal equality via canonical reduced bases (as sets) -----
(define (gb4-ideal-equal? F1 F2) (gb4-set-equal? (gb4-reduced F1) (gb4-reduced F2)))
(define (gb4-set-equal? A B) (if (gb4-subset? A B) (gb4-subset? B A) #f))
(define (gb4-subset? A B) (cond ((null? A) #t) ((gb4-member? (car A) B) (gb4-subset? (cdr A) B)) (else #f)))
(define (gb4-member? x B) (cond ((null? B) #f) ((equal? x (car B)) #t) (else (gb4-member? x (cdr B)))))
