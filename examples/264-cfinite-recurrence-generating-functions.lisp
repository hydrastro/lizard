; Rational generating functions for C-finite linear recurrences -- reaching the cases linrec cannot,
; whose characteristic polynomials do NOT split over Q (Fibonacci, Pell).  A recurrence
; a_n = c_1 a_{n-1} + ... + c_d a_{n-d} always has G(x) = sum a_n x^n = N(x)/D(x) with
; D(x) = 1 - c_1 x - ... - c_d x^d, regardless of the roots.  The result is certified against the
; recurrence itself: the Taylor series of N/D (via the certified rational-function series) reproduces the
; terms the recurrence generates directly.
(import "cas/cfrec.lisp")
(define (must l x) (display "  ") (display l) (display " : ") (display (if x "ok" "FAIL")) (newline) (if x #t (raise 'fail)))
(display "Fibonacci a_n = a_{n-1} + a_{n-2}:  G(x) = ") (display (cfrec-gf (list 1 1) (list 0 1)))
(display "  = x/(1 - x - x^2)") (newline)
(display "  first 12 Fibonacci terms: ") (display (cfrec-terms (list 1 1) (list 0 1) 12)) (newline) (newline)
(must "Fibonacci generating function certified (series = recurrence to order 40)" (cfrec-gf-verify (list 1 1) (list 0 1) 40))
(must "Pell a_n = 2a_{n-1}+a_{n-2} generating function certified" (cfrec-gf-verify (list 2 1) (list 0 1) 40))
(must "Tribonacci generating function certified" (cfrec-gf-verify (list 1 1 1) (list 0 0 1) 40))
(must "a_n = 4a_{n-1} - 4a_{n-2} (repeated root 2) certified" (cfrec-gf-verify (list 4 -4) (list 1 2) 40))
(newline) (display "C-finite generating functions certified, including irrational-root recurrences.") (newline)
