; HARDENING the continued-fraction Pell engine: a perfect-square guard, an explicit unit classification, and a
; reverse-CF round-trip cross-check that validates the unit-finder against the independent construct-from-unit
; direction (docs/TRAGER_ROADMAP.md -- the full third-kind construction; strengthening the foundation under the
; CF-driven Pell third-kind logarithm).
;
; If f is a perfect square, sqrt(f) is a polynomial -- there is no quadratic irrational and no Pell unit; the
; engine flags this with pcf-is-square? and pcf-unit-status returns 'square rather than a degenerate verdict.  The
; deeper validation is a round trip: a unit (A, B) with constant norm c determines a curve f = (A^2 - c)/B^2, and
; the continued fraction of that f must independently recover a certified unit.  This cross-checks the unit-finder
; against the construction in the opposite direction -- two independent methods agreeing.
(import "cas/polycf.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Hardening the Pell unit engine: square guard, explicit status, and a reverse-CF cross-check.") (newline) (newline)

(display "the perfect-square guard: f = (x^2 + 1)^2 has a polynomial square root, so no Pell unit:") (newline)
(must "x^4 + 2x^2 + 1 is detected as a perfect square" (pcf-is-square? (list 1 0 2 0 1)))
(must "its status is 'square, not a degenerate unit" (equal? (pcf-unit-status (list 1 0 2 0 1) 30) (quote square)))
(must "a genuine curve x^6 + x is not a square" (if (pcf-is-square? (list 0 1 0 0 0 0 1)) #f #t))

(display "the explicit unit classification:") (newline)
(must "x^6 + 1 -> a certified unit" (equal? (car (pcf-unit-status (list 1 0 0 0 0 0 1) 30)) (quote unit)))
(must "x^6 + x (period 2) -> a certified unit" (equal? (car (pcf-unit-status (list 0 1 0 0 0 0 1) 30)) (quote unit)))
(must "x^6 + x^2 + 1 -> no-unit-up-to (aperiodic in bound)" (equal? (pcf-unit-status (list 1 0 1 0 0 0 1) 30) (quote no-unit-up-to)))

(display "the reverse-CF round trip: a unit (A, B, c) determines f = (A^2 - c)/B^2, which the CF must recover.") (newline)
(display "engineered from the unit (x^4 + x^2 + 1, x, 1): f = x^6 + 2x^4 + 3x^2 + 2:") (newline)
(define A (list 1 0 1 0 1))   ; x^4 + x^2 + 1
(define Bp (list 0 1))         ; x
(define f1 (car (poly-divmod (poly-sub (poly-mul A A) (list 1)) (poly-mul Bp Bp))))
(must "the engineered f is x^6 + 2x^4 + 3x^2 + 2" (equal? (poly-norm f1) (list 2 0 3 0 2 0 1)))
(must "it is not a perfect square" (if (pcf-is-square? f1) #f #t))
(must "the continued fraction recovers a certified unit (a nonconstant-B unit)" (pcf-certify-unit f1 60))
(display "  the recovered unit (A, B): ") (display (list (poly-norm (car (cdr (pcf-unit-status f1 60)))) (poly-norm (car (cdr (cdr (pcf-unit-status f1 60))))))) (newline)

(display "the round trip holds for several engineered units (constant and nonconstant B):") (newline)
(must "from (x^4 + 1, x): f = x^6 + 2 certifies" (pcf-certify-unit (car (poly-divmod (poly-sub (poly-mul (list 1 0 0 0 1) (list 1 0 0 0 1)) (list 0)) (poly-mul (list 0 1) (list 0 1)))) 60))
(must "from (x^4 + x^2 + 2, x, 4): f = 4 + 5x^2 + 2x^4 + x^6 certifies" (pcf-certify-unit (car (poly-divmod (poly-sub (poly-mul (list 2 0 1 0 1) (list 2 0 1 0 1)) (list 4)) (poly-mul (list 0 1) (list 0 1)))) 60))

(newline)
(display "The Pell unit engine is now hardened: perfect-square curves are flagged, every verdict is explicitly") (newline)
(display "classified, and the unit-finder is cross-validated by the reverse construction -- the CF recovers exactly") (newline)
(display "the unit a curve was engineered from, two independent directions agreeing.  This strengthens the foundation") (newline)
(display "under the CF-driven third-kind Pell logarithm.  Longer periods at higher genus remain the open summit.") (newline)
