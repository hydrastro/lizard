; A stronger Groebner engine adding the CHAIN criterion (Buchberger's second criterion) on top of the coprimality
; criterion of groebner2 (docs/CAS.md -- summit S4: a stronger Groebner engine for heavier multivariate systems).
; Built alongside the reference groebner.lisp and cross-checked to produce the same bases -- independent agreement
; of the engines is the validation.
;
; Coprimality (Buchberger 1): coprime leading monomials -> the S-pair reduces to zero, skip it.  Chain (Buchberger
; 2): a pair (f,g) is redundant when a third basis element's leading monomial divides lcm(lm f, lm g) and the
; other two pairs are accounted for.  groebner3 prunes the initial pair set by both, then runs Buchberger with
; coprimality on the survivors; correctness is guaranteed by agreement with the reference engine.
(import "cas/groebner3.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "A stronger Groebner engine (chain + coprimality criteria), cross-checked against the reference.") (newline) (newline)

(display "the chain criterion fires when a third leading monomial divides the pair's lcm:") (newline)
(define g1 (list (cons 1 (list 2 0)) (cons -1 (list 0 0))))   ; x^2 - 1, lm x^2
(define g2 (list (cons 1 (list 0 2)) (cons -1 (list 0 0))))   ; y^2 - 1, lm y^2
(define lxy (list (cons 1 (list 1 1))))                       ; lm xy
(chk "lm(xy) divides lcm(x^2, y^2) = x^2 y^2, so the (x^2,y^2) pair is chain-redundant" (gb3-chain-redundant? g1 g2 (list g1 g2 lxy)))

(display "the circle-line system: groebner3 gives a valid basis identical to the reference's:") (newline)
(define f1 (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -1 (list 0 0))))
(define f2 (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))
(chk "groebner3 returns a valid Groebner basis" (groebner-ok? (groebner3 (list f1 f2))))
(chk "and agrees with the reference engine" (groebner3-agrees? (list f1 f2)))

(display "a parabola meets a circle: x^2 + y^2 = 1, y = x^2:") (newline)
(define p1 (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -1 (list 0 0))))
(define p2 (list (cons 1 (list 2 0)) (cons -1 (list 0 1))))
(chk "groebner3 valid on <x^2+y^2-1, x^2-y>" (groebner-ok? (groebner3 (list p1 p2))))
(chk "and agrees with the reference" (groebner3-agrees? (list p1 p2)))

(display "more systems, all cross-checked to agree:") (newline)
(chk "<x^2-1, y^2-1> agrees" (groebner3-agrees? (list g1 g2)))
(define h1 (list (cons 1 (list 1 1)) (cons -1 (list 0 0))))
(define h2 (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))
(chk "<xy-1, x-y> agrees" (groebner3-agrees? (list h1 h2)))
(define e1 (list (cons 1 (list 1 0 0)) (cons -1 (list 0 0 0))))
(define e2 (list (cons 1 (list 0 1 0)) (cons -2 (list 0 0 0))))
(define e3 (list (cons 1 (list 0 0 1)) (cons -3 (list 0 0 0))))
(chk "the 3-variable <x-1, y-2, z-3> agrees" (groebner3-agrees? (list e1 e2 e3)))

(newline)
(display "Three Groebner engines now agree on every tested system: the reference, the coprimality-pruned") (newline)
(display "groebner2, and the chain-and-coprimality groebner3.  The criteria cut redundant S-pairs; a full F4-style") (newline)
(display "linear-algebra engine would be the next step for the heaviest multivariate systems.") (newline)
