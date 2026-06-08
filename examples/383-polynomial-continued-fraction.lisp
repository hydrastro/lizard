; The CONTINUED FRACTION of sqrt(f) over Q[x], with periodicity detection and the fundamental-unit convergent --
; the function-field analogue of the numeric continued fraction for sqrt(N), the engine that decides whether a
; hyperelliptic curve y^2 = f has a Pell unit and produces it (docs/TRAGER_ROADMAP.md -- generalizing the
; f = h^2 + c family of hyperpell to curves whose sqrt(f) is periodic, where the unit is not obvious by inspection).
;
; For f of even degree, sqrt(f) has a polynomial part a0; Abel's recurrence expands sqrt(f) as a continued fraction
; with complete quotients (P_i + sqrt f)/Q_i.  The expansion is periodic exactly when some Q_i returns to a nonzero
; constant; the curve then has a fundamental Pell unit, read off the convergent.  Every unit is gated by its norm
; A^2 - B^2 f being a nonzero constant; a candidate that does not certify is reported, never asserted.
(import "cas/polycf.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise (quote fail))))

(display "The continued fraction of sqrt(f) over Q[x]: deciding the Pell unit of a hyperelliptic curve.") (newline) (newline)

(display "the polynomial part of sqrt(f):") (newline)
(must "polypart(sqrt(x^6 + 1)) = x^3" (equal? (poly-norm (pcf-polypart (list 1 0 0 0 0 0 1))) (list 0 0 0 1)))
(must "polypart(sqrt((x^3 + x)^2 + 2)) = x^3 + x" (equal? (poly-norm (pcf-polypart (list 2 0 1 0 2 0 1))) (list 0 1 0 1)))
(must "polypart of an odd-degree f is reported not-even-degree" (equal? (pcf-polypart (list 1 0 0 0 0 1)) (quote not-even-degree)))

(display "periodicity and the fundamental unit on y^2 = x^6 + 1 (genus 2):") (newline)
(define f1 (list 1 0 0 0 0 0 1))
(must "sqrt(x^6 + 1) is periodic with period 1" (= (pcf-period f1 30) 1))
(define u1 (pcf-unit-verified f1 30))
(display "  the verified fundamental unit (A, B): ") (display (list (poly-norm (car u1)) (poly-norm (car (cdr u1))))) (newline)
(must "the unit is (x^3, 1)" (equal? (list (poly-norm (car u1)) (poly-norm (car (cdr u1)))) (list (list 0 0 0 1) (list 1))))
(must "its norm A^2 - B^2 f is the constant -1" (equal? (poly-norm (pcf-unit-norm f1 (car u1) (car (cdr u1)))) (list -1)))
(must "the Pell certificate holds" (pcf-certify-unit f1 30))

(display "the engine computes the unit on a curve where it is not obvious, e.g. y^2 = (x^3 + x)^2 + 2:") (newline)
(define f2 (list 2 0 1 0 2 0 1))
(must "it certifies a fundamental unit" (pcf-certify-unit f2 30))
(must "the unit is (x^3 + x, 1)" (equal? (list (poly-norm (car (pcf-unit-verified f2 30))) (poly-norm (car (cdr (pcf-unit-verified f2 30))))) (list (list 0 1 0 1) (list 1))))

(display "and it recovers the classical genus-0 unit of sqrt(x^2 + 1):") (newline)
(must "x^2 + 1 is periodic" (pcf-is-periodic? (list 1 0 1) 30))
(must "its unit (x, 1) certifies" (pcf-certify-unit (list 1 0 1) 30))

(display "soundness: a curve whose sqrt(f) is not periodic within the bound is reported, not forced:") (newline)
(must "y^2 = x^6 + x^2 + 1 returns no-unit-up-to (aperiodic in bound)" (equal? (pcf-unit-verified (list 1 0 1 0 0 0 1) 30) (quote no-unit-up-to)))

(display "and a genuine PERIOD-2 curve y^2 = x^6 + x now certifies its fundamental unit:") (newline)
(must "sqrt(x^6 + x) has period 2" (= (pcf-period (list 0 1 0 0 0 0 1) 30) 2))
(must "its period-2 unit certifies (constant norm)" (pcf-certify-unit (list 0 1 0 0 0 0 1) 30))
(define u3 (pcf-unit-verified (list 0 1 0 0 0 0 1) 30))
(display "  the verified period-2 unit (A, B): ") (display (list (poly-norm (car u3)) (poly-norm (car (cdr u3))))) (newline)
(must "the unit norm A^2 - B^2 f is a constant" (= (pcf-len (poly-norm (pcf-unit-norm (list 0 1 0 0 0 0 1) (car u3) (car (cdr u3))))) 1))

(newline)
(display "The continued-fraction engine for sqrt(f) now decides periodicity and produces the fundamental Pell unit,") (newline)
(display "certified by its constant norm -- so the genus-2 third-kind Pell construction works for curves where the") (newline)
(display "unit is not visible by inspection, including genuine period-2 curves, not only the f = h^2 + c family.") (newline)
(display "Longer periods at higher genus, and unconditional aperiodicity proofs, remain the open summit.") (newline)
