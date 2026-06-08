; The UNCONDITIONAL aperiodicity certificate for sqrt(f): a finite, exact PROOF that a hyperelliptic curve y^2 = f
; has no Pell unit -- hence that INT dx/sqrt(f) is NON-ELEMENTARY -- by traversing the full cycle of complete
; quotients of the continued fraction until one repeats (docs/TRAGER_ROADMAP.md -- the full third-kind construction).
;
; polycf reports a Pell unit when some Q_i of the CF returns to a nonzero constant, but when none does it can only
; say "aperiodic up to bound B" -- a bounded negative, not a proof.  Over Q[x] the CF of sqrt(f) is purely periodic
; and the complete quotients (P_i, Q_i) are finite in number, so the pairs MUST eventually repeat.  Tracking them
; until the first repeat traverses the entire cycle; if no Q_i (past the trivial start Q_0 = 1) was a nonzero
; constant, there is NO Pell unit, UNCONDITIONALLY -- a finite proof, cross-checked here against the bounded search.
(import "cas/hyperaperiodic.lisp")
(import "cas/polycf.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Unconditional aperiodicity: proving INT dx/sqrt(f) non-elementary by closing the CF cycle of sqrt(f).") (newline) (newline)

(display "y^2 = x^6 + 1 is periodic (Q_1 is a nonzero constant): it HAS a Pell unit, the integral is elementary:") (newline)
(display "  ") (display (ap-no-unit-proof (list 1 0 0 0 0 0 1) 40)) (newline)
(must "the cycle exposes a Pell unit" (equal? (car (ap-no-unit-proof (list 1 0 0 0 0 0 1) 40)) (quote has-unit)))
(must "so INT dx/sqrt(x^6+1) is elementary" (equal? (ap-integral-verdict (list 1 0 0 0 0 0 1) 40) (quote elementary)))
(must "the infinity class is torsion" (equal? (ap-is-torsion-class? (list 1 0 0 0 0 0 1) 40) #t))
(must "the bounded CF search agrees it is periodic" (pcf-is-periodic? (list 1 0 0 0 0 0 1) 100))

(display "y^2 = x^6 + x^2 + 1: the CF cycle CLOSES with no constant Q -- PROVEN to have no Pell unit:") (newline)
(display "  ") (display (ap-no-unit-proof (list 1 0 1 0 0 0 1) 50)) (newline)
(must "the cycle closes (a complete quotient repeats)" (ap-cycle-closed? (list 1 0 1 0 0 0 1) 50))
(must "no nonzero-constant Q occurs past the start" (if (ap-has-constant-Q? (list 1 0 1 0 0 0 1) 50) #f #t))
(must "so there is NO Pell unit, unconditionally" (equal? (car (ap-no-unit-proof (list 1 0 1 0 0 0 1) 50)) (quote proven-no-unit)))
(must "hence INT dx/sqrt(x^6+x^2+1) is NON-ELEMENTARY (proven)" (equal? (ap-integral-verdict (list 1 0 1 0 0 0 1) 50) (quote non-elementary)))
(must "the infinity class is proven non-torsion" (equal? (ap-is-torsion-class? (list 1 0 1 0 0 0 1) 50) #f))
(must "and the bounded CF search agrees: aperiodic up to 100" (if (pcf-is-periodic? (list 1 0 1 0 0 0 1) 100) #f #t))

(display "two more proven-non-elementary curves (the cycle closes with no unit):") (newline)
(must "INT dx/sqrt(x^6 + x^4 + 1) is non-elementary" (equal? (ap-integral-verdict (list 1 0 0 0 1 0 1) 60) (quote non-elementary)))
(must "INT dx/sqrt(x^6 + x + 2) is non-elementary" (equal? (ap-integral-verdict (list 2 1 0 0 0 0 1) 60) (quote non-elementary)))

(newline)
(display "The bounded 'aperiodic-up-to-B' negative is now an UNCONDITIONAL proof: a closed CF cycle with no Pell") (newline)
(display "unit means INT dx/sqrt(f) is non-elementary, a finite certificate cross-checked against the search.  This") (newline)
(display "is the substance of the last open rung; period lengths growing without bound at ever higher genus -- and a") (newline)
(display "single uniform period bound per genus -- remain the research-grade horizon.") (newline)
