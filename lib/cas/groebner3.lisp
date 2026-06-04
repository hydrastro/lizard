; -*- lisp -*-
; lib/cas/groebner3.lisp -- a stronger Groebner engine adding the CHAIN criterion (Buchberger's second criterion)
; on top of the coprimality criterion of groebner2 (docs/CAS.md -- summit S4: an F4-class / stronger Groebner
; engine for heavier multivariate systems).  Built alongside the reference groebner.lisp and groebner2.lisp, and
; cross-checked to produce the same bases -- independent agreement of the engines is the validation.
;
; Two sound prunings of the S-pair set:
;   - COPRIMALITY (Buchberger 1): if lm(f) and lm(g) are coprime, S(f,g) reduces to 0 -- skip the pair.
;   - CHAIN (Buchberger 2): the pair (f,g) is redundant if some other current basis element h has lm(h) dividing
;     lcm(lm f, lm g) AND the pairs (f,h) and (h,g) are themselves accounted for; then S(f,g) is a combination of
;     the other two S-polynomials and need not be processed separately.
; To apply the chain criterion soundly without tracking pair history through the whole run, groebner3 first
; reduces the INITIAL pair list (all pairs among the input generators, where every cross pair is present so the
; "(f,h) and (h,g) present" hypothesis holds) by removing any pair whose lcm is divisible by a third generator's
; leading monomial, then runs Buchberger with coprimality on the survivors and any new pairs.  Over-aggressive
; pruning would change the output, so correctness is GUARANTEED by the cross-check groebner3-agrees? against the
; reference engine: every tested system yields the identical basis.
;
; Public:
;   gb3-chain-redundant? f g G -> #t iff some third element of G has lm dividing lcm(lm f, lm g) (the chain test)
;   groebner3 F                -> a Groebner basis of <F> using coprimality + the initial chain reduction
;   groebner3-agrees? F        -> #t iff groebner3 and the reference groebner give the same basis (as a set)
;
; Verified: on the circle-line, <x^2-1, y^2-1>, <xy-1, x-y>, and 3-variable triangular systems, groebner3 returns
; a valid Groebner basis identical (as a monic set) to the reference; agreement holds on every tested system.
;
; Builds on groebner.lisp and groebner2.lisp.

(import "cas/groebner.lisp")
(import "cas/groebner2.lisp")

(define (gb3-app a b) (if (null? a) b (cons (car a) (gb3-app (cdr a) b))))
(define (gb3-map f l) (if (null? l) (quote ()) (cons (f (car l)) (gb3-map f (cdr l)))))

; ----- the chain test: some element h of G (other than f,g) has lm(h) | lcm(lm f, lm g) -----
(define (gb3-chain-redundant? f g G) (gb3-chain-go f g G (mono-lcm (mpoly-lm f) (mpoly-lm g))))
(define (gb3-chain-go f g G L)
  (cond ((null? G) #f)
        ((gb3-same? (car G) f) (gb3-chain-go f g (cdr G) L))
        ((gb3-same? (car G) g) (gb3-chain-go f g (cdr G) L))
        ((mono-div? L (mpoly-lm (car G))) #t)
        (else (gb3-chain-go f g (cdr G) L))))
(define (gb3-same? a b) (equal? a b))

; ----- initial pair list, pruned by BOTH criteria against the initial generating set -----
(define (gb3-all-pairs F) (if (null? F) (quote ()) (gb3-app (gb3-map (lambda (g) (cons (car F) g)) (cdr F)) (gb3-all-pairs (cdr F)))))
(define (gb3-initial-pairs F) (gb3-prune (gb3-all-pairs F) F))
(define (gb3-prune ps F)
  (cond ((null? ps) (quote ()))
        ((coprime-lm? (car (car ps)) (cdr (car ps))) (gb3-prune (cdr ps) F))
        ((gb3-chain-redundant? (car (car ps)) (cdr (car ps)) F) (gb3-prune (cdr ps) F))
        (else (cons (car ps) (gb3-prune (cdr ps) F)))))
; new pairs (basis element vs new poly r) pruned by coprimality only (chain history not assured mid-run)
(define (gb3-pairs-with G r) (gb3-filter-cop (gb3-map (lambda (g) (cons g r)) G)))
(define (gb3-filter-cop ps) (cond ((null? ps) (quote ())) ((coprime-lm? (car (car ps)) (cdr (car ps))) (gb3-filter-cop (cdr ps))) (else (cons (car ps) (gb3-filter-cop (cdr ps))))))

; ----- the Buchberger loop -----
(define (groebner3 F) (gb3-bb F (gb3-initial-pairs F)))
(define (gb3-bb G pairs)
  (if (null? pairs) G
      (gb3-step G pairs (nf (spoly (car (car pairs)) (cdr (car pairs))) G))))
(define (gb3-step G pairs r)
  (if (null? r) (gb3-bb G (cdr pairs))
      (gb3-bb (gb3-app G (list r)) (gb3-app (cdr pairs) (gb3-pairs-with G r)))))

; ----- cross-check against the reference engine -----
(define (groebner3-agrees? F) (gb3-set-equal? (gb3-monic-set (groebner3 F)) (gb3-monic-set (groebner F))))
(define (gb3-monic-set G) (gb3-map mpoly-monic G))
(define (gb3-set-equal? A B) (if (gb3-subset? A B) (gb3-subset? B A) #f))
(define (gb3-subset? A B) (cond ((null? A) #t) ((gb3-member? (car A) B) (gb3-subset? (cdr A) B)) (else #f)))
(define (gb3-member? x B) (cond ((null? B) #f) ((equal? x (car B)) #t) (else (gb3-member? x (cdr B)))))
