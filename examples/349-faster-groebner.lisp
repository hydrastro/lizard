; A faster Groebner-basis engine: Buchberger's algorithm WITH the coprimality criterion (Buchberger's first
; criterion), which skips S-pairs whose leading monomials are coprime -- those S-polynomials provably reduce to
; zero, so skipping them is sound and reduces the work (docs/CAS.md -- frontier e, a more efficient Groebner
; engine).  Built alongside the reference groebner.lisp and CROSS-CHECKED to produce the same bases: independent
; agreement of two engines is the validation.
(import "cas/groebner2.lisp")
(define (chk l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "A faster Groebner engine via Buchberger's coprimality criterion, cross-checked against the reference.") (newline) (newline)

(display "the criterion: S(f,g) reduces to zero when lm(f) and lm(g) are coprime (share no variable):") (newline)
(define px2 (list (cons 1 (list 2 0))))   ; leading monomial x^2
(define py (list (cons 1 (list 0 1))))    ; leading monomial y
(define px (list (cons 1 (list 1 0))))    ; x
(define pxy (list (cons 1 (list 1 1))))   ; xy
(chk "lm(x^2) and lm(y) are coprime -- that S-pair is skipped" (coprime-lm? px2 py))
(chk "lm(x) and lm(xy) are not coprime -- that S-pair is kept" (if (coprime-lm? px pxy) #f #t))

(display "the circle-line system: groebner2 gives a valid basis identical to the reference engine's:") (newline)
(define f1 (list (cons 1 (list 2 0)) (cons 1 (list 0 2)) (cons -1 (list 0 0))))
(define f2 (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))
(define S1 (list f1 f2))
(chk "groebner2 returns a valid Groebner basis (Buchberger's criterion holds)" (groebner-ok? (groebner2 S1)))
(chk "groebner2 agrees with the reference groebner (same basis as a set)" (groebner2-agrees? S1))

(display "a system where the criterion bites: <x^2 - 1, y^2 - 1> has coprime leading terms x^2, y^2:") (newline)
(define g1 (list (cons 1 (list 2 0)) (cons -1 (list 0 0))))
(define g2 (list (cons 1 (list 0 2)) (cons -1 (list 0 0))))
(chk "groebner2 valid on <x^2-1, y^2-1>" (groebner-ok? (groebner2 (list g1 g2))))
(chk "and agrees with the reference" (groebner2-agrees? (list g1 g2)))

(display "more systems, all cross-checked to agree with the reference engine:") (newline)
(define h1 (list (cons 1 (list 1 1)) (cons -1 (list 0 0))))
(define h2 (list (cons 1 (list 1 0)) (cons -1 (list 0 1))))
(chk "<xy-1, x-y> agrees" (groebner2-agrees? (list h1 h2)))
(define e1 (list (cons 1 (list 1 0 0)) (cons -1 (list 0 0 0))))
(define e2 (list (cons 1 (list 0 1 0)) (cons -2 (list 0 0 0))))
(define e3 (list (cons 1 (list 0 0 1)) (cons -3 (list 0 0 0))))
(chk "the 3-variable <x-1, y-2, z-3> agrees" (groebner2-agrees? (list e1 e2 e3)))

(newline)
(display "The optimized engine adds Buchberger's coprimality criterion to skip provably-redundant S-pairs, and is") (newline)
(display "validated the strongest way available: it produces the same Groebner basis as the independent reference") (newline)
(display "engine on every system tested.  Deeper speedups (the chain criterion, an F4-style linear-algebra engine)") (newline)
(display "would lift the heavier multivariate cases further.") (newline)
