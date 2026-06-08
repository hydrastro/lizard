; The CF-DRIVEN genus-2 third-kind Pell construction: given ANY hyperelliptic curve y^2 = f, find the fundamental
; unit by the continued fraction of sqrt(f) and construct the certified third-kind logarithm from it -- generalizing
; hyperpell past the f = h^2 + c family to every periodic curve, including genuine period-2 curves
; (docs/TRAGER_ROADMAP.md -- the full third-kind construction).
;
; hyperpell built the nonconstant-B arguments g0^n only on f = h^2 + c, where the unit g0 = h + y is obvious.  polycf
; now computes the fundamental unit (A, B) for an arbitrary curve whose sqrt(f) is periodic, so the same power
; construction applies with g0 = A + B y.  This bridge asks polycf for the certified unit, builds g0 in the field,
; and produces INT ((g0^n)'/g0^n) dx = log(g0^n), each gated by the differentiation certificate.  A non-periodic
; curve reports no-unit, the honest bounded negative.
(import "cas/hyperpellcf.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "Third-kind logarithms from CF-found Pell units -- now for curves where the unit is not obvious.") (newline) (newline)

(display "a genuine PERIOD-2 curve y^2 = x^6 + x, which is NOT of the form h^2 + c:") (newline)
(define f (list 0 1 0 0 0 0 1))
(must "the continued fraction finds a certified fundamental unit" (hpc-has-unit? f 30))
(display "  the unit (A, B) = ") (display (list (poly-norm (car (hpc-unit f 30))) (poly-norm (car (cdr (hpc-unit f 30)))))) (newline)
(must "INT (g0'/g0) dx = log(g0) certifies" (hpc-log-cert f 30 1))
(must "INT ((g0^2)'/g0^2) dx = log(g0^2) certifies" (hpc-log-cert f 30 2))
(must "INT ((g0^3)'/g0^3) dx = log(g0^3) certifies" (hpc-log-cert f 30 3))
(display "  INT ((g0^2)'/g0^2) dx = log of the argument ") (display (hpc-log f 30 2)) (newline)

(display "the period-1 family still works and agrees with hyperpell, e.g. y^2 = x^6 + 1:") (newline)
(define f1 (list 1 0 0 0 0 0 1))
(must "x^6 + 1 has a certified unit" (hpc-has-unit? f1 30))
(must "the unit is (x^3, 1), as hyperpell found by inspection" (equal? (list (poly-norm (car (hpc-unit f1 30))) (poly-norm (car (cdr (hpc-unit f1 30))))) (list (list 0 0 0 1) (list 1))))
(must "its g0^2 logarithm certifies" (hpc-log-cert f1 30 2))

(display "soundness: a curve whose sqrt(f) is not periodic within the bound reports no-unit, not forced:") (newline)
(define fn (list 1 0 1 0 0 0 1))
(must "y^2 = x^6 + x^2 + 1 reports no-unit" (if (hpc-has-unit? fn 30) #f #t))
(must "the no-unit verdict propagates through the log construction" (equal? (hpc-log fn 30 2) (quote no-unit)))

(newline)
(display "The third-kind Pell construction now works for any periodic hyperelliptic curve: the continued-fraction") (newline)
(display "engine supplies the certified fundamental unit, the genus-agnostic field arithmetic builds the logarithm,") (newline)
(display "and the differentiation certificate gates every answer -- no longer limited to the f = h^2 + c family.") (newline)
(display "Longer periods at higher genus, and unconditional aperiodicity proofs, remain the open summit.") (newline)
