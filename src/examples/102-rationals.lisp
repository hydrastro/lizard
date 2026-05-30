; -*- lisp -*-
; ============================================================
;  EXAMPLE 102 — Exact rational arithmetic
; ============================================================

(import "match.lisp")
(import "rational.lisp")

(display "=== CONSTRUCTION (auto-normalizes) ===") (newline)
(display "  2/4 → ") (display (rat->string (make-rat 2 4))) (newline)
(display "  6/3 → ") (display (rat->string (make-rat 6 3))) (newline)
(display "  -3/-6 → ") (display (rat->string (make-rat -3 -6))) (newline)
(display "  3/-6 → ") (display (rat->string (make-rat 3 -6))) (newline)
(newline)

(display "=== ARITHMETIC (exact!) ===") (newline)
(display "  1/2 + 1/3 = ")
(display (rat->string (rat+ (make-rat 1 2) (make-rat 1 3)))) (newline)
(display "  3/4 - 1/6 = ")
(display (rat->string (rat- (make-rat 3 4) (make-rat 1 6)))) (newline)
(display "  2/3 * 3/4 = ")
(display (rat->string (rat* (make-rat 2 3) (make-rat 3 4)))) (newline)
(display "  1/2 / 3/4 = ")
(display (rat->string (rat/ (make-rat 1 2) (make-rat 3 4)))) (newline)
(newline)

(display "=== COMPARISON ===") (newline)
(display "  1/3 < 1/2? ")
(display (rat< (make-rat 1 3) (make-rat 1 2))) (newline)
(display "  2/4 = 1/2? ")
(display (rat= (make-rat 2 4) (make-rat 1 2))) (newline)
(newline)

(display "=== MIXING INTEGERS AND RATIONALS ===") (newline)
(display "  2 + 1/3 = ")
(display (rat->string (rat+ 2 (make-rat 1 3)))) (newline)
(display "  1/2 * 6 = ")
(display (rat->string (rat* (make-rat 1 2) 6))) (newline)
(newline)

(display "=== EXACT HARMONIC NUMBERS ===") (newline)
(display "  H(1) = ") (display (rat->string (harmonic 1))) (newline)
(display "  H(2) = ") (display (rat->string (harmonic 2))) (newline)
(display "  H(3) = ") (display (rat->string (harmonic 3))) (newline)
(display "  H(4) = ") (display (rat->string (harmonic 4))) (newline)
(display "  H(5) = ") (display (rat->string (harmonic 5))) (newline)
(display "  H(10) = ") (display (rat->string (harmonic 10))) (newline)
(display "  (no floating-point error — these are exact)") (newline)
(newline)

(display "=== SUM OF A SERIES (exact) ===") (newline)
; 1/2 + 1/4 + 1/8 + 1/16 = 15/16
(define halves (list (make-rat 1 2) (make-rat 1 4)
                     (make-rat 1 8) (make-rat 1 16)))
(display "  1/2+1/4+1/8+1/16 = ")
(display (rat->string (rat-sum halves))) (newline)
(newline)

(display "=== RECIPROCALS & NEGATION ===") (newline)
(display "  reciprocal of 3/7: ")
(display (rat->string (rat-reciprocal (make-rat 3 7)))) (newline)
(display "  negate 5/8: ")
(display (rat->string (rat-negate (make-rat 5 8)))) (newline)
(newline)

(display "=== End of exact rationals ===") (newline)
